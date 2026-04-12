/*++

Module Name:

    video.c

Abstract:

    This module implements VGA text-mode display, cursor management,
    and character/string output routines.

Author:

    Noah Juopperi <nipfswd@gmail.com>
    Clay Sanders (made first kernel) <claylikepython@yahoo.com>

Environment:

    Text-mode VGA (0xB8000), PC hardware cursor via I/O ports.

--*/

#include "inc/video.h"
#include "inc/io.h"

volatile char* VIDEO_BUF = (volatile char*)0xb8000;
int            cursor_pos = 0;

/*++

Routine Description:

    Updates the hardware text-mode cursor position.

Arguments:

    None.

Return Value:

    None.

--*/

VOID
UpdateCursor (
    VOID
    )
{
    uint16_t Position;

    Position = (uint16_t)(cursor_pos / 2);

    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(Position & 0xFF));

    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((Position >> 8) & 0xFF));
}

/*++

Routine Description:

    Prints a single character with an attribute at the current cursor
    position and advances the cursor.

Arguments:

    Character - Character to print.
    Attribute - Attribute byte.

Return Value:

    None.

--*/

VOID
PrintChar (
    char Character,
    char Attribute
    )
{
    if (Character == '\n') {
        cursor_pos = (cursor_pos / 160 + 1) * 160;
    } else {
        VIDEO_BUF[cursor_pos++] = Character;
        VIDEO_BUF[cursor_pos++] = Attribute;
    }

    UpdateCursor();
}

/*++

Routine Description:

    Prints a null-terminated string using the default attribute.

Arguments:

    String - String to print.

Return Value:

    None.

--*/

VOID
Print (
    const char* String
    )
{
    int Index;

    for (Index = 0; String[Index]; Index++) {
        PrintChar(String[Index], 0x07);
    }
}

/*++

Routine Description:

    Prints a decimal integer.

Arguments:

    Number - Integer to print.

Return Value:

    None.

--*/

VOID
PrintInt (
    int Number
    )
{
    char Buffer[10];
    int  Index;

    if (Number == 0) {
        PrintChar('0', 0x07);
        return;
    }

    if (Number < 0) {
        PrintChar('-', 0x07);
        Number = -Number;
    }

    Index = 0;
    while (Number > 0) {
        Buffer[Index++] = (char)((Number % 10) + '0');
        Number /= 10;
    }

    while (--Index >= 0) {
        PrintChar(Buffer[Index], 0x07);
    }
}

/*++

Routine Description:

    Clears the text-mode screen and resets the cursor.

Arguments:

    None.

Return Value:

    None.

--*/

VOID
ClearScreen (
    VOID
    )
{
    int Index;

    for (Index = 0; Index < 4000; Index += 2) {
        VIDEO_BUF[Index]   = ' ';
        VIDEO_BUF[Index+1] = 0x07;
    }

    cursor_pos = 0;
    UpdateCursor();
}