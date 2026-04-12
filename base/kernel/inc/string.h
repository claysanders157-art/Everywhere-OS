/*++

Module Name:

    string.h

Abstract:

    Declarations for string utility routines.

Author:

    Noah Juopperi <nipfswd@gmail.com>

--*/

#pragma once

#include "types.h"

int  StrLen     ( const char* String );
VOID StrCpy     ( char* Destination, const char* Source );
VOID StrCat     ( char* Destination, const char* Source );
int  StrCmp     ( const char* String1, const char* String2 );
int  StrNCmp    ( const char* String1, const char* String2, int Count );
int  StrICmp    ( const char* String1, const char* String2 );
int  StrNICmp   ( const char* String1, const char* String2, int Count );
int  EndsWith   ( const char* String, const char* Suffix );
char ToLowerChar ( char Character );