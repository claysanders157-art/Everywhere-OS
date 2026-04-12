/*++

Module Name:

    box.c

Abstract:

    This module implements the BOX scripting language interpreter,
    which executes note files as simple command scripts.

Author:

    Noah Juopperi <nipfswd@gmail.com>
    Clay Sanders (made first kernel) <claylikepython@yahoo.com>

--*/

#include "inc/box.h"
#include "inc/video.h"
#include "inc/string.h"
#include "inc/fs.h"
#include "inc/shell.h"

/*++

Routine Description:

    Performs an approximate delay in seconds.
    (Forward declaration - implemented in snake.c)

Arguments:

    Seconds - Number of seconds to delay.

Return Value:

    None.

--*/

extern VOID DelaySeconds ( int Seconds );

/*++

Routine Description:

    Prints help information for the BOX scripting language.

Arguments:

    None.

Return Value:

    None.

--*/

VOID
PrintBoxHelp (
    VOID
    )
{
    Print("BOX Language Help:\n");
    Print("  - Create a note script with: new note\n");
    Print("  - Give it a name; .note will be added if missing.\n");
    Print("  - Each line in the note is a command, same as typing it at the prompt.\n");
    Print("  - Example lines:\n");
    Print("      clear\n");
    Print("      show Hello from BOX!\n");
    Print("      delay 1\n");
    Print("      show This appears 1 second later.\n");
    Print("  - Use: box <filename.note> to run the script.\n");
    Print("  - 'delay N' waits N seconds before continuing.\n");
}

/*++

Routine Description:

    Executes a BOX script stored in a note file, running each line as a
    shell command and handling 'delay N' directives.

Arguments:

    FileName - Name of the note file to run.

Return Value:

    None.

--*/

VOID
RunBoxScript (
    const char* FileName
    )
{
    int FileIndex;

    FileIndex = FindFileIndex(FileName);

    if (FileIndex < 0) {
        Print("File not found.\n");
        return;
    }

    {
        char LineBuffer[64];
        int  LineIndex;
        int  ContentIndex;

        LineIndex = 0;

        for (ContentIndex = 0;
             file_system[FileIndex].content[ContentIndex] != '\0';
             ContentIndex++) {

            char Character;

            Character = file_system[FileIndex].content[ContentIndex];

            if (Character == '\n') {
                LineBuffer[LineIndex] = '\0';

                if (LineIndex > 0) {
                    char LineLower[64];
                    int  T;

                    for (T = 0;
                         T < LineIndex && T < (int)sizeof(LineLower) - 1;
                         T++) {
                        LineLower[T] = ToLowerChar(LineBuffer[T]);
                    }

                    LineLower[T] = '\0';

                    if (StrNICmp(LineLower, "delay ", 6) == 0) {
                        int Seconds;
                        int P;

                        Seconds = 0;
                        P       = 6;

                        while (LineBuffer[P] >= '0' &&
                               LineBuffer[P] <= '9') {
                            Seconds = Seconds * 10 +
                                      (LineBuffer[P] - '0');
                            P++;
                        }

                        if (Seconds > 0) {
                            DelaySeconds(Seconds);
                        }
                    } else {
                        ProcessCommand(LineBuffer);
                    }
                }

                LineIndex = 0;
            } else {
                if (LineIndex < (int)sizeof(LineBuffer) - 1) {
                    LineBuffer[LineIndex++] = Character;
                }
            }
        }

        // Handle final line if not newline-terminated.
        if (LineIndex > 0) {
            char LineLower[64];
            int  T;

            LineBuffer[LineIndex] = '\0';

            for (T = 0;
                 T < LineIndex && T < (int)sizeof(LineLower) - 1;
                 T++) {
                LineLower[T] = ToLowerChar(LineBuffer[T]);
            }

            LineLower[T] = '\0';

            if (StrNICmp(LineLower, "delay ", 6) == 0) {
                int Seconds;
                int P;

                Seconds = 0;
                P       = 6;

                while (LineBuffer[P] >= '0' &&
                       LineBuffer[P] <= '9') {
                    Seconds = Seconds * 10 + (LineBuffer[P] - '0');
                    P++;
                }

                if (Seconds > 0) {
                    DelaySeconds(Seconds);
                }
            } else {
                ProcessCommand(LineBuffer);
            }
        }
    }
}