/*++

Module Name:

    kernel.c
    
Abstract:

    This module implements a simple text-mode shell with a note system,
    a snake game, and a small BOX scripting language.

Author:

    Noah Juopperi <nipfswd@gmail.com>
    Clay Sanders (made first kernel) <claylikepython@yahoo.com>
Environment:

    Text-mode VGA, PC keyboard controller.

NOTES:

    Perhaps we'll migrate this into multiple files.
    
--*/

#include <stdint.h>
// version and OS name header in public/sdk/inc/
#include "osver.h"

// define Windows ish style types for compatibility
typedef void VOID;
typedef unsigned char UCHAR;
typedef int INT;

volatile char* VIDEO_BUF = (volatile char*)0xb8000;

#define MAX_FILES       10
#define MAX_FILENAME    32
#define MAX_CONTENT     512
#define MAX_SNAKE       100

typedef struct _FILE_ENTRY {
    char name[MAX_FILENAME];
    char content[MAX_CONTENT];
    int  active;
} FILE_ENTRY, *PFILE_ENTRY;

FILE_ENTRY file_system[MAX_FILES];
int        file_count    = 0;
int        cursor_pos    = 0;
int        shift_pressed = 0;
int        high_score    = 0;
char       user_name[32] = "User";

/*++

Routine Description:

    Writes a byte to an I/O port.
    
Arguments:

    Port - IO port.
    Data - Byte to write
    
Return Value:

    None
    
--*/

VOID
outb (
    uint16_t Port,
    uint8_t  Data
    )
{
    __asm__("outb %1, %0" : : "dN" (Port), "a" (Data));
}

/*++

Routine Description:

    Reads a byte from an IO port.
    
Arguments:

    Port - IO port.
    
Return Value:

    Byte read.
    
--*/

uint8_t
inb (
    uint16_t Port
    )
{
    uint8_t Result;

    __asm__("inb %1, %0" : "=a" (Result) : "Nd" (Port));
    return Result;
}

/*++

Routine Description:

    Attempts to reboot the machine via the keyboard controller.

Arguments:

    None.

Return Value:

    None. Does not return on success.

--*/

VOID
RebootSystem (
    VOID
    )
{
    //Wait for input buffer to clear
    while (inb(0x64) & 0x02) {
    }

    //pulse reset line.
    outb(0x64, 0xFE);

    //if reboot fails, spin 4ever !.
    for (;;) {
    }
}

/*++

Routine Description:

    Updates the hardware text-mode cursor position.

Arguments:

    None.

Return Value:

    None.

--*/

VOID
UpdateCursor (
    VOID
    )
{
    uint16_t Position;

    Position = (uint16_t)(cursor_pos / 2);

    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(Position & 0xFF));

    outb (0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((Position >> 8) & 0xFF));
}

/*++

Routine Description:

    Computes the length of a null-terminated string.

Arguments:

    String - Pointer to string.

Return Value:

    Length of string.

--*/

int
StrLen (
    const char* String
    )
{
    int Length;

    Length = 0;
    while (String[Length]) {
        Length++;
    }

    return Length;
}

/*++

Routine Description:

    Copies a null-terminated string.

Arguments:

    Destination - Destination buffer.
    Source      - Source string.

Return Value:

    None.

--*/

VOID
StrCpy (
    char*       Destination,
    const char* Source
    )
{
    while (*Source) {
        *Destination++ = *Source++;
    }

    *Destination = '\0';
}

/*++

Routine Description:

    Appends a null-terminated string to another.

Arguments:

    Destination - Destination buffer.
    Source      - Source string.

Return Value:

    None.

--*/

VOID
StrCat (
    char*       Destination,
    const char* Source
    )
{
    while (*Destination) {
        Destination++;
    }

    while (*Source) {
        *Destination++ = *Source++;
    }

    *Destination = '\0';
}

/*++

Routine Description:

    Compares two strings (case-sensitive).

Arguments:

    String1 - First string.
    String2 - Second string.

Return Value:

    <0 if String1 < String2, 0 if equal, >0 if String1 > String2.

--*/

int
StrCmp (
    const char* String1,
    const char* String2
    )
{
    while (*String1 && (*String1 == *String2)) {
        String1++;
        String2++;
    }

    return (int)(*(const unsigned char*)String1) -
           (int)(*(const unsigned char*)String2);
}

/*++

Routine Description:

    Compares two strings up to a maximum length (case-sensitive).

Arguments:

    String1 - First string.
    String2 - Second string.
    Count   - Maximum characters to compare.

Return Value:

    0 if equal up to Count characters, non-zero otherwise.

--*/

int
StrNCmp (
    const char* String1,
    const char* String2,
    int         Count
    )
{
    while (Count--) {
        if (*String1 != *String2++) {
            return 1;
        }

        if (*String1++ == 0) {
            break;
        }
    }

    return 0;
}

/*++

Routine Description:

    Converts an ASCII character to lowercase if it is uppercase.

Arguments:

    Character - Character to convert.

Return Value:

    Lowercase character.

--*/

char
ToLowerChar (
    char Character
    )
{
    if (Character >= 'A' && Character <= 'Z') {
        return (char)(Character - 'A' + 'a');
    }

    return Character;
}

/*++

Routine Description:

    Compares two strings (case-insensitive).

Arguments:

    String1 - First string.
    String2 - Second string.

Return Value:

    <0 if String1 < String2, 0 if equal, >0 if String1 > String2.

--*/

int
StrICmp (
    const char* String1,
    const char* String2
    )
{
    while (*String1 && *String2) {
        char C1;
        char C2;

        C1 = ToLowerChar(*String1);
        C2 = ToLowerChar(*String2);

        if (C1 != C2) {
            return (int)((unsigned char)C1) - (int)((unsigned char)C2);
        }

        String1++;
        String2++;
    }

    return (int)((unsigned char)ToLowerChar(*String1)) -
           (int)((unsigned char)ToLowerChar(*String2));
}

/*++

Routine Description:

    Compares two strings up to a maximum length (case-insensitive).

Arguments:

    String1 - First string.
    String2 - Second string.
    Count   - Maximum characters to compare.

Return Value:

    0 if equal up to Count characters, non-zero otherwise.

--*/

int
StrNICmp (
    const char* String1,
    const char* String2,
    int         Count
    )
{
    while (Count-- && *String1 && *String2) {
        char C1;
        char C2;

        C1 = ToLowerChar(*String1);
        C2 = ToLowerChar(*String2);

        if (C1 != C2) {
            return 1;
        }

        String1++;
        String2++;
    }

    return 0;
}

/*++

Routine Description:

    Tests whether a string ends with a given suffix (case-insensitive).

Arguments:

    String - String to test.
    Suffix - Suffix to check.

Return Value:

    Non-zero if String ends with Suffix, zero otherwise.

--*/

int
EndsWith (
    const char* String,
    const char* Suffix
    )
{
    int LengthString;
    int LengthSuffix;

    LengthString = StrLen(String);
    LengthSuffix = StrLen(Suffix);

    if (LengthSuffix > LengthString) {
        return 0;
    }

    return (StrICmp(String + LengthString - LengthSuffix, Suffix) == 0);
}

/*++

Routine Description:

    Performs a busy-wait delay.

Arguments:

    Ticks - Approximate delay factor.

Return Value:

    None.

--*/

VOID
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

    Prints a single character with an attribute at the current cursor position
    and advances the cursor.

Arguments:

    Character - Character to print.
    Attribute - Attribute byte.

Return Value:

    None.

--*/

VOID
PrintChar (
    char Character,
    char Attribute
    )
{
    if (Character == '\n') {
        cursor_pos = (cursor_pos / 160 + 1) * 160;
    } else {
        VIDEO_BUF[cursor_pos++] = Character;
        VIDEO_BUF[cursor_pos++] = Attribute;
    }

    UpdateCursor();
}

/*++

Routine Description:

    Prints a null-terminated string using the default attribute.

Arguments:

    String - String to print.

Return Value:

    None.

--*/

VOID
Print (
    const char* String
    )
{
    int Index;

    for (Index = 0; String[Index]; Index++) {
        PrintChar(String[Index], 0x07);
    }
}

/*++

Routine Description:

    Prints a decimal integer.

Arguments:

    Number - Integer to print.

Return Value:

    None.

--*/

VOID
PrintInt (
    int Number
    )
{
    char Buffer[10];
    int  Index;

    if (Number == 0) {
        PrintChar('0', 0x07);
        return;
    }

    if (Number < 0) {
        PrintChar('-', 0x07);
        Number = -Number;
    }

    Index = 0;
    while (Number > 0) {
        Buffer[Index++] = (char)((Number % 10) + '0');
        Number /= 10;
    }

    while (--Index >= 0) {
        PrintChar(Buffer[Index], 0x07);
    }
}

/*++

Routine Description:

    Clears the text-mode screen and resets the cursor.

Arguments:

    None.

Return Value:

    None.

--*/

VOID
ClearScreen (
    VOID
    )
{
    int Index;

    for (Index = 0; Index < 4000; Index += 2) {
        VIDEO_BUF[Index]   = ' ';
        VIDEO_BUF[Index+1] = 0x07;
    }

    cursor_pos = 0;
    UpdateCursor();
}

/*++

Routine Description:

    Reads a character from the keyboard, handling shift and basic control keys.

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

    Reads a line of input from the keyboard, echoing characters and handling
    backspace. ESC is ignored here (used in note editor instead).

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

    Ensures that a filename ends with ".note".

Arguments:

    Name - Filename buffer.

Return Value:

    None.

--*/

VOID
EnsureNoteExtension (
    char* Name
    )
{
    if (!EndsWith(Name, ".note")) {
        StrCat(Name, ".note");
    }
}

/*++

Routine Description:

    Finds a file by name in the in-memory file system.

Arguments:

    Name - Filename.

Return Value:

    Index of file or -1 if not found.

--*/

int
FindFileIndex (
    const char* Name
    )
{
    int Index;

    for (Index = 0; Index < file_count; Index++) {
        if (file_system[Index].active &&
            StrICmp(file_system[Index].name, Name) == 0) {
            return Index;
        }
    }

    return -1;
}

/*++

Routine Description:

    Implements a simple snake game. Displays score, moves the snake, updates
    high score, and reboots on exit.

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

        OldCursor = cursor_pos;
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

        if (SnakeX[0] < 0 || SnakeX[0] >= 80 ||
            SnakeY[0] < 1 || SnakeY[0] >= 25) {
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
    (void)GetChar();
    RebootSystem();
}

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

    Processes a single command line, including shell commands, note management,
    the snake game, and BOX script execution.

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
        char FileName[MAX_FILENAME];
        int  J;
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
        char FileName[MAX_FILENAME];
        int  J;
        const char* Source;

        J      = 0;
        Source = Command + 4;

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
                                 T < LineIndex &&
                                 T < (int)sizeof(LineLower) - 1;
                                 T++) {
                                LineLower[T] =
                                    ToLowerChar(LineBuffer[T]);
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
                        if (LineIndex <
                            (int)sizeof(LineBuffer) - 1) {
                            LineBuffer[LineIndex++] = Character;
                        }
                    }
                }

                if (LineIndex > 0) {
                    char LineLower[64];
                    int  T;

                    LineBuffer[LineIndex] = '\0';

                    for (T = 0;
                         T < LineIndex &&
                         T < (int)sizeof(LineLower) - 1;
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
            }
        }

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
        Print("Main Developer Clay Sanders, Co Developer Noah Juopperi\n");
        return;
    }

    if (Command[0] != '\0') {
        Print("Unknown command.\n");
    }
}

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
