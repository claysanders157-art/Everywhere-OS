/*++

Module Name:

    ext2_dir.c

Abstract:

    This module implements ext2 directory operations: walking directory
    entries, resolving absolute paths to inode numbers, adding new
    directory entries, and removing existing entries.

Author:

    Noah Juopperi <nipfswd@gmail.com>

--*/

#include "inc/ext2_dir.h"
#include "inc/ext2_block.h"
#include "inc/ext2_inode.h"
#include "../../kernel/inc/string.h"
#include "../../kernel/inc/video.h"

/*++

Routine Description:

    Walks all directory entries of a directory inode, invoking Callback
    for each live entry (inode != 0).

Arguments:

    DirIno   - Inode number of the directory to walk.
    Callback - Function called per entry; return non-zero to stop early.
    Context  - Opaque pointer passed to Callback.

Return Value:

    0 if walk completed, non-zero if Callback stopped it early.

--*/

INT
Ext2DirWalk (
    uint32_t   DirIno,
    EXT2_DIR_CB Callback,
    void*      Context
    )
{
    EXT2_INODE Inode;
    uint32_t   Block;
    uint8_t    Buf[4096];
    uint32_t   Offset;
    EXT2_DIRENT* Entry;
    INT        Result;

    if (Ext2ReadInode(DirIno, &Inode) != 0) {
        return -1;
    }

    for (Block = 0; Block * g_Fs.BlockSize < Inode.i_size; Block++) {
        uint32_t PhysBlock;

        PhysBlock = Ext2InodeGetBlock(&Inode, Block);
        if (PhysBlock == 0) {
            break;
        }

        if (Ext2ReadBlock(PhysBlock, Buf) != 0) {
            break;
        }

        Offset = 0;

        while (Offset < g_Fs.BlockSize) {
            Entry = (EXT2_DIRENT*)(Buf + Offset);

            if (Entry->rec_len == 0) {
                break;
            }

            if (Entry->inode != 0) {
                Result = Callback(Entry, Context);
                if (Result != 0) {
                    return Result;
                }
            }

            Offset += Entry->rec_len;
        }
    }

    return 0;
}

/*
 * Context for the name-lookup callback.
 */
typedef struct _LOOKUP_CTX {
    const char* Name;
    int         NameLen;
    uint32_t    FoundIno;
} LOOKUP_CTX;

/*
 * Callback that compares an entry's name to Name.
 */
static INT
LookupCallback (
    const EXT2_DIRENT* Entry,
    void*              Context
    )
{
    LOOKUP_CTX* Ctx = (LOOKUP_CTX*)Context;

    if ((int)Entry->name_len == Ctx->NameLen &&
        StrNCmp(Entry->name, Ctx->Name, Ctx->NameLen) == 0) {
        Ctx->FoundIno = Entry->inode;
        return 1; /* Stop walking. */
    }

    return 0;
}

/*++

Routine Description:

    Resolves an absolute path to its inode number by walking each
    path component starting from the root inode.

Arguments:

    Path - Null-terminated absolute path (e.g. "/usr/bin/sh").

Return Value:

    Inode number on success, 0 if any component is not found.

--*/

uint32_t
Ext2Lookup (
    const char* Path
    )
{
    uint32_t    CurrentIno;
    const char* P;
    char        Component[EXT2_NAME_LEN + 1];
    int         Len;
    LOOKUP_CTX  Ctx;

    if (Path[0] != '/') {
        return 0;
    }

    CurrentIno = EXT2_ROOT_INO;
    P          = Path + 1;

    while (*P) {
        /* Extract next component. */
        Len = 0;

        while (*P && *P != '/') {
            Component[Len++] = *P++;
        }

        Component[Len] = '\0';

        if (*P == '/') {
            P++;
        }

        if (Len == 0) {
            continue;
        }

        Ctx.Name     = Component;
        Ctx.NameLen  = Len;
        Ctx.FoundIno = 0;

        Ext2DirWalk(CurrentIno, LookupCallback, &Ctx);

        if (Ctx.FoundIno == 0) {
            return 0;
        }

        CurrentIno = Ctx.FoundIno;
    }

    return CurrentIno;
}

/*++

Routine Description:

    Adds a directory entry into the directory inode DirIno.  Searches
    for a gap in existing entries large enough to hold the new entry,
    allocating a new block if necessary.

Arguments:

    DirIno    - Inode number of the directory to modify.
    Name      - Entry name (null-terminated).
    ChildIno  - Target inode number.
    FileType  - EXT2_FT_* constant.

Return Value:

    0 on success, -1 on error.

--*/

INT
Ext2DirAddEntry (
    uint32_t    DirIno,
    const char* Name,
    uint32_t    ChildIno,
    uint8_t     FileType
    )
{
    EXT2_INODE Inode;
    int        NameLen;
    uint32_t   Needed;
    uint32_t   Block;
    uint8_t    Buf[4096];
    uint32_t   Offset;
    EXT2_DIRENT* Entry;
    uint32_t   PhysBlock;

    if (Ext2ReadInode(DirIno, &Inode) != 0) {
        return -1;
    }

    NameLen = StrLen(Name);

    /* rec_len must be 4-byte aligned. */
    Needed = (uint32_t)(8 + NameLen);
    if (Needed % 4) {
        Needed += 4 - (Needed % 4);
    }

    /* Walk existing blocks looking for a gap in an existing entry. */
    for (Block = 0; Block * g_Fs.BlockSize < Inode.i_size; Block++) {
        PhysBlock = Ext2InodeGetBlock(&Inode, Block);
        if (PhysBlock == 0) {
            break;
        }

        if (Ext2ReadBlock(PhysBlock, Buf) != 0) {
            break;
        }

        Offset = 0;

        while (Offset < g_Fs.BlockSize) {
            uint32_t RealLen;
            uint32_t Gap;

            Entry = (EXT2_DIRENT*)(Buf + Offset);

            if (Entry->rec_len == 0) {
                break;
            }

            /* Real space used by this entry. */
            RealLen = 8 + Entry->name_len;
            if (RealLen % 4) {
                RealLen += 4 - (RealLen % 4);
            }

            Gap = Entry->rec_len - RealLen;

            if (Gap >= Needed) {
                EXT2_DIRENT* New;
                int          I;

                /* Shrink current entry and place new one after it. */
                New = (EXT2_DIRENT*)(Buf + Offset + RealLen);

                New->inode     = ChildIno;
                New->rec_len   = (uint16_t)Gap;
                New->name_len  = (uint8_t)NameLen;
                New->file_type = FileType;

                for (I = 0; I < NameLen; I++) {
                    New->name[I] = Name[I];
                }

                Entry->rec_len = (uint16_t)RealLen;

                return Ext2WriteBlock(PhysBlock, Buf);
            }

            Offset += Entry->rec_len;
        }
    }

    /* No gap found — allocate a new block. */
    PhysBlock = Ext2AllocBlock(0);
    if (PhysBlock == 0) {
        return -1;
    }

    {
        uint32_t I;
        int      J;

        for (I = 0; I < g_Fs.BlockSize; I++) {
            Buf[I] = 0;
        }

        Entry              = (EXT2_DIRENT*)Buf;
        Entry->inode       = ChildIno;
        Entry->rec_len     = (uint16_t)g_Fs.BlockSize;
        Entry->name_len    = (uint8_t)NameLen;
        Entry->file_type   = FileType;

        for (J = 0; J < NameLen; J++) {
            Entry->name[J] = Name[J];
        }
    }

    if (Ext2WriteBlock(PhysBlock, Buf) != 0) {
        return -1;
    }

    /* Map the new block into the directory inode. */
    if (Ext2InodeSetBlock(&Inode, DirIno, Block, PhysBlock) != 0) {
        return -1;
    }

    Inode.i_size += g_Fs.BlockSize;
    return Ext2WriteInode(DirIno, &Inode);
}

/*++

Routine Description:

    Removes the first directory entry matching Name from directory DirIno
    by zeroing its inode field and merging its space into the previous
    entry (or leaving the block as a dead entry if it is first).

Arguments:

    DirIno - Inode number of the directory.
    Name   - Entry name to remove.

Return Value:

    0 on success, -1 if not found.

--*/

INT
Ext2DirRemoveEntry (
    uint32_t    DirIno,
    const char* Name
    )
{
    EXT2_INODE   Inode;
    int          NameLen;
    uint32_t     Block;
    uint8_t      Buf[4096];
    uint32_t     Offset;
    EXT2_DIRENT* Entry;
    EXT2_DIRENT* Prev;
    uint32_t     PhysBlock;

    if (Ext2ReadInode(DirIno, &Inode) != 0) {
        return -1;
    }

    NameLen = StrLen(Name);

    for (Block = 0; Block * g_Fs.BlockSize < Inode.i_size; Block++) {
        PhysBlock = Ext2InodeGetBlock(&Inode, Block);
        if (PhysBlock == 0) {
            break;
        }

        if (Ext2ReadBlock(PhysBlock, Buf) != 0) {
            break;
        }

        Offset = 0;
        Prev   = 0;

        while (Offset < g_Fs.BlockSize) {
            Entry = (EXT2_DIRENT*)(Buf + Offset);

            if (Entry->rec_len == 0) {
                break;
            }

            if (Entry->inode != 0 &&
                (int)Entry->name_len == NameLen &&
                StrNCmp(Entry->name, Name, NameLen) == 0) {

                if (Prev) {
                    /* Absorb this entry's space into the previous one. */
                    Prev->rec_len = (uint16_t)(Prev->rec_len + Entry->rec_len);
                } else {
                    /* First entry in block - just zero the inode. */
                    Entry->inode = 0;
                }

                return Ext2WriteBlock(PhysBlock, Buf);
            }

            Prev    = Entry;
            Offset += Entry->rec_len;
        }
    }

    return -1; /* Not found. */
}