/*++

Module Name:

    ext2_block.c

Abstract:

    This module implements ext2 block I/O: reading blocks, writing
    blocks, reading/writing block group descriptors, allocating free
    blocks from the block bitmap, and freeing blocks.

Author:

    Noah Juopperi <nipfswd@gmail.com>

--*/

#include "inc/ext2_block.h"
#include "../../kernel/inc/video.h"
#include "../inc/ata.h"

/*++

Routine Description:

    Converts a filesystem block number to its starting LBA on disk.

Arguments:

    BlockNum - Filesystem block number (1-based in ext2).

Return Value:

    LBA value suitable for AtaReadSectors / AtaWriteSectors.

--*/

static uint32_t
BlockToLba (
    uint32_t BlockNum
    )
{
    return g_Fs.PartLba + BlockNum * g_Fs.SectorsPerBlock;
}

/*++

Routine Description:

    Reads a single filesystem block into caller-supplied memory.

Arguments:

    BlockNum - Block number to read.
    Buffer   - Destination buffer (must be >= g_Fs.BlockSize bytes).

Return Value:

    0 on success, -1 on ATA error.

--*/

INT
Ext2ReadBlock (
    uint32_t BlockNum,
    void*    Buffer
    )
{
    INT Result;

    Result = AtaReadSectors(BlockToLba(BlockNum),
                            (uint8_t)g_Fs.SectorsPerBlock,
                            Buffer);

    if (Result != ATA_OK) {
        Print("ext2: block read error\n");
        return -1;
    }

    return 0;
}

/*++

Routine Description:

    Writes a single filesystem block from caller-supplied memory.

Arguments:

    BlockNum - Block number to write.
    Buffer   - Source buffer (must be >= g_Fs.BlockSize bytes).

Return Value:

    0 on success, -1 on ATA error.

--*/

INT
Ext2WriteBlock (
    uint32_t    BlockNum,
    const void* Buffer
    )
{
    INT Result;

    Result = AtaWriteSectors(BlockToLba(BlockNum),
                             (uint8_t)g_Fs.SectorsPerBlock,
                             Buffer);

    if (Result != ATA_OK) {
        Print("ext2: block write error\n");
        return -1;
    }

    return 0;
}

/*++

Routine Description:

    Reads the block group descriptor for the given group index.

Arguments:

    GroupIdx - Zero-based block group index.
    Desc     - Receives the descriptor.

Return Value:

    0 on success, -1 on error.

--*/

INT
Ext2ReadGroupDesc (
    uint32_t       GroupIdx,
    EXT2_GROUP_DESC* Desc
    )
{
    /*
     * The BGDT starts at block 1 when block_size==1024 (because
     * block 0 is the boot block and block 1 is the superblock).
     * When block_size > 1024, block 0 holds the boot block AND the
     * superblock, so the BGDT starts at block 1 still.
     */
    uint32_t BgdtBlock;
    uint32_t ByteOffset;
    uint32_t BlockOffset;
    uint8_t  Buf[4096]; /* max block size */
    EXT2_GROUP_DESC* Table;

    BgdtBlock  = g_Fs.Sb.s_first_data_block + 1;
    ByteOffset = GroupIdx * sizeof(EXT2_GROUP_DESC);
    BlockOffset = ByteOffset / g_Fs.BlockSize;

    if (Ext2ReadBlock(BgdtBlock + BlockOffset, Buf) != 0) {
        return -1;
    }

    Table = (EXT2_GROUP_DESC*)(Buf + (ByteOffset % g_Fs.BlockSize));
    *Desc = *Table;

    return 0;
}

/*++

Routine Description:

    Writes the block group descriptor for the given group index.

Arguments:

    GroupIdx - Zero-based block group index.
    Desc     - Descriptor to write.

Return Value:

    0 on success, -1 on error.

--*/

INT
Ext2WriteGroupDesc (
    uint32_t             GroupIdx,
    const EXT2_GROUP_DESC* Desc
    )
{
    uint32_t BgdtBlock;
    uint32_t ByteOffset;
    uint32_t BlockOffset;
    uint8_t  Buf[4096];
    EXT2_GROUP_DESC* Table;

    BgdtBlock   = g_Fs.Sb.s_first_data_block + 1;
    ByteOffset  = GroupIdx * sizeof(EXT2_GROUP_DESC);
    BlockOffset = ByteOffset / g_Fs.BlockSize;

    if (Ext2ReadBlock(BgdtBlock + BlockOffset, Buf) != 0) {
        return -1;
    }

    Table  = (EXT2_GROUP_DESC*)(Buf + (ByteOffset % g_Fs.BlockSize));
    *Table = *Desc;

    return Ext2WriteBlock(BgdtBlock + BlockOffset, Buf);
}

/*++

Routine Description:

    Allocates a free block, searching from PreferGroup outward.

Arguments:

    PreferGroup - Preferred block group to allocate from (hint).

Return Value:

    Physical block number on success, 0 on failure (no free blocks).

--*/

uint32_t
Ext2AllocBlock (
    uint32_t PreferGroup
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

        if (Desc.bg_free_blocks_count == 0) {
            continue;
        }

        if (Ext2ReadBlock(Desc.bg_block_bitmap, Bitmap) != 0) {
            continue;
        }

        /* Scan bitmap for first 0 bit. */
        for (I = 0; I < g_Fs.Sb.s_blocks_per_group; I++) {
            Bit = I % 8;
            if ((Bitmap[I / 8] & (1u << Bit)) == 0) {
                uint32_t BlockNum;

                /* Mark as used. */
                Bitmap[I / 8] |= (uint8_t)(1u << Bit);
                Ext2WriteBlock(Desc.bg_block_bitmap, Bitmap);

                Desc.bg_free_blocks_count--;
                Ext2WriteGroupDesc(GroupIdx, &Desc);

                g_Fs.Sb.s_free_blocks_count--;

                BlockNum = GroupIdx * g_Fs.Sb.s_blocks_per_group
                           + g_Fs.Sb.s_first_data_block + I;

                return BlockNum;
            }
        }
    }

    return 0; /* Disk full. */
}

/*++

Routine Description:

    Frees a previously allocated block by clearing its bitmap bit.

Arguments:

    BlockNum - Block number to free.

Return Value:

    0 on success, -1 on error.

--*/

INT
Ext2FreeBlock (
    uint32_t BlockNum
    )
{
    uint32_t       GroupIdx;
    uint32_t       LocalBlock;
    EXT2_GROUP_DESC Desc;
    uint8_t        Bitmap[4096];

    GroupIdx   = (BlockNum - g_Fs.Sb.s_first_data_block)
                 / g_Fs.Sb.s_blocks_per_group;
    LocalBlock = (BlockNum - g_Fs.Sb.s_first_data_block)
                 % g_Fs.Sb.s_blocks_per_group;

    if (Ext2ReadGroupDesc(GroupIdx, &Desc) != 0) {
        return -1;
    }

    if (Ext2ReadBlock(Desc.bg_block_bitmap, Bitmap) != 0) {
        return -1;
    }

    Bitmap[LocalBlock / 8] &= (uint8_t)~(1u << (LocalBlock % 8));
    Ext2WriteBlock(Desc.bg_block_bitmap, Bitmap);

    Desc.bg_free_blocks_count++;
    Ext2WriteGroupDesc(GroupIdx, &Desc);
    g_Fs.Sb.s_free_blocks_count++;

    return 0;
}