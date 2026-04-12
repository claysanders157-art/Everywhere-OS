/*++

Module Name:

    shell.c

Abstract:

    This module implements keyboard input, line editing, and the
    interactive command shell.

Author:

    Noah Juopperi <nipfswd@gmail.com>

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
#include "../fs/ext2/inc/ext2.h"
#include "../fs/ext2/inc/ext2_inode.h"
#include "../fs/ext2/inc/ext2_dir.h"

int  shift_pressed = 0;
char user_name[32] = "User";

/*
 * Current working directory.  Always an absolute path, never a
 * trailing slash except when it is the root itself.
 */
char cwd[256] = "/";

/*++

Routine Description:

    Resolves Arg into an absolute path stored in Out (capacity OutSize).
    If Arg is already absolute it is copied as-is.  Otherwise it is
    appended to the current working directory.

Arguments:

    Arg     - Input path (relative or absolute).
    Out     - Buffer to receive the resolved absolute path.
    OutSize - Size of Out in bytes.

Return Value:

    None.

--*/

static VOID
ResolvePath (
    const char* Arg,
    char*       Out,
    int         OutSize
    )
{
    int I;
    int J;

    if (Arg[0] == '/') {
        /* Already absolute. */
        for (I = 0; Arg[I] && I < OutSize - 1; I++) {
            Out[I] = Arg[I];
        }
        Out[I] = '\0';
        return;
    }

    /* Build cwd + '/' + Arg. */
    I = 0;
    J = 0;

    while (cwd[J] && I < OutSize - 1) {
        Out[I++] = cwd[J++];
    }

    /* Add separator only if cwd is not root. */
    if (I > 1 && Out[I - 1] != '/' && I < OutSize - 1) {
        Out[I++] = '/';
    }

    J = 0;
    while (Arg[J] && I < OutSize - 1) {
        Out[I++] = Arg[J++];
    }

    Out[I] = '\0';
}

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

    if (StrNICmp(CommandLower, "mkdir ", 6) == 0) {
        const char* Arg;
        char        Path[256];

        Arg = Command + 6;
        while (*Arg == ' ') {
            Arg++;
        }

        if (*Arg == '\0') {
            Print("Usage: mkdir <path>\n");
        } else {
            ResolvePath(Arg, Path, 256);
            Ext2Mkdir(Path);
        }

        return;
    }

    if (StrICmp(CommandLower, "ls") == 0 ||
        StrNICmp(CommandLower, "ls ", 3) == 0) {
        const char* Arg;
        char        Path[256];

        Arg = CommandLower + 2;
        while (*Arg == ' ') {
            Arg++;
        }

        if (*Arg == '\0') {
            /* No argument — list cwd. */
            Ext2Ls(cwd);
        } else {
            ResolvePath(Command + (int)(Arg - CommandLower), Path, 256);
            Ext2Ls(Path);
        }

        return;
    }

    if (StrNICmp(CommandLower, "cd ", 3) == 0 ||
        StrICmp(CommandLower, "cd") == 0) {
        const char* Arg;
        char        Path[256];
        uint32_t    Ino;
        EXT2_INODE  Inode;
        int         J;

        Arg = CommandLower + 2;
        while (*Arg == ' ') {
            Arg++;
        }

        if (*Arg == '\0') {
            /* cd with no argument goes to root. */
            cwd[0] = '/';
            cwd[1] = '\0';
            return;
        }

        ResolvePath(Command + (int)(Arg - CommandLower), Path, 256);

        /* Verify the target exists and is a directory. */
        Ino = Ext2Lookup(Path);
        if (Ino == 0) {
            Print("cd: not found: ");
            Print(Path);
            Print("\n");
            return;
        }

        if (Ext2ReadInode(Ino, &Inode) != 0) {
            Print("cd: cannot read inode\n");
            return;
        }

        if ((Inode.i_mode & EXT2_S_IFDIR) == 0) {
            Print("cd: not a directory: ");
            Print(Path);
            Print("\n");
            return;
        }

        /* Commit the new cwd, stripping any trailing slash unless root. */
        J = 0;
        while (Path[J] && J < 254) {
            cwd[J] = Path[J];
            J++;
        }
        cwd[J] = '\0';

        if (J > 1 && cwd[J - 1] == '/') {
            cwd[J - 1] = '\0';
        }

        return;
    }

    if (StrICmp(CommandLower, "pwd") == 0) {
        Print(cwd);
        Print("\n");
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
        Print("  ls [path]     - List directory contents.\n");
        Print("  cd <path>     - Change current directory.\n");
        Print("  pwd           - Print current directory.\n");
        Print("  mkdir <path>  - Create a new directory.\n");
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