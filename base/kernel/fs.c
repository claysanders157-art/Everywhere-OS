/*++

Module Name:

    fs.c

Abstract:

    This module implements the in-memory note file system.

Author:

    Noah Juopperi <nipfswd@gmail.com>
    Clay Sanders (made first kernel) <claylikepython@yahoo.com>

--*/

#include "inc/fs.h"
#include "inc/string.h"

FILE_ENTRY file_system[MAX_FILES];
int        file_count = 0;

/*++

Routine Description:

    Ensures that a filename ends with ".note".

Arguments:

    Name - Filename buffer.

Return Value:

    None.

--*/

VOID
EnsureNoteExtension (
    char* Name
    )
{
    if (!EndsWith(Name, ".note")) {
        StrCat(Name, ".note");
    }
}

/*++

Routine Description:

    Finds a file by name in the in-memory file system.

Arguments:

    Name - Filename.

Return Value:

    Index of file or -1 if not found.

--*/

int
FindFileIndex (
    const char* Name
    )
{
    int Index;

    for (Index = 0; Index < file_count; Index++) {
        if (file_system[Index].active &&
            StrICmp(file_system[Index].name, Name) == 0) {
            return Index;
        }
    }

    return -1;
}