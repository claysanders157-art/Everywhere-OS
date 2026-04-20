/*++

Copyright (c) 2026  The EverywhereOS Authors

Module Name:

    kernel.c
    
Abstract:

    Main entry point for the GUI kernel. All subsystems are now split
    into base\ntos\ke\* (kernel) and shell\explorer\* (userspace).

Author:

    Noah Juopperi <nipfswd@gmail.com>
    Clay Sanders (made the first version of the kernel) <claylikepython@yahoo.com>

Environment:

    Literally the kernel.

--*/

#include "ke.h"
#include "explorer.h"
#include "evryfs.h"

/*++

Routine Description:

    Main entry point for the GUI kernel. Initializes hardware, shows the
    boot screen, then enters the main event loop handling keyboard input,
    mouse interaction, window management, and rendering.

Arguments:

    None.

Return Value:

    None.

--*/

void kernelMain(uint32_t* mbi) {
    SetupFramebuffer(mbi);
    InitFont();
    InitMouse();
    HalInitInterrupts();
    SnakeInit();
    EvryFsInit();

    DrawBootScreen();
    for (volatile int d = 0; d < 2000000; d++) { __asm__ __volatile__("nop"); }

    while (1) {
        last_scancode = 0;
        char ch = GetKeyChar();
        if (ch == 27) {
            RebootSystem();
        }

        /* Route keyboard input to the active window */
        HandleKeyboardInput(ch);

        UpdateMouse();
        SnakeStep();

        HandleWindowMouse(&ShellWin, 0);
        HandleWindowMouse(&NotesWin, 1);
        HandleWindowMouse(&SnakeWin, 2);
        HandleWindowMouse(&FilesWin, 3);
        HandleTaskbarClick();

        UpdateWindowPhysics(&ShellWin);
        UpdateWindowPhysics(&NotesWin);
        UpdateWindowPhysics(&SnakeWin);
        UpdateWindowPhysics(&FilesWin);

        DrawDesktop();
        DrawWindowFrame(&ShellWin);
        DrawWindowFrame(&NotesWin);
        DrawWindowFrame(&SnakeWin);
        DrawWindowFrame(&FilesWin);

        ShellDraw();
        NotesDraw();
        SnakeDraw();
        FilesDraw();

        DrawTaskbar();
        DrawMouseCursor();
        FlipBuffers();

        for (volatile int d = 0; d < 30000; d++) { __asm__ __volatile__("nop"); }
    }
}