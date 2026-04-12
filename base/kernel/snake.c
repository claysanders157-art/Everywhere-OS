/*++

Module Name:

    snake.c

Abstract:

    This module implements the text-mode snake game.

Author:

    Noah Juopperi <nipfswd@gmail.com>
    Clay Sanders (made first kernel) <claylikepython@yahoo.com>

Environment:

    Text-mode VGA, PC keyboard controller.

--*/

#include "inc/snake.h"
#include "inc/video.h"
#include "inc/io.h"

int high_score = 0;

/*++

Routine Description:

    Performs a busy-wait delay.

Arguments:

    Ticks - Approximate delay factor.

Return Value:

    None.

--*/

static VOID
DelayTicks (
    int Ticks
    )
{
    volatile int Index;

    for (Index = 0; Index < Ticks; Index++) {
    }
}

/*++

Routine Description:

    Performs an approximate delay in seconds.

Arguments:

    Seconds - Number of seconds to delay.

Return Value:

    None.

--*/

VOID
DelaySeconds (
    int Seconds
    )
{
    int Index;

    for (Index = 0; Index < Seconds; Index++) {
        DelayTicks(12000000);
    }
}

/*++

Routine Description:

    Implements a simple snake game. Displays score, moves the snake,
    updates the high score, and reboots on exit.

Arguments:

    None.

Return Value:

    None.

--*/

VOID
SnakeGame (
    VOID
    )
{
    int SnakeX[MAX_SNAKE];
    int SnakeY[MAX_SNAKE];
    int Length;
    int FoodX;
    int FoodY;
    int DeltaX;
    int DeltaY;
    int Score;
    int Index;

    Length = 1;
    FoodX  = 20;
    FoodY  = 10;
    DeltaX = 1;
    DeltaY = 0;
    Score  = 0;

    SnakeX[0] = 40;
    SnakeY[0] = 12;

    ClearScreen();

    for (;;) {
        int OldCursor;

        OldCursor  = cursor_pos;
        cursor_pos = 0;

        Print("Score: ");
        PrintInt(Score);
        Print(" | High Score: ");
        PrintInt(high_score);
        Print(" | WASD to Move, Q to Quit");

        cursor_pos = OldCursor;
        UpdateCursor();

        VIDEO_BUF[(FoodY * 80 + FoodX) * 2]     = '@';
        VIDEO_BUF[(FoodY * 80 + FoodX) * 2 + 1] = 0x04;

        for (Index = 0; Index < Length; Index++) {
            VIDEO_BUF[(SnakeY[Index] * 80 + SnakeX[Index]) * 2]     =
                (Index == 0) ? 'O' : '*';
            VIDEO_BUF[(SnakeY[Index] * 80 + SnakeX[Index]) * 2 + 1] = 0x02;
        }

        DelayTicks(6000000);

        if (inb(0x64) & 0x01) {
            unsigned char ScanCode;

            ScanCode = inb(0x60);

            if (ScanCode == 0x11 && DeltaY != 1) {
                DeltaX = 0;
                DeltaY = -1;
            }

            if (ScanCode == 0x1E && DeltaX != 1) {
                DeltaX = -1;
                DeltaY = 0;
            }

            if (ScanCode == 0x1F && DeltaY != -1) {
                DeltaX = 0;
                DeltaY = 1;
            }

            if (ScanCode == 0x20 && DeltaX != -1) {
                DeltaX = 1;
                DeltaY = 0;
            }

            if (ScanCode == 0x10) {
                break;
            }
        }

        VIDEO_BUF[(SnakeY[Length-1] * 80 + SnakeX[Length-1]) * 2] = ' ';

        for (Index = Length - 1; Index > 0; Index--) {
            SnakeX[Index] = SnakeX[Index-1];
            SnakeY[Index] = SnakeY[Index-1];
        }

        SnakeX[0] += DeltaX;
        SnakeY[0] += DeltaY;

        if (SnakeX[0] < 0  || SnakeX[0] >= 80 ||
            SnakeY[0] < 1  || SnakeY[0] >= 25) {
            break;
        }

        if (SnakeX[0] == FoodX && SnakeY[0] == FoodY) {
            Score++;

            if (Length < MAX_SNAKE) {
                Length++;
            }

            if (Score > high_score) {
                high_score = Score;
            }

            FoodX = (FoodX * 3 + 7) % 70 + 5;
            FoodY = (FoodY * 7 + 3) % 20 + 2;
        }
    }

    ClearScreen();
    Print("Game Over!\n");
    Print("Score: ");
    PrintInt(Score);
    Print("\nHigh Score: ");
    PrintInt(high_score);
    Print("\nPress any key to reboot...");
    (void)inb(0x60);
    RebootSystem();
}