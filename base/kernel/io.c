/*++

Module Name:

    io.c

Abstract:

    This module implements port I/O primitives and system reboot.

Author:

    Noah Juopperi <nipfswd@gmail.com>

Environment:

    PC keyboard controller, x86 I/O ports.

--*/

#include "inc/io.h"

/*++

Routine Description:

    Writes a byte to an I/O port.

Arguments:

    Port - IO port.
    Data - Byte to write.

Return Value:

    None.

--*/

VOID
outb (
    uint16_t Port,
    uint8_t  Data
    )
{
    __asm__("outb %1, %0" : : "dN" (Port), "a" (Data));
}

/*++

Routine Description:

    Writes a word to an I/O port.

Arguments:

    Port - IO port.
    Data - Word to write.

Return Value:

    None.

--*/

VOID
outw (
    uint16_t Port,
    uint16_t Data
    )
{
    __asm__("outw %1, %0" : : "dN" (Port), "a" (Data));
}

/*++

Routine Description:

    Reads a byte from an IO port.

Arguments:

    Port - IO port.

Return Value:

    Byte read.

--*/

uint8_t
inb (
    uint16_t Port
    )
{
    uint8_t Result;

    __asm__("inb %1, %0" : "=a" (Result) : "Nd" (Port));
    return Result;
}

/*++

Routine Description:

    Reads a word from an IO port.

Arguments:

    Port - IO port.

Return Value:

    Word read.

--*/

uint16_t
inw (
    uint16_t Port
    )
{
    uint16_t Result;

    __asm__("inw %1, %0" : "=a" (Result) : "Nd" (Port));
    return Result;
}

/*++

Routine Description:

    Attempts to reboot the machine via the keyboard controller.

Arguments:

    None.

Return Value:

    None. Does not return on success.

--*/

VOID
RebootSystem (
    VOID
    )
{
    // Wait for input buffer to clear.
    while (inb(0x64) & 0x02) {
    }

    // Pulse reset line.
    outb(0x64, 0xFE);

    // If reboot fails, spin forever.
    for (;;) {
    }
}