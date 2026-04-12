/*++

Module Name:

    video.h

Abstract:

    Declarations for VGA text-mode display and cursor routines.

Author:

    Noah Juopperi <nipfswd@gmail.com>

--*/

#pragma once

#include "types.h"

extern volatile char* VIDEO_BUF;
extern int            cursor_pos;

VOID UpdateCursor ( VOID );
VOID PrintChar    ( char Character, char Attribute );
VOID Print        ( const char* String );
VOID PrintInt     ( int Number );
VOID ClearScreen  ( VOID );