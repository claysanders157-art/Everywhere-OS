/*++

Module Name:

    ext2_inode.c

Abstract:

    This module implements ext2 inode operations: reading and writing
    inodes by number, resolving logical block numbers through the
    direct/indirect block maps, allocating fresh inodes, and freeing
    inodes together with all their data blocks.

Author:

    Noah Juopperi <nipfswd@gmail.com>

--*/

#include "inc/ext2_inode.h"
#include "inc/ext2_block.h"
#include "../../kernel/inc/video.h"

/*
 * Number of block pointers that fit in one filesystem block.
 * (Computed at runtime from g_Fs.BlockSize.)
 */
#define PTRS_PER_BLOCK(bs) ((bs) / 4u)

/*++

Routine Description:

    Computes the byte offset of inode InodeNum within the inode table
    of its block group, and reads the containing block.

Arguments:

    InodeNum  - 1-based inode number.
    Inode     - Receives the inode on success.

Return Value:

    0 on success, -1 on error.

--*/

INT
Ext2ReadInode (
    uint32_t   InodeNum,
    EXT2_INODE* Inode
    )
{
    uint32_t       GroupIdx;
    uint32_t       LocalIdx;
    EXT2_GROUP_DESC Desc;
    uint32_t       InodeSize;
    uint32_t       InodesPerBlock;
    uint32_t       TableBlock;
    uint32_t       BlockIdx;
    uint32_t       Offset;
    uint8_t        Buf[4096];
    uint8_t*       Src;
    uint8_t*       Dst;
    uint32_t       I;

    if (InodeNum == 0) {
        return -1;
    }

    GroupIdx = (InodeNum - 1) / g_Fs.Sb.s_inodes_per_group;
    LocalIdx = (InodeNum - 1) % g_Fs.Sb.s_inodes_per_group;

    if (Ext2ReadGroupDesc(GroupIdx, &Desc) != 0) {
        return -1;
    }

    InodeSize      = g_Fs.Sb.s_rev_level == 0 ? 128 : g_Fs.Sb.s_inode_size;
    InodesPerBlock = g_Fs.BlockSize / InodeSize;
    BlockIdx       = LocalIdx / InodesPerBlock;
    Offset         = (LocalIdx % InodesPerBlock) * InodeSize;
    TableBlock     = Desc.bg_inode_table + BlockIdx;

    if (Ext2ReadBlock(TableBlock, Buf) != 0) {
        return -1;
    }

    Src = Buf + Offset;
    Dst = (uint8_t*)Inode;

    for (I = 0; I < sizeof(EXT2_INODE); I++) {
        Dst[I] = Src[I];
    }

    return 0;
}

/*++

Routine Description:

    Writes an inode back to its position in the inode table.

Arguments:

    InodeNum - 1-based inode number.
    Inode    - Inode data to write.

Return Value:

    0 on success, -1 on error.

--*/

INT
Ext2WriteInode (
    uint32_t         InodeNum,
    const EXT2_INODE* Inode
    )
{
    uint32_t       GroupIdx;
    uint32_t       LocalIdx;
    EXT2_GROUP_DESC Desc;
    uint32_t       InodeSize;
    uint32_t       InodesPerBlock;
    uint32_t       TableBlock;
    uint32_t       BlockIdx;
    uint32_t       Offset;
    uint8_t        Buf[4096];
    uint8_t*       Dst;
    const uint8_t* Src;
    uint32_t       I;

    if (InodeNum == 0) {
        return -1;
    }

    GroupIdx = (InodeNum - 1) / g_Fs.Sb.s_inodes_per_group;
    LocalIdx = (InodeNum - 1) % g_Fs.Sb.s_inodes_per_group;

    if (Ext2ReadGroupDesc(GroupIdx, &Desc) != 0) {
        return -1;
    }

    InodeSize      = g_Fs.Sb.s_rev_level == 0 ? 128 : g_Fs.Sb.s_inode_size;
    InodesPerBlock = g_Fs.BlockSize / InodeSize;
    BlockIdx       = LocalIdx / InodesPerBlock;
    Offset         = (LocalIdx % InodesPerBlock) * InodeSize;
    TableBlock     = Desc.bg_inode_table + BlockIdx;

    if (Ext2ReadBlock(TableBlock, Buf) != 0) {
        return -1;
    }

    Dst = Buf + Offset;
    Src = (const uint8_t*)Inode;

    for (I = 0; I < sizeof(EXT2_INODE); I++) {
        Dst[I] = Src[I];
    }

    return Ext2WriteBlock(TableBlock, Buf);
}

/*++

Routine Description:

    Resolves the Nth logical block of an inode to its physical block
    number, following single/double/triple indirect maps as needed.

Arguments:

    Inode        - Pointer to the inode.
    LogicalBlock - Zero-based logical block index within the file.

Return Value:

    Physical block number, or 0 if the block is not allocated.

--*/

uint32_t
Ext2InodeGetBlock (
    const EXT2_INODE* Inode,
    uint32_t          LogicalBlock
    )
{
    uint32_t PtrsPerBlock;
    uint32_t Buf[1024]; /* max 4096/4 pointers */

    PtrsPerBlock = PTRS_PER_BLOCK(g_Fs.BlockSize);

    /* Direct blocks 0-11. */
    if (LogicalBlock < 12) {
        return Inode->i_block[LogicalBlock];
    }

    LogicalBlock -= 12;

    /* Single indirect (block 12). */
    if (LogicalBlock < PtrsPerBlock) {
        if (Inode->i_block[12] == 0) {
            return 0;
        }

        Ext2ReadBlock(Inode->i_block[12], Buf);
        return Buf[LogicalBlock];
    }

    LogicalBlock -= PtrsPerBlock;

    /* Double indirect (block 13). */
    if (LogicalBlock < PtrsPerBlock * PtrsPerBlock) {
        uint32_t Buf2[1024];
        uint32_t L1;
        uint32_t L2;

        if (Inode->i_block[13] == 0) {
            return 0;
        }

        L1 = LogicalBlock / PtrsPerBlock;
        L2 = LogicalBlock % PtrsPerBlock;

        Ext2ReadBlock(Inode->i_block[13], Buf);
        if (Buf[L1] == 0) {
            return 0;
        }

        Ext2ReadBlock(Buf[L1], Buf2);
        return Buf2[L2];
    }

    /* Triple indirect — not common for small kernels; return 0. */
    return 0;
}

/*++

Routine Description:

    Maps logical block LogicalBlock of an inode to PhysBlock, allocating
    any required indirect blocks along the way.

Arguments:

    Inode        - Inode to update (will be modified and written).
    InodeNum     - Inode number (needed to write back).
    LogicalBlock - Zero-based logical block index.
    PhysBlock    - Physical block number to set.

Return Value:

    0 on success, -1 on error.

--*/

INT
Ext2InodeSetBlock (
    EXT2_INODE* Inode,
    uint32_t    InodeNum,
    uint32_t    LogicalBlock,
    uint32_t    PhysBlock
    )
{
    uint32_t PtrsPerBlock;
    uint32_t Buf[1024];

    PtrsPerBlock = PTRS_PER_BLOCK(g_Fs.BlockSize);

    /* Direct. */
    if (LogicalBlock < 12) {
        Inode->i_block[LogicalBlock] = PhysBlock;
        return Ext2WriteInode(InodeNum, Inode);
    }

    LogicalBlock -= 12;

    /* Single indirect. */
    if (LogicalBlock < PtrsPerBlock) {
        if (Inode->i_block[12] == 0) {
            uint32_t IndirBlock;
            uint32_t I;

            IndirBlock = Ext2AllocBlock(0);
            if (IndirBlock == 0) {
                return -1;
            }

            for (I = 0; I < PtrsPerBlock; I++) {
                Buf[I] = 0;
            }

            Ext2WriteBlock(IndirBlock, Buf);
            Inode->i_block[12] = IndirBlock;
            Ext2WriteInode(InodeNum, Inode);
        }

        Ext2ReadBlock(Inode->i_block[12], Buf);
        Buf[LogicalBlock] = PhysBlock;
        return Ext2WriteBlock(Inode->i_block[12], Buf);
    }

    /* Double indirect — allocate as needed. */
    LogicalBlock -= PtrsPerBlock;

    if (LogicalBlock < PtrsPerBlock * PtrsPerBlock) {
        uint32_t Buf2[1024];
        uint32_t L1;
        uint32_t L2;
        uint32_t I;

        L1 = LogicalBlock / PtrsPerBlock;
        L2 = LogicalBlock % PtrsPerBlock;

        if (Inode->i_block[13] == 0) {
            uint32_t IndirBlock;

            IndirBlock = Ext2AllocBlock(0);
            if (IndirBlock == 0) {
                return -1;
            }

            for (I = 0; I < PtrsPerBlock; I++) {
                Buf[I] = 0;
            }

            Ext2WriteBlock(IndirBlock, Buf);
            Inode->i_block[13] = IndirBlock;
            Ext2WriteInode(InodeNum, Inode);
        }

        Ext2ReadBlock(Inode->i_block[13], Buf);

        if (Buf[L1] == 0) {
            uint32_t IndirBlock;

            IndirBlock = Ext2AllocBlock(0);
            if (IndirBlock == 0) {
                return -1;
            }

            for (I = 0; I < PtrsPerBlock; I++) {
                Buf2[I] = 0;
            }

            Ext2WriteBlock(IndirBlock, Buf2);
            Buf[L1] = IndirBlock;
            Ext2WriteBlock(Inode->i_block[13], Buf);
        }

        Ext2ReadBlock(Buf[L1], Buf2);
        Buf2[L2] = PhysBlock;
        return Ext2WriteBlock(Buf[L1], Buf2);
    }

    /* Triple indirect not supported. */
    return -1;
}

/*++

Routine Description:

    Allocates a free inode from the inode bitmap, preferring PreferGroup.

Arguments:

    PreferGroup - Preferred block group (hint).
    IsDir       - Non-zero if the new inode is for a directory.

Return Value:

    New 1-based inode number, or 0 if no free inodes.

--*/

uint32_t
Ext2AllocInode (
    uint32_t PreferGroup,
    int      IsDir
    )
{
    uint32_t       G;
    EXT2_GROUP_DESC Desc;
    uint8_t        Bitmap[4096];
    uint32_t       I;
    uint32_t       Bit;

    for (G = 0; G < g_Fs.GroupCount; G++) {
        uint32_t GroupIdx;

        GroupIdx = (PreferGroup + G) % g_Fs.GroupCount;

        if (Ext2ReadGroupDesc(GroupIdx, &Desc) != 0) {
            continue;
        }

        if (Desc.bg_free_inodes_count == 0) {
            continue;
        }

        if (Ext2ReadBlock(Desc.bg_inode_bitmap, Bitmap) != 0) {
            continue;
        }

        for (I = 0; I < g_Fs.Sb.s_inodes_per_group; I++) {
            Bit = I % 8;

            if ((Bitmap[I / 8] & (1u << Bit)) == 0) {
                uint32_t InodeNum;

                Bitmap[I / 8] |= (uint8_t)(1u << Bit);
                Ext2WriteBlock(Desc.bg_inode_bitmap, Bitmap);

                Desc.bg_free_inodes_count--;

                if (IsDir) {
                    Desc.bg_used_dirs_count++;
                }

                Ext2WriteGroupDesc(GroupIdx, &Desc);
                g_Fs.Sb.s_free_inodes_count--;

                InodeNum = GroupIdx * g_Fs.Sb.s_inodes_per_group + I + 1;
                return InodeNum;
            }
        }
    }

    return 0;
}

/*++

Routine Description:

    Frees an inode and releases all its data blocks (direct and single
    indirect; double indirect blocks also released).

Arguments:

    InodeNum - 1-based inode number to free.

Return Value:

    0 on success, -1 on error.

--*/

INT
Ext2FreeInode (
    uint32_t InodeNum
    )
{
    EXT2_INODE      Inode;
    uint32_t        GroupIdx;
    uint32_t        LocalIdx;
    EXT2_GROUP_DESC  Desc;
    uint8_t         Bitmap[4096];
    uint32_t        I;
    uint32_t        PtrsPerBlock;
    uint32_t        Buf[1024];

    if (Ext2ReadInode(InodeNum, &Inode) != 0) {
        return -1;
    }

    PtrsPerBlock = PTRS_PER_BLOCK(g_Fs.BlockSize);

    /* Free direct blocks. */
    for (I = 0; I < 12; I++) {
        if (Inode.i_block[I]) {
            Ext2FreeBlock(Inode.i_block[I]);
        }
    }

    /* Free single indirect. */
    if (Inode.i_block[12]) {
        Ext2ReadBlock(Inode.i_block[12], Buf);

        for (I = 0; I < PtrsPerBlock; I++) {
            if (Buf[I]) {
                Ext2FreeBlock(Buf[I]);
            }
        }

        Ext2FreeBlock(Inode.i_block[12]);
    }

    /* Free double indirect. */
    if (Inode.i_block[13]) {
        uint32_t Buf2[1024];
        uint32_t J;

        Ext2ReadBlock(Inode.i_block[13], Buf);

        for (I = 0; I < PtrsPerBlock; I++) {
            if (Buf[I]) {
                Ext2ReadBlock(Buf[I], Buf2);

                for (J = 0; J < PtrsPerBlock; J++) {
                    if (Buf2[J]) {
                        Ext2FreeBlock(Buf2[J]);
                    }
                }

                Ext2FreeBlock(Buf[I]);
            }
        }

        Ext2FreeBlock(Inode.i_block[13]);
    }

    /* Clear the inode bitmap bit. */
    GroupIdx = (InodeNum - 1) / g_Fs.Sb.s_inodes_per_group;
    LocalIdx = (InodeNum - 1) % g_Fs.Sb.s_inodes_per_group;

    if (Ext2ReadGroupDesc(GroupIdx, &Desc) != 0) {
        return -1;
    }

    if (Ext2ReadBlock(Desc.bg_inode_bitmap, Bitmap) != 0) {
        return -1;
    }

    Bitmap[LocalIdx / 8] &= (uint8_t)~(1u << (LocalIdx % 8));
    Ext2WriteBlock(Desc.bg_inode_bitmap, Bitmap);

    if (Inode.i_mode & EXT2_S_IFDIR) {
        Desc.bg_used_dirs_count--;
    }

    Desc.bg_free_inodes_count++;
    Ext2WriteGroupDesc(GroupIdx, &Desc);
    g_Fs.Sb.s_free_inodes_count++;

    return 0;
}