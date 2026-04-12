/*++

Module Name:

    kernel.c

Abstract:

    Main entry point for the OS kernel. Initialises the display and
    enters the interactive shell loop.

Author:

    Noah Juopperi <nipfswd@gmail.com>
    Clay Sanders (made first kernel) <claylikepython@yahoo.com>

Environment:

    Text-mode VGA, PC keyboard controller.

--*/

#include "inc/types.h"
#include "inc/video.h"
#include "inc/shell.h"
#include "osver.h"

/*++

Routine Description:

    Main entry point for the OS shell.

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

    for (;;) {
        Print(user_name);
        Print(": ");
        GetInput(Buffer);
        ProcessCommand(Buffer);
    }
}