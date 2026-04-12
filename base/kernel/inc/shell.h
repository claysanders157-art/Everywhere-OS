/*++

Module Name:

    shell.h

Abstract:

    Declarations for the interactive shell and command processor.

Author:

    Noah Juopperi <nipfswd@gmail.com>

--*/

#pragma once

#include "types.h"

extern char user_name[32];
extern int  shift_pressed;

char GetChar  ( VOID );
VOID GetInput ( char* Buffer );
VOID ProcessCommand ( char* Command );