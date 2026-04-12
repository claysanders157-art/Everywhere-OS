/*++

Module Name:

    box.h

Abstract:

    Declarations for the BOX scripting language interpreter.

Author:

    Noah Juopperi <nipfswd@gmail.com>

--*/

#pragma once

#include "types.h"

VOID PrintBoxHelp  ( VOID );
VOID RunBoxScript  ( const char* FileName );