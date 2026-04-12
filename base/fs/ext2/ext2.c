/*++

Module Name:

    ext2.c

Abstract:

    This module implements ext2 filesystem mount/unmount and the
    high-level operations exposed to the shell: ls, cat, mkdir,
    rm, and write.

Author:

    Noah Juopperi <nipfswd@gmail.com>

--*/

#include "inc/ext2.h"
#include "inc/ext2_block.h"
#include "inc/ext2_inode.h"
#include "inc/ext2_dir.h"
#include "../../kernel/inc/video.h"
#include "../../kernel/inc/string.h"
#include "../inc/ata.h"

EXT2_FS g_Fs;

/*++

Routine Description:

    Mounts an ext2 filesystem by reading and validating the superblock.

Arguments:

    PartitionLba - LBA of the first sector of the partition.

Return Value:

    0 on success, -1 on failure.

--*/

INT
Ext2Mount (
    uint32_t PartitionLba
    )
{
    uint8_t  Buf[1024];
    uint32_t SbLba;

    /* Superblock is at byte 1024 = sectors 2-3 (0-based) of the partition. */
    SbLba = PartitionLba + (EXT2_SUPERBLOCK_OFFSET / ATA_SECTOR_SIZE);

    if (AtaReadSectors(SbLba, 2, Buf) != ATA_OK) {
        Print("ext2: ATA read failed during mount\n");
        return -1;
    }

    /* Copy into the cached superblock. */
    {
        int      I;
        uint8_t* Dst = (uint8_t*)&g_Fs.Sb;
        for (I = 0; I < (int)sizeof(EXT2_SUPERBLOCK); I++) {
            Dst[I] = Buf[I];
        }
    }

    if (g_Fs.Sb.s_magic != EXT2_MAGIC) {
        Print("ext2: bad magic — not an ext2 partition\n");
        return -1;
    }

    g_Fs.BlockSize       = 1024u << g_Fs.Sb.s_log_block_size;
    g_Fs.SectorsPerBlock = g_Fs.BlockSize / ATA_SECTOR_SIZE;
    g_Fs.PartLba         = PartitionLba;
    g_Fs.GroupCount      =
        (g_Fs.Sb.s_blocks_count + g_Fs.Sb.s_blocks_per_group - 1)
        / g_Fs.Sb.s_blocks_per_group;
    g_Fs.Mounted         = 1;

    Print("ext2: mounted OK — block size ");
    PrintInt((int)g_Fs.BlockSize);
    Print(" bytes, ");
    PrintInt((int)g_Fs.GroupCount);
    Print(" group(s)\n");

    return 0;
}

/*++

Routine Description:

    Unmounts the filesystem.

Arguments:

    None.

Return Value:

    None.

--*/

VOID
Ext2Unmount (
    VOID
    )
{
    g_Fs.Mounted = 0;
}

/* ------------------------------------------------------------------ */
/* Shell-level operations                                               */
/* ------------------------------------------------------------------ */

/*
 * Callback context for Ext2Ls.
 */
typedef struct _LS_CTX {
    int Count;
} LS_CTX;

/*
 * Callback invoked by Ext2DirWalk for each directory entry during ls.
 */
static INT
LsCallback (
    const EXT2_DIRENT* Entry,
    void*              Context
    )
{
    LS_CTX* Ctx = (LS_CTX*)Context;

    Print("  ");
    {
        int I;
        for (I = 0; I < (int)Entry->name_len; I++) {
            PrintChar(Entry->name[I], 0x07);
        }
    }

    if (Entry->file_type == EXT2_FT_DIR) {
        Print("/");
    }

    Print("\n");
    Ctx->Count++;
    return 0;
}

/*++

Routine Description:

    Lists the contents of a directory path.

Arguments:

    Path - Absolute path to directory.

Return Value:

    0 on success, -1 on error.

--*/

INT
Ext2Ls (
    const char* Path
    )
{
    uint32_t   Ino;
    EXT2_INODE Inode;
    LS_CTX     Ctx;

    if (!g_Fs.Mounted) {
        Print("ext2: not mounted\n");
        return -1;
    }

    Ino = Ext2Lookup(Path);
    if (Ino == 0) {
        Print("ls: path not found\n");
        return -1;
    }

    if (Ext2ReadInode(Ino, &Inode) != 0) {
        return -1;
    }

    if ((Inode.i_mode & EXT2_S_IFDIR) == 0) {
        Print("ls: not a directory\n");
        return -1;
    }

    Ctx.Count = 0;
    Ext2DirWalk(Ino, LsCallback, &Ctx);

    if (Ctx.Count == 0) {
        Print("  (empty)\n");
    }

    return 0;
}

/*++

Routine Description:

    Prints the contents of a regular file to the screen.

Arguments:

    Path - Absolute path to file.

Return Value:

    0 on success, -1 on error.

--*/

INT
Ext2Cat (
    const char* Path
    )
{
    uint32_t   Ino;
    EXT2_INODE Inode;
    uint32_t   Block;
    uint32_t   BytesLeft;
    uint8_t*   Buf;
    uint8_t    Scratch[4096]; /* max block size */

    if (!g_Fs.Mounted) {
        Print("ext2: not mounted\n");
        return -1;
    }

    Ino = Ext2Lookup(Path);
    if (Ino == 0) {
        Print("cat: file not found\n");
        return -1;
    }

    if (Ext2ReadInode(Ino, &Inode) != 0) {
        return -1;
    }

    if (Inode.i_mode & EXT2_S_IFDIR) {
        Print("cat: is a directory\n");
        return -1;
    }

    BytesLeft = Inode.i_size;
    Block     = 0;
    Buf       = Scratch;

    while (BytesLeft > 0) {
        uint32_t PhysBlock;
        uint32_t Chunk;
        uint32_t I;

        PhysBlock = Ext2InodeGetBlock(&Inode, Block);
        if (PhysBlock == 0) {
            break;
        }

        if (Ext2ReadBlock(PhysBlock, Buf) != 0) {
            break;
        }

        Chunk = BytesLeft < g_Fs.BlockSize ? BytesLeft : g_Fs.BlockSize;

        for (I = 0; I < Chunk; I++) {
            PrintChar((char)Buf[I], 0x07);
        }

        BytesLeft -= Chunk;
        Block++;
    }

    Print("\n");
    return 0;
}

/*++

Routine Description:

    Creates a directory at the given absolute path.  Allocates a fresh
    inode, writes '.' and '..' into its first data block directly so
    that both entries are packed correctly into a single block, inserts
    the new name into the parent directory, and bumps the parent link
    count for the '..' back-reference.

Arguments:

    Path - Absolute path for the new directory (parent must exist).

Return Value:

    0 on success, -1 on error.

--*/

INT
Ext2Mkdir (
    const char* Path
    )
{
    char         ParentPath[256];
    const char*  Name;
    int          I;
    int          LastSlash;
    uint32_t     ParentIno;
    uint32_t     NewIno;
    EXT2_INODE   NewInode;
    EXT2_INODE   ParentInode;
    uint32_t     DataBlock;
    uint8_t      Buf[4096];
    EXT2_DIRENT* Entry;
    uint32_t     DotRecLen;
    uint32_t     J;

    if (!g_Fs.Mounted) {
        Print("ext2: not mounted\n");
        return -1;
    }

    /* Split path into parent path and new directory name. */
    LastSlash = -1;
    for (I = 0; Path[I]; I++) {
        if (Path[I] == '/') {
            LastSlash = I;
        }
    }

    if (LastSlash < 0) {
        Print("mkdir: invalid path\n");
        return -1;
    }

    if (LastSlash == 0) {
        ParentPath[0] = '/';
        ParentPath[1] = '\0';
    } else {
        for (I = 0; I < LastSlash; I++) {
            ParentPath[I] = Path[I];
        }
        ParentPath[LastSlash] = '\0';
    }

    Name = Path + LastSlash + 1;

    if (Name[0] == '\0') {
        Print("mkdir: empty name\n");
        return -1;
    }

    /* Verify parent exists. */
    ParentIno = Ext2Lookup(ParentPath);
    if (ParentIno == 0) {
        Print("mkdir: parent not found\n");
        return -1;
    }

    /* Refuse to create if target already exists. */
    if (Ext2Lookup(Path) != 0) {
        Print("mkdir: already exists\n");
        return -1;
    }

    /* Allocate a fresh inode for the new directory. */
    NewIno = Ext2AllocInode(0, 1 /* IsDir */);
    if (NewIno == 0) {
        Print("mkdir: no free inodes\n");
        return -1;
    }

    /* Zero and populate the new inode. */
    {
        uint8_t* P = (uint8_t*)&NewInode;
        for (J = 0; J < (uint32_t)sizeof(EXT2_INODE); J++) {
            P[J] = 0;
        }
    }

    NewInode.i_mode        = (uint16_t)(EXT2_S_IFDIR | 0x1ED); /* 0755 */
    NewInode.i_links_count = 2;              /* entry in parent + '.'    */
    NewInode.i_size        = g_Fs.BlockSize;
    NewInode.i_blocks      = g_Fs.SectorsPerBlock * 2; /* 512-byte units */

    /* Allocate the first data block to hold '.' and '..'. */
    DataBlock = Ext2AllocBlock(0);
    if (DataBlock == 0) {
        Ext2FreeInode(NewIno);
        Print("mkdir: no free blocks\n");
        return -1;
    }

    /* Zero the block. */
    for (J = 0; J < g_Fs.BlockSize; J++) {
        Buf[J] = 0;
    }

    /*
     * '.' entry.
     * Real size = 8 + 1 = 9, rounded up to 12 (4-byte aligned).
     * rec_len is set to exactly 12 so '..' follows immediately.
     */
    DotRecLen        = 12;
    Entry            = (EXT2_DIRENT*)Buf;
    Entry->inode     = NewIno;
    Entry->rec_len   = (uint16_t)DotRecLen;
    Entry->name_len  = 1;
    Entry->file_type = EXT2_FT_DIR;
    Entry->name[0]   = '.';

    /*
     * '..' entry.
     * rec_len spans the rest of the block so walkers advance cleanly
     * to the next block boundary.
     */
    Entry            = (EXT2_DIRENT*)(Buf + DotRecLen);
    Entry->inode     = ParentIno;
    Entry->rec_len   = (uint16_t)(g_Fs.BlockSize - DotRecLen);
    Entry->name_len  = 2;
    Entry->file_type = EXT2_FT_DIR;
    Entry->name[0]   = '.';
    Entry->name[1]   = '.';

    if (Ext2WriteBlock(DataBlock, Buf) != 0) {
        Ext2FreeInode(NewIno);
        Ext2FreeBlock(DataBlock);
        Print("mkdir: block write failed\n");
        return -1;
    }

    /* Wire the data block into the new inode and write it. */
    NewInode.i_block[0] = DataBlock;

    if (Ext2WriteInode(NewIno, &NewInode) != 0) {
        Ext2FreeInode(NewIno);
        Ext2FreeBlock(DataBlock);
        Print("mkdir: inode write failed\n");
        return -1;
    }

    /* Insert the new name into the parent directory. */
    if (Ext2DirAddEntry(ParentIno, Name, NewIno, EXT2_FT_DIR) != 0) {
        Ext2FreeInode(NewIno);
        Ext2FreeBlock(DataBlock);
        Print("mkdir: failed to add directory entry\n");
        return -1;
    }

    /*
     * Each subdirectory's '..' contributes one extra hard link to the
     * parent, so bump the parent link count.
     */
    if (Ext2ReadInode(ParentIno, &ParentInode) == 0) {
        ParentInode.i_links_count++;
        Ext2WriteInode(ParentIno, &ParentInode);
    }

    Print("mkdir: created ");
    Print(Path);
    Print("\n");
    return 0;
}

/*++

Routine Description:

    Removes a file or empty directory at the given path.

Arguments:

    Path - Absolute path to remove.

Return Value:

    0 on success, -1 on error.

--*/

INT
Ext2Rm (
    const char* Path
    )
{
    char        ParentPath[256];
    const char* Name;
    int         I;
    int         LastSlash;
    uint32_t    ParentIno;
    uint32_t    Ino;
    EXT2_INODE  Inode;

    if (!g_Fs.Mounted) {
        Print("ext2: not mounted\n");
        return -1;
    }

    LastSlash = -1;
    for (I = 0; Path[I]; I++) {
        if (Path[I] == '/') {
            LastSlash = I;
        }
    }

    if (LastSlash <= 0) {
        Print("rm: cannot remove root\n");
        return -1;
    }

    for (I = 0; I < LastSlash; I++) {
        ParentPath[I] = Path[I];
    }
    ParentPath[LastSlash] = '\0';
    Name = Path + LastSlash + 1;

    ParentIno = Ext2Lookup(ParentPath);
    if (ParentIno == 0) {
        Print("rm: parent not found\n");
        return -1;
    }

    Ino = Ext2Lookup(Path);
    if (Ino == 0) {
        Print("rm: not found\n");
        return -1;
    }

    if (Ext2ReadInode(Ino, &Inode) != 0) {
        return -1;
    }

    /* Refuse to remove non-empty directories. */
    if (Inode.i_mode & EXT2_S_IFDIR) {
        /* A directory with only . and .. has links_count == 2. */
        if (Inode.i_links_count > 2) {
            Print("rm: directory not empty\n");
            return -1;
        }
    }

    Ext2DirRemoveEntry(ParentIno, Name);
    Ext2FreeInode(Ino);

    Print("rm: removed ");
    Print(Path);
    Print("\n");
    return 0;
}

/*++

Routine Description:

    Writes Data to the file at Path, creating it if necessary and
    truncating any existing content.

Arguments:

    Path   - Absolute path to file.
    Data   - Bytes to write.
    Length - Number of bytes.

Return Value:

    0 on success, -1 on error.

--*/

INT
Ext2Write (
    const char* Path,
    const char* Data,
    uint32_t    Length
    )
{
    char        ParentPath[256];
    const char* Name;
    int         I;
    int         LastSlash;
    uint32_t    ParentIno;
    uint32_t    Ino;
    EXT2_INODE  Inode;
    uint32_t    Written;
    uint32_t    Block;
    uint8_t     Scratch[4096];

    if (!g_Fs.Mounted) {
        Print("ext2: not mounted\n");
        return -1;
    }

    LastSlash = -1;
    for (I = 0; Path[I]; I++) {
        if (Path[I] == '/') {
            LastSlash = I;
        }
    }

    if (LastSlash < 0) {
        Print("write: invalid path\n");
        return -1;
    }

    if (LastSlash == 0) {
        ParentPath[0] = '/';
        ParentPath[1] = '\0';
    } else {
        for (I = 0; I < LastSlash; I++) {
            ParentPath[I] = Path[I];
        }
        ParentPath[LastSlash] = '\0';
    }

    Name = Path + LastSlash + 1;

    ParentIno = Ext2Lookup(ParentPath);
    if (ParentIno == 0) {
        Print("write: parent not found\n");
        return -1;
    }

    Ino = Ext2Lookup(Path);

    if (Ino == 0) {
        /* Create new file. */
        Ino = Ext2AllocInode(0, 0);
        if (Ino == 0) {
            Print("write: no free inodes\n");
            return -1;
        }

        {
            int      J;
            uint8_t* P = (uint8_t*)&Inode;
            for (J = 0; J < (int)sizeof(EXT2_INODE); J++) {
                P[J] = 0;
            }
        }

        Inode.i_mode        = (uint16_t)(EXT2_S_IFREG | 0x1A4); /* 0644 */
        Inode.i_links_count = 1;

        if (Ext2WriteInode(Ino, &Inode) != 0) {
            return -1;
        }

        Ext2DirAddEntry(ParentIno, Name, Ino, EXT2_FT_REG_FILE);
    } else {
        if (Ext2ReadInode(Ino, &Inode) != 0) {
            return -1;
        }
    }

    /* Write data block by block. */
    Written = 0;
    Block   = 0;

    while (Written < Length) {
        uint32_t PhysBlock;
        uint32_t Chunk;
        uint32_t J;

        /* Zero scratch buffer. */
        for (J = 0; J < g_Fs.BlockSize; J++) {
            Scratch[J] = 0;
        }

        Chunk = Length - Written;
        if (Chunk > g_Fs.BlockSize) {
            Chunk = g_Fs.BlockSize;
        }

        for (J = 0; J < Chunk; J++) {
            Scratch[J] = (uint8_t)Data[Written + J];
        }

        PhysBlock = Ext2InodeGetBlock(&Inode, Block);

        if (PhysBlock == 0) {
            PhysBlock = Ext2AllocBlock(0);
            if (PhysBlock == 0) {
                Print("write: disk full\n");
                return -1;
            }

            if (Ext2InodeSetBlock(&Inode, Ino, Block, PhysBlock) != 0) {
                return -1;
            }
        }

        if (Ext2WriteBlock(PhysBlock, Scratch) != 0) {
            return -1;
        }

        Written += Chunk;
        Block++;
    }

    Inode.i_size   = Length;
    Inode.i_blocks = Block * g_Fs.SectorsPerBlock * 2;
    Ext2WriteInode(Ino, &Inode);

    Print("write: ");
    PrintInt((int)Length);
    Print(" bytes written to ");
    Print(Path);
    Print("\n");
    return 0;
}