/*++

Module Name:

    ext2_dir.h

Abstract:

    Declarations for ext2 directory operations: path lookup,
    entry addition, and entry removal.

Author:

    Noah Juopperi <nipfswd@gmail.com>

--*/

#pragma once

#include "ext2.h"
#include "ext2_inode.h"

#pragma pack(push, 1)

/*
 * On-disk directory entry (variable length, 4-byte aligned rec_len).
 */
typedef struct _EXT2_DIRENT {
    uint32_t inode;
    uint16_t rec_len;
    uint8_t  name_len;
    uint8_t  file_type;
    char     name[EXT2_NAME_LEN];
} EXT2_DIRENT;

#pragma pack(pop)

/*
 * Walk the directory entries of a directory inode, calling Callback
 * for each live entry.  Stops early if Callback returns non-zero.
 */
typedef INT (*EXT2_DIR_CB)(const EXT2_DIRENT* Entry, void* Context);
INT Ext2DirWalk ( uint32_t DirIno, EXT2_DIR_CB Callback, void* Context );

/*
 * Resolve a path to an inode number.  Path must start with '/'.
 * Returns 0 if not found.
 */
uint32_t Ext2Lookup ( const char* Path );

/*
 * Add a directory entry (Name -> ChildIno) inside directory DirIno.
 */
INT Ext2DirAddEntry ( uint32_t DirIno, const char* Name,
                      uint32_t ChildIno, uint8_t FileType );

/*
 * Remove the entry Name from directory DirIno (marks inode 0).
 */
INT Ext2DirRemoveEntry ( uint32_t DirIno, const char* Name );