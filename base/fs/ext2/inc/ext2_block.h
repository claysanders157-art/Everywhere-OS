/*++

Module Name:

    ext2_block.h

Abstract:

    Declarations for ext2 block I/O: reading, writing, allocating,
    and freeing blocks.

Author:

    Noah Juopperi <nipfswd@gmail.com>

--*/

#pragma once

#include "ext2.h"

/*
 * Read/write a single filesystem block into/from a caller-supplied
 * buffer.  Buffer must be at least g_Fs.BlockSize bytes.
 */
INT  Ext2ReadBlock  ( uint32_t BlockNum, void* Buffer );
INT  Ext2WriteBlock ( uint32_t BlockNum, const void* Buffer );

/*
 * Read the block group descriptor for group GroupIdx.
 */
INT  Ext2ReadGroupDesc  ( uint32_t GroupIdx, EXT2_GROUP_DESC* Desc );
INT  Ext2WriteGroupDesc ( uint32_t GroupIdx, const EXT2_GROUP_DESC* Desc );

/*
 * Allocate a free block, returning its block number (0 = failure).
 * PreferGroup hints which block group to search first.
 */
uint32_t Ext2AllocBlock ( uint32_t PreferGroup );

/*
 * Free a previously allocated block.
 */
INT Ext2FreeBlock ( uint32_t BlockNum );