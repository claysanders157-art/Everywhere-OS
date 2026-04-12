/*++

Module Name:

    fs.h

Abstract:

    Declarations for the in-memory note file system.

Author:

    Noah Juopperi <nipfswd@gmail.com>

--*/

#pragma once

#include "types.h"

#define MAX_FILES       10
#define MAX_FILENAME    32
#define MAX_CONTENT     512

typedef struct _FILE_ENTRY {
    char name[MAX_FILENAME];
    char content[MAX_CONTENT];
    int  active;
} FILE_ENTRY, *PFILE_ENTRY;

extern FILE_ENTRY file_system[MAX_FILES];
extern int        file_count;

VOID EnsureNoteExtension ( char* Name );
int  FindFileIndex        ( const char* Name );