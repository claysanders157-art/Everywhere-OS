/*++

Module Name:

    ext2_inode.h

Abstract:

    Declarations for ext2 inode operations: reading, writing,
    allocating, freeing, and block-map resolution.

Author:

    Noah Juopperi <nipfswd@gmail.com>

--*/

#pragma once

#include "ext2.h"

#pragma pack(push, 1)

/*
 * On-disk inode (128 bytes for rev 0; dynamic rev uses s_inode_size).
 */
typedef struct _EXT2_INODE {
    uint16_t i_mode;
    uint16_t i_uid;
    uint32_t i_size;
    uint32_t i_atime;
    uint32_t i_ctime;
    uint32_t i_mtime;
    uint32_t i_dtime;
    uint16_t i_gid;
    uint16_t i_links_count;
    uint32_t i_blocks;          /* 512-byte units, NOT filesystem blocks */
    uint32_t i_flags;
    uint32_t i_osd1;
    uint32_t i_block[15];       /* 12 direct + 1 indirect + 1 dbl + 1 tpl */
    uint32_t i_generation;
    uint32_t i_file_acl;
    uint32_t i_dir_acl;
    uint32_t i_faddr;
    uint8_t  i_osd2[12];
} EXT2_INODE;

#pragma pack(pop)

/*
 * Read/write an inode by its 1-based inode number.
 */
INT Ext2ReadInode  ( uint32_t InodeNum, EXT2_INODE* Inode );
INT Ext2WriteInode ( uint32_t InodeNum, const EXT2_INODE* Inode );

/*
 * Resolve the Nth logical filesystem block of an inode to a physical
 * block number (0 = sparse / not allocated).
 */
uint32_t Ext2InodeGetBlock ( const EXT2_INODE* Inode, uint32_t LogicalBlock );

/*
 * Set the Nth logical block of an inode, allocating indirect blocks
 * as needed.
 */
INT Ext2InodeSetBlock ( EXT2_INODE* Inode, uint32_t InodeNum,
                        uint32_t LogicalBlock, uint32_t PhysBlock );

/*
 * Allocate a fresh inode in block group PreferGroup.
 * Returns the new inode number (0 = failure).
 */
uint32_t Ext2AllocInode ( uint32_t PreferGroup, int IsDir );

/*
 * Free an inode and release all its data blocks.
 */
INT Ext2FreeInode ( uint32_t InodeNum );