/*++

Copyright (c) 2026  The EverywhereOS Authors

Module Name:

    explorer.h

Abstract:

    Shell and explorer declarations: desktop, taskbar, shell command
    processor, notes, and snake game.

Author:

    Noah Juopperi <nipfswd@gmail.com>
    Clay Sanders (made the first version of the kernel) <claylikepython@yahoo.com>

Environment:

    User-mode

--*/

#ifndef _EXPLORER_H_
#define _EXPLORER_H_

#include "ke.h"

/* ********** Window instances (defined in base/ntos/ke/window.c) ********** */

extern WINDOW ShellWin;
extern WINDOW NotesWin;
extern WINDOW SnakeWin;

/* ********** Desktop ********** */

void DrawDesktop(void);
void DrawBootScreen(void);

/* ********** Taskbar ********** */

void DrawTaskbar(void);
void HandleTaskbarClick(void);

/* ********** Shell ********** */

#define SHELL_INPUT_SIZE 64

extern char shell_input[];
extern int  shell_len;

void ShellClear(void);
void ShellDraw(void);
void ShellExec(void);

/* ********** Notes ********** */

#define NOTES_BUF_SIZE 256

extern char notes_buf[];
extern int  notes_len;

void NotesDraw(void);

/* ********** Snake ********** */

#define SNAKE_MAX 100

extern int snake_x[];
extern int snake_y[];
extern int snake_len;
extern int snake_dx;
extern int snake_dy;
extern int food_x;
extern int food_y;

void SnakeInit(void);
void SnakeStep(void);
void SnakeDraw(void);

#endif /* _EXPLORER_H_ */
