/*++

Module Name:

    string.c

Abstract:

    This module implements string utility routines.

Author:

    Noah Juopperi <nipfswd@gmail.com>
    Clay Sanders (made first kernel) <claylikepython@yahoo.com>

--*/

#include "inc/string.h"

/*++

Routine Description:

    Converts an ASCII character to lowercase if it is uppercase.

Arguments:

    Character - Character to convert.

Return Value:

    Lowercase character.

--*/

char
ToLowerChar (
    char Character
    )
{
    if (Character >= 'A' && Character <= 'Z') {
        return (char)(Character - 'A' + 'a');
    }

    return Character;
}

/*++

Routine Description:

    Computes the length of a null-terminated string.

Arguments:

    String - Pointer to string.

Return Value:

    Length of string.

--*/

int
StrLen (
    const char* String
    )
{
    int Length;

    Length = 0;
    while (String[Length]) {
        Length++;
    }

    return Length;
}

/*++

Routine Description:

    Copies a null-terminated string.

Arguments:

    Destination - Destination buffer.
    Source      - Source string.

Return Value:

    None.

--*/

VOID
StrCpy (
    char*       Destination,
    const char* Source
    )
{
    while (*Source) {
        *Destination++ = *Source++;
    }

    *Destination = '\0';
}

/*++

Routine Description:

    Appends a null-terminated string to another.

Arguments:

    Destination - Destination buffer.
    Source      - Source string.

Return Value:

    None.

--*/

VOID
StrCat (
    char*       Destination,
    const char* Source
    )
{
    while (*Destination) {
        Destination++;
    }

    while (*Source) {
        *Destination++ = *Source++;
    }

    *Destination = '\0';
}

/*++

Routine Description:

    Compares two strings (case-sensitive).

Arguments:

    String1 - First string.
    String2 - Second string.

Return Value:

    <0 if String1 < String2, 0 if equal, >0 if String1 > String2.

--*/

int
StrCmp (
    const char* String1,
    const char* String2
    )
{
    while (*String1 && (*String1 == *String2)) {
        String1++;
        String2++;
    }

    return (int)(*(const unsigned char*)String1) -
           (int)(*(const unsigned char*)String2);
}

/*++

Routine Description:

    Compares two strings up to a maximum length (case-sensitive).

Arguments:

    String1 - First string.
    String2 - Second string.
    Count   - Maximum characters to compare.

Return Value:

    0 if equal up to Count characters, non-zero otherwise.

--*/

int
StrNCmp (
    const char* String1,
    const char* String2,
    int         Count
    )
{
    while (Count--) {
        if (*String1 != *String2++) {
            return 1;
        }

        if (*String1++ == 0) {
            break;
        }
    }

    return 0;
}

/*++

Routine Description:

    Compares two strings (case-insensitive).

Arguments:

    String1 - First string.
    String2 - Second string.

Return Value:

    <0 if String1 < String2, 0 if equal, >0 if String1 > String2.

--*/

int
StrICmp (
    const char* String1,
    const char* String2
    )
{
    while (*String1 && *String2) {
        char C1;
        char C2;

        C1 = ToLowerChar(*String1);
        C2 = ToLowerChar(*String2);

        if (C1 != C2) {
            return (int)((unsigned char)C1) - (int)((unsigned char)C2);
        }

        String1++;
        String2++;
    }

    return (int)((unsigned char)ToLowerChar(*String1)) -
           (int)((unsigned char)ToLowerChar(*String2));
}

/*++

Routine Description:

    Compares two strings up to a maximum length (case-insensitive).

Arguments:

    String1 - First string.
    String2 - Second string.
    Count   - Maximum characters to compare.

Return Value:

    0 if equal up to Count characters, non-zero otherwise.

--*/

int
StrNICmp (
    const char* String1,
    const char* String2,
    int         Count
    )
{
    while (Count-- && *String1 && *String2) {
        char C1;
        char C2;

        C1 = ToLowerChar(*String1);
        C2 = ToLowerChar(*String2);

        if (C1 != C2) {
            return 1;
        }

        String1++;
        String2++;
    }

    return 0;
}

/*++

Routine Description:

    Tests whether a string ends with a given suffix (case-insensitive).

Arguments:

    String - String to test.
    Suffix - Suffix to check.

Return Value:

    Non-zero if String ends with Suffix, zero otherwise.

--*/

int
EndsWith (
    const char* String,
    const char* Suffix
    )
{
    int LengthString;
    int LengthSuffix;

    LengthString = StrLen(String);
    LengthSuffix = StrLen(Suffix);

    if (LengthSuffix > LengthString) {
        return 0;
    }

    return (StrICmp(String + LengthString - LengthSuffix, Suffix) == 0);
}