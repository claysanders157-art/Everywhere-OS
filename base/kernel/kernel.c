/*++

Module Name:

    kernel.c

Abstract:

    Main entry point for the OS kernel. Initialises the display and
    enters the interactive shell loop.

Author:

    Noah Juopperi <nipfswd@gmail.com>

Environment:

    Text-mode VGA, PC keyboard controller.

--*/

#include "inc/types.h"
#include "inc/video.h"
#include "inc/shell.h"
#include "../fs/ext2/inc/ext2.h"
#include "../fs/inc/ata.h"
#include "osver.h"

/*
 * The ext2 filesystem starts at sector 0 of the raw disk image
 * (no partition table).
 */
#define EXT2_PARTITION_LBA  0

/*++

Routine Description:

    Main entry point for the OS kernel.

Arguments:

    None.

Return Value:

    None.

--*/

VOID
kernelMain (
    VOID
    )
{
    char Buffer[128];

    ClearScreen();
    Print(OS_NAME " v" OS_VERSION_STRING "\n");

    if (Ext2Mount(EXT2_PARTITION_LBA) != 0) {
        Print("warning: filesystem could not be mounted\n");
    }

    for (;;) {
        Print(user_name);
        Print(": ");
        GetInput(Buffer);
        ProcessCommand(Buffer);
    }
}