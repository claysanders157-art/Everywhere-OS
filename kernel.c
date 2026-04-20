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

void kernelMain(void) {
    SetMode13h();
    InitFont();
    InitMouse();
    SnakeInit();

    DrawBootScreen();
    for (volatile int d = 0; d < 2000000; d++) { __asm__ __volatile__("nop"); }

    while (1) {
        last_scancode = 0;
        char ch = GetKeyChar();
        if (ch == 27) {
            RebootSystem();
        }

        /* Snake arrow key controls (scancodes: up=0x48, down=0x50, left=0x4B, right=0x4D) */
        if (active_window == 2 && SnakeWin.visible && !SnakeWin.minimized) {
            if (last_scancode == 0x48 && snake_dy != 1)  { snake_dx = 0; snake_dy = -1; }
            if (last_scancode == 0x50 && snake_dy != -1) { snake_dx = 0; snake_dy = 1;  }
            if (last_scancode == 0x4B && snake_dx != 1)  { snake_dx = -1; snake_dy = 0; }
            if (last_scancode == 0x4D && snake_dx != -1) { snake_dx = 1;  snake_dy = 0; }
        }

        /* Keyboard input routed by active window */
        if (ch) {
            if (active_window == 0) {
                /* Shell input */
                if (ch == '\n') {
                    ShellExec();
                } else if (ch == 0x08) {
                    if (shell_len > 0) shell_input[--shell_len] = 0;
                } else if (shell_len < SHELL_INPUT_SIZE - 1) {
                    shell_input[shell_len++] = ch;
                    shell_input[shell_len] = 0;
                }
            } else if (active_window == 1) {
                /* Notes input */
                if (ch == 0x08) {
                    if (notes_len > 0) notes_buf[--notes_len] = 0;
                } else if (notes_len < NOTES_BUF_SIZE - 1) {
                    notes_buf[notes_len++] = ch;
                    notes_buf[notes_len] = 0;
                }
            }
        }

        UpdateMouse();
        SnakeStep();

        HandleWindowMouse(&ShellWin, 0);
        HandleWindowMouse(&NotesWin, 1);
        HandleWindowMouse(&SnakeWin, 2);
        HandleTaskbarClick();

        UpdateWindowPhysics(&ShellWin);
        UpdateWindowPhysics(&NotesWin);
        UpdateWindowPhysics(&SnakeWin);

        DrawDesktop();
        DrawWindowFrame(&ShellWin);
        DrawWindowFrame(&NotesWin);
        DrawWindowFrame(&SnakeWin);

        ShellDraw();
        NotesDraw();
        SnakeDraw();

        DrawTaskbar();
        DrawMouseCursor();
        FlipBuffers();

        for (volatile int d = 0; d < 30000; d++) { __asm__ __volatile__("nop"); }
    }
}