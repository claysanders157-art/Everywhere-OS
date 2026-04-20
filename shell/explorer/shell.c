/*++

Copyright (c) 2026  The EverywhereOS Authors

Module Name:

    shell.c

Abstract:

    Shell command input and execution (clear, credits, snake, notes).

Author:

    Noah Juopperi <nipfswd@gmail.com>
    Clay Sanders (made the first version of the kernel) <claylikepython@yahoo.com>

Environment:

    Userspace

--*/

#include "explorer.h"

char shell_input[SHELL_INPUT_SIZE];
int  shell_len = 0;

/*++

Routine Description:

    Clears the shell window content area.

Arguments:

    None.

Return Value:

    None.

--*/

void ShellClear(void) {
    FillRect(ShellWin.x + 2, ShellWin.y + 12, ShellWin.w - 4, ShellWin.h - 14, 0x00);
}

/*++

Routine Description:

    Draws the current shell input text inside the shell window.

Arguments:

    None.

Return Value:

    None.

--*/

void ShellDraw(void) {
    if (!ShellWin.visible || ShellWin.minimized) return;
    DrawString(ShellWin.x + 4, ShellWin.y + 14, shell_input, 0x0F);
}

/*++

Routine Description:

    Executes the current shell command. Recognizes "clear", "credits",
    "snake", and "notes".

Arguments:

    None.

Return Value:

    None.

--*/

void ShellExec(void) {
    if (shell_len == 0) return;

    if (StrEq(shell_input, "clear")) {
        ShellClear();
    } else if (StrEq(shell_input, "credits")) {
        DrawString(ShellWin.x + 4, ShellWin.y + 24,
                   "Clay Sanders, Noah Juopperi\n", 0x0F);
    } else if (StrEq(shell_input, "snake")) {
        SnakeWin.visible = 1;
        SnakeWin.minimized = 0;
    } else if (StrEq(shell_input, "notes")) {
        NotesWin.visible = 1;
        NotesWin.minimized = 0;
    }

    shell_len = 0;
    shell_input[0] = 0;
}
