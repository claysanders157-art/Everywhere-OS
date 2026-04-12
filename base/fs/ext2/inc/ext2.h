/*++

Module Name:

    ext2.h

Abstract:

    Top-level ext2 filesystem structures: superblock, block group
    descriptor, and filesystem mount state.

Author:

    Noah Juopperi <nipfswd@gmail.com>

References:

    https://www.nongnu.org/ext2-doc/ext2.html

--*/

#pragma once

#include "../../../kernel/inc/types.h"

/*
 * Ext2 magic number (bytes 56-57 of superblock).
 */
#define EXT2_MAGIC          0xEF53

/*
 * Inode numbers for well-known inodes.
 */
#define EXT2_ROOT_INO       2

/*
 * Superblock is always at byte offset 1024 from the start of the
 * partition (LBA-relative).  It occupies one 1024-byte logical block.
 */
#define EXT2_SUPERBLOCK_OFFSET  1024

/*
 * File type flags stored in inode.i_mode.
 */
#define EXT2_S_IFREG        0x8000
#define EXT2_S_IFDIR        0x4000

/*
 * Directory entry file-type byte (d_file_type).
 */
#define EXT2_FT_UNKNOWN     0
#define EXT2_FT_REG_FILE    1
#define EXT2_FT_DIR         2

/*
 * Maximum path component length.
 */
#define EXT2_NAME_LEN       255

#pragma pack(push, 1)

/*
 * On-disk superblock layout (1024 bytes; only used fields are named).
 */
typedef struct _EXT2_SUPERBLOCK {
    uint32_t s_inodes_count;
    uint32_t s_blocks_count;
    uint32_t s_r_blocks_count;
    uint32_t s_free_blocks_count;
    uint32_t s_free_inodes_count;
    uint32_t s_first_data_block;
    uint32_t s_log_block_size;      /* Block size = 1024 << s_log_block_size */
    uint32_t s_log_frag_size;
    uint32_t s_blocks_per_group;
    uint32_t s_frags_per_group;
    uint32_t s_inodes_per_group;
    uint32_t s_mtime;
    uint32_t s_wtime;
    uint16_t s_mnt_count;
    uint16_t s_max_mnt_count;
    uint16_t s_magic;
    uint16_t s_state;
    uint16_t s_errors;
    uint16_t s_minor_rev_level;
    uint32_t s_lastcheck;
    uint32_t s_checkinterval;
    uint32_t s_creator_os;
    uint32_t s_rev_level;
    uint16_t s_def_resuid;
    uint16_t s_def_resgid;
    /* EXT2_DYNAMIC_REV fields */
    uint32_t s_first_ino;
    uint16_t s_inode_size;
    uint16_t s_block_group_nr;
    uint32_t s_feature_compat;
    uint32_t s_feature_incompat;
    uint32_t s_feature_ro_compat;
    uint8_t  s_uuid[16];
    char     s_volume_name[16];
    char     s_last_mounted[64];
    uint32_t s_algo_bitmap;
    /* Padding to 1024 bytes */
    uint8_t  s_pad[820];
} EXT2_SUPERBLOCK;

/*
 * Block group descriptor (32 bytes each, packed into block group
 * descriptor table starting at block 1 when block_size==1024, or
 * block 2 when block_size>1024).
 */
typedef struct _EXT2_GROUP_DESC {
    uint32_t bg_block_bitmap;
    uint32_t bg_inode_bitmap;
    uint32_t bg_inode_table;
    uint16_t bg_free_blocks_count;
    uint16_t bg_free_inodes_count;
    uint16_t bg_used_dirs_count;
    uint16_t bg_pad;
    uint8_t  bg_reserved[12];
} EXT2_GROUP_DESC;

#pragma pack(pop)

/*
 * In-memory filesystem state (one per mounted volume).
 */
typedef struct _EXT2_FS {
    EXT2_SUPERBLOCK Sb;             /* Cached superblock                  */
    uint32_t        BlockSize;      /* Bytes per block                    */
    uint32_t        SectorsPerBlock;/* ATA sectors per block              */
    uint32_t        GroupCount;     /* Number of block groups             */
    uint32_t        PartLba;        /* LBA of partition start (sector 0)  */
    int             Mounted;        /* Non-zero when valid                */
} EXT2_FS;

/*
 * Global filesystem instance (one volume supported).
 */
extern EXT2_FS g_Fs;

/*
 * Mount / unmount.
 */
INT  Ext2Mount   ( uint32_t PartitionLba );
VOID Ext2Unmount ( VOID );

/*
 * High-level path operations (used by shell).
 */
INT  Ext2Ls      ( const char* Path );
INT  Ext2Cat     ( const char* Path );
INT  Ext2Mkdir   ( const char* Path );
INT  Ext2Rm      ( const char* Path );
INT  Ext2Write   ( const char* Path, const char* Data, uint32_t Length );