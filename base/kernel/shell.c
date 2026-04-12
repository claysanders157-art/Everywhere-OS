/*++

Module Name:

    shell.c

Abstract:

    This module implements keyboard input, line editing, and the
    interactive command shell.

Author:

    Noah Juopperi <nipfswd@gmail.com>
    Clay Sanders (made first kernel) <claylikepython@yahoo.com>

Environment:

    Text-mode VGA, PC keyboard controller.

--*/

#include "inc/shell.h"
#include "inc/video.h"
#include "inc/io.h"
#include "inc/string.h"
#include "inc/fs.h"
#include "inc/snake.h"
#include "inc/box.h"

int  shift_pressed = 0;
char user_name[32] = "User";

/*++

Routine Description:

    Reads a character from the keyboard, handling shift and basic
    control keys.

Arguments:

    None.

Return Value:

    ASCII character, '\n' for Enter, 0x08 for Backspace, 27 for ESC.

--*/

char
GetChar (
    VOID
    )
{
    char Lower[] = {
        0,0,'1','2','3','4','5','6','7','8','9','0','-','=',0,0,
        'q','w','e','r','t','y','u','i','o','p','[',']',0,0,
        'a','s','d','f','g','h','j','k','l',';','\'','`',0,'\\',
        'z','x','c','v','b','n','m',',','.','/',0,'*',0,' '
    };

    char Upper[] = {
        0,0,'!','@','#','$','%','^','&','*','(',')','_','+',0,0,
        'Q','W','E','R','T','Y','U','I','O','P','{','}',0,0,
        'A','S','D','F','G','H','J','K','L',':','\"','~',0,'|',
        'Z','X','C','V','B','N','M','<','>','?',0,'*',0,' '
    };

    for (;;) {
        if (inb(0x64) & 0x01) {
            uint8_t ScanCode;

            ScanCode = inb(0x60);

            if (ScanCode == 0x2A || ScanCode == 0x36) {
                shift_pressed = 1;
                continue;
            }

            if (ScanCode == 0xAA || ScanCode == 0xB6) {
                shift_pressed = 0;
                continue;
            }

            if (ScanCode & 0x80) {
                continue;
            }

            if (ScanCode == 0x0E) {
                return 0x08;
            }

            if (ScanCode == 0x1C) {
                return '\n';
            }

            if (ScanCode == 0x01) {
                return 27;
            }

            return shift_pressed ? Upper[ScanCode] : Lower[ScanCode];
        }
    }
}

/*++

Routine Description:

    Reads a line of input from the keyboard, echoing characters and
    handling backspace.

Arguments:

    Buffer - Buffer to receive null-terminated input string.

Return Value:

    None.

--*/

VOID
GetInput (
    char* Buffer
    )
{
    int Index;

    Index = 0;

    for (;;) {
        char Character;

        Character = GetChar();

        if (Character == '\n') {
            Buffer[Index] = '\0';
            PrintChar('\n', 0x07);
            break;
        } else if (Character == 0x08 && Index > 0) {
            Index--;
            cursor_pos -= 2;
            VIDEO_BUF[cursor_pos] = ' ';
            UpdateCursor();
        } else if (Character != 0x08 && Character != 27) {
            Buffer[Index++] = Character;
            PrintChar(Character, 0x07);
        }
    }
}

/*++

Routine Description:

    Processes a single command line, dispatching to shell commands,
    note management, the snake game, and BOX script execution.

Arguments:

    Command - Command line string.

Return Value:

    None.

--*/

VOID
ProcessCommand (
    char* Command
    )
{
    char CommandLower[128];
    int  Index;

    for (Index = 0;
         Command[Index] && Index < (int)(sizeof(CommandLower) - 1);
         Index++) {
        CommandLower[Index] = ToLowerChar(Command[Index]);
    }

    CommandLower[Index] = '\0';

    if (StrICmp(CommandLower, "clear") == 0) {
        ClearScreen();
        return;
    }

    if (StrNICmp(CommandLower, "show ", 5) == 0) {
        Print(Command + 5);
        Print("\n");
        return;
    }

    if (StrICmp(CommandLower, "snake") == 0) {
        Print("Press Enter to start Snake...\n");
        for (;;) {
            char C;

            C = GetChar();
            if (C == '\n') {
                break;
            }
        }

        SnakeGame();
        return;
    }

    if (StrICmp(CommandLower, "new note") == 0) {
        if (file_count >= MAX_FILES) {
            Print("Storage Full!\n");
            return;
        }

        Print("Filename: ");

        {
            char Name[MAX_FILENAME];
            int  J;

            for (J = 0; J < MAX_FILENAME; J++) {
                Name[J] = 0;
            }

            GetInput(Name);
            EnsureNoteExtension(Name);

            Print("note (ESC to cancel, ` to save):\n");

            {
                int  FileIndex;
                int  ContentIndex;
                char Character;

                FileIndex    = file_count;
                ContentIndex = 0;

                for (;;) {
                    Character = GetChar();

                    if (Character == 27) {
                        Print("\nCanceled.\n");
                        return;
                    }

                    if (Character == '`') {
                        break;
                    }

                    if (Character == 0x08 && ContentIndex > 0) {
                        ContentIndex--;
                        cursor_pos -= 2;
                        VIDEO_BUF[cursor_pos] = ' ';
                        UpdateCursor();
                        continue;
                    }

                    if (Character == '\n') {
                        if (ContentIndex < MAX_CONTENT - 1) {
                            file_system[FileIndex].content[ContentIndex++] =
                                Character;
                            PrintChar(Character, 0x02);
                        }
                        continue;
                    }

                    if (Character != 0x08 &&
                        ContentIndex < MAX_CONTENT - 1) {
                        file_system[FileIndex].content[ContentIndex++] =
                            Character;
                        PrintChar(Character, 0x02);
                    }
                }

                file_system[FileIndex].content[ContentIndex] = '\0';

                for (ContentIndex = 0;
                     ContentIndex < MAX_FILENAME;
                     ContentIndex++) {
                    file_system[FileIndex].name[ContentIndex] = 0;
                }

                for (ContentIndex = 0;
                     Name[ContentIndex] &&
                     ContentIndex < MAX_FILENAME - 1;
                     ContentIndex++) {
                    file_system[FileIndex].name[ContentIndex] =
                        Name[ContentIndex];
                }

                file_system[FileIndex].active = 1;
                file_count++;

                Print("\nSaved!\n");
            }
        }

        return;
    }

    if (StrNICmp(CommandLower, "delete ", 7) == 0) {
        char        FileName[MAX_FILENAME];
        int         J;
        const char* Source;

        J      = 0;
        Source = Command + 7;

        while (*Source && J < MAX_FILENAME - 1) {
            FileName[J++] = *Source++;
        }

        FileName[J] = '\0';
        EnsureNoteExtension(FileName);

        {
            int FileIndex;

            FileIndex = FindFileIndex(FileName);

            if (FileIndex < 0) {
                Print("File not found.\n");
            } else {
                file_system[FileIndex].active = 0;
                Print("Deleted ");
                Print(FileName);
                Print("\n");
            }
        }

        return;
    }

    if (StrICmp(CommandLower, "box help") == 0) {
        PrintBoxHelp();
        return;
    }

    if (StrNICmp(CommandLower, "box ", 4) == 0) {
        char        FileName[MAX_FILENAME];
        int         J;
        const char* Source;

        J      = 0;
        Source = Command + 4;

        while (*Source && J < MAX_FILENAME - 1) {
            FileName[J++] = *Source++;
        }

        FileName[J] = '\0';
        EnsureNoteExtension(FileName);
        RunBoxScript(FileName);
        return;
    }

    if (StrICmp(CommandLower, "setup") == 0) {
        int J;

        Print("Enter your name: ");

        for (J = 0; J < (int)sizeof(user_name); J++) {
            user_name[J] = 0;
        }

        GetInput(user_name);
        Print("Name set.\n");
        return;
    }

    if (StrICmp(CommandLower, "what to do") == 0) {
        Print("Commands:\n");
        Print("  clear         - Clear the screen.\n");
        Print("  show <text>   - Print text.\n");
        Print("  new note      - Create a new .note file.\n");
        Print("  delete <name> - Delete a .note file.\n");
        Print("  box <name>    - Run a BOX script from a .note file.\n");
        Print("  box help      - Show BOX scripting help.\n");
        Print("  snake         - Play the snake game.\n");
        Print("  setup         - Set your user name.\n");
        Print("  what to do    - Show this command list.\n");
        Print("  credits       - Show credits.\n");
        return;
    }

    if (StrICmp(CommandLower, "credits") == 0) {
        Print("Made by Clay Sanders\n");
        return;
    }

    if (Command[0] != '\0') {
        Print("Unknown command.\n");
    }
}