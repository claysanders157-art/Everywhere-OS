/*++

Copyright (c) 2026  The EverywhereOS Authors

Module Name:

    kernel.c
    
Abstract:

    Entry point

Author:

    Noah Juopperi <nipfswd@gmail.com>
    Clay Sanders (made the first version of the kernel) <claylikepython@yahoo.com>

Environment:

    Literally the kernel.

NOTES:

    Perhaps we'll migrate this into multiple files.
    
--*/

#include <stdint.h>

/*++

Routine Description:

    Writes a byte to an I/O port.

Arguments:

    port - I/O port address.
    val  - Byte to write.

Return Value:

    None.

--*/

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ __volatile__("outb %0, %1" : : "a"(val), "Nd"(port));
}

/*++

Routine Description:

    Reads a byte from an I/O port.

Arguments:

    port - I/O port address.

Return Value:

    Byte read from the port.

--*/

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ __volatile__("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/*++

Routine Description:

    Attempts to reboot the machine via the keyboard controller.

Arguments:

    None.

Return Value:

    None. Does not return on success.

--*/

void RebootSystem(void) {
    while (inb(0x64) & 0x02) { }
    outb(0x64, 0xFE);
    for (;;) { }
}

#define SCR_W 320
#define SCR_H 200

uint8_t* FB = (uint8_t*)0xA0000;

/*++

Routine Description:

    Switches the video adapter to VGA Mode 13h (320x200x256 color).

Arguments:

    None.

Return Value:

    None.

--*/

void SetMode13h(void) {
    /* Video mode is now set by GRUB via the multiboot header.
       Program VGA palette to standard 256-color as a fallback. */
}

/*++

Routine Description:

    Sets a single pixel in the VGA framebuffer.

Arguments:

    x - X coordinate.
    y - Y coordinate.
    c - Color index.

Return Value:

    None.

--*/

void PutPixel(int x, int y, uint8_t c) {
    if (x < 0 || x >= SCR_W || y < 0 || y >= SCR_H) return;
    FB[y * SCR_W + x] = c;
}

/*++

Routine Description:

    Fills a rectangular region with a solid color.

Arguments:

    x - Left edge.
    y - Top edge.
    w - Width in pixels.
    h - Height in pixels.
    c - Color index.

Return Value:

    None.

--*/

void FillRect(int x, int y, int w, int h, uint8_t c) {
    for (int yy = 0; yy < h; yy++) {
        int py = y + yy;
        if (py < 0 || py >= SCR_H) continue;
        for (int xx = 0; xx < w; xx++) {
            int px = x + xx;
            if (px < 0 || px >= SCR_W) continue;
            FB[py * SCR_W + px] = c;
        }
    }
}

uint8_t Font8x8[128][8];

/*++

Routine Description:

    Initializes the 8x8 bitmap font for printable ASCII characters (32-126).

Arguments:

    None.

Return Value:

    None.

--*/

void InitFont(void) {
    /* Clear all glyphs to zero */
    for (int c = 0; c < 128; c++)
        for (int r = 0; r < 8; r++)
            Font8x8[c][r] = 0x00;

    /* Proper 8x8 bitmap font for printable ASCII (32-126) */
    /* Each row: MSB = leftmost pixel */
    static const uint8_t fontdata[95][8] = {
        /* 32 ' ' */ {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        /* 33 '!' */ {0x18,0x18,0x18,0x18,0x18,0x00,0x18,0x00},
        /* 34 '"' */ {0x6C,0x6C,0x00,0x00,0x00,0x00,0x00,0x00},
        /* 35 '#' */ {0x6C,0x6C,0xFE,0x6C,0xFE,0x6C,0x6C,0x00},
        /* 36 '$' */ {0x18,0x3E,0x60,0x3C,0x06,0x7C,0x18,0x00},
        /* 37 '%' */ {0x00,0xC6,0xCC,0x18,0x30,0x66,0xC6,0x00},
        /* 38 '&' */ {0x38,0x6C,0x38,0x76,0xDC,0xCC,0x76,0x00},
        /* 39 ''' */ {0x18,0x18,0x30,0x00,0x00,0x00,0x00,0x00},
        /* 40 '(' */ {0x0C,0x18,0x30,0x30,0x30,0x18,0x0C,0x00},
        /* 41 ')' */ {0x30,0x18,0x0C,0x0C,0x0C,0x18,0x30,0x00},
        /* 42 '*' */ {0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00},
        /* 43 '+' */ {0x00,0x18,0x18,0x7E,0x18,0x18,0x00,0x00},
        /* 44 ',' */ {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x30},
        /* 45 '-' */ {0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00},
        /* 46 '.' */ {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00},
        /* 47 '/' */ {0x06,0x0C,0x18,0x30,0x60,0xC0,0x80,0x00},
        /* 48 '0' */ {0x3C,0x66,0x6E,0x7E,0x76,0x66,0x3C,0x00},
        /* 49 '1' */ {0x18,0x38,0x18,0x18,0x18,0x18,0x7E,0x00},
        /* 50 '2' */ {0x3C,0x66,0x06,0x0C,0x18,0x30,0x7E,0x00},
        /* 51 '3' */ {0x3C,0x66,0x06,0x1C,0x06,0x66,0x3C,0x00},
        /* 52 '4' */ {0x0C,0x1C,0x3C,0x6C,0x7E,0x0C,0x0C,0x00},
        /* 53 '5' */ {0x7E,0x60,0x7C,0x06,0x06,0x66,0x3C,0x00},
        /* 54 '6' */ {0x1C,0x30,0x60,0x7C,0x66,0x66,0x3C,0x00},
        /* 55 '7' */ {0x7E,0x06,0x0C,0x18,0x30,0x30,0x30,0x00},
        /* 56 '8' */ {0x3C,0x66,0x66,0x3C,0x66,0x66,0x3C,0x00},
        /* 57 '9' */ {0x3C,0x66,0x66,0x3E,0x06,0x0C,0x38,0x00},
        /* 58 ':' */ {0x00,0x00,0x18,0x00,0x00,0x18,0x00,0x00},
        /* 59 ';' */ {0x00,0x00,0x18,0x00,0x00,0x18,0x18,0x30},
        /* 60 '<' */ {0x0C,0x18,0x30,0x60,0x30,0x18,0x0C,0x00},
        /* 61 '=' */ {0x00,0x00,0x7E,0x00,0x7E,0x00,0x00,0x00},
        /* 62 '>' */ {0x30,0x18,0x0C,0x06,0x0C,0x18,0x30,0x00},
        /* 63 '?' */ {0x3C,0x66,0x06,0x0C,0x18,0x00,0x18,0x00},
        /* 64 '@' */ {0x3C,0x66,0x6E,0x6A,0x6E,0x60,0x3C,0x00},
        /* 65 'A' */ {0x18,0x3C,0x66,0x66,0x7E,0x66,0x66,0x00},
        /* 66 'B' */ {0x7C,0x66,0x66,0x7C,0x66,0x66,0x7C,0x00},
        /* 67 'C' */ {0x3C,0x66,0x60,0x60,0x60,0x66,0x3C,0x00},
        /* 68 'D' */ {0x78,0x6C,0x66,0x66,0x66,0x6C,0x78,0x00},
        /* 69 'E' */ {0x7E,0x60,0x60,0x7C,0x60,0x60,0x7E,0x00},
        /* 70 'F' */ {0x7E,0x60,0x60,0x7C,0x60,0x60,0x60,0x00},
        /* 71 'G' */ {0x3C,0x66,0x60,0x6E,0x66,0x66,0x3E,0x00},
        /* 72 'H' */ {0x66,0x66,0x66,0x7E,0x66,0x66,0x66,0x00},
        /* 73 'I' */ {0x3C,0x18,0x18,0x18,0x18,0x18,0x3C,0x00},
        /* 74 'J' */ {0x06,0x06,0x06,0x06,0x06,0x66,0x3C,0x00},
        /* 75 'K' */ {0x66,0x6C,0x78,0x70,0x78,0x6C,0x66,0x00},
        /* 76 'L' */ {0x60,0x60,0x60,0x60,0x60,0x60,0x7E,0x00},
        /* 77 'M' */ {0xC6,0xEE,0xFE,0xD6,0xC6,0xC6,0xC6,0x00},
        /* 78 'N' */ {0x66,0x76,0x7E,0x7E,0x6E,0x66,0x66,0x00},
        /* 79 'O' */ {0x3C,0x66,0x66,0x66,0x66,0x66,0x3C,0x00},
        /* 80 'P' */ {0x7C,0x66,0x66,0x7C,0x60,0x60,0x60,0x00},
        /* 81 'Q' */ {0x3C,0x66,0x66,0x66,0x6A,0x6C,0x36,0x00},
        /* 82 'R' */ {0x7C,0x66,0x66,0x7C,0x6C,0x66,0x66,0x00},
        /* 83 'S' */ {0x3C,0x66,0x60,0x3C,0x06,0x66,0x3C,0x00},
        /* 84 'T' */ {0x7E,0x18,0x18,0x18,0x18,0x18,0x18,0x00},
        /* 85 'U' */ {0x66,0x66,0x66,0x66,0x66,0x66,0x3C,0x00},
        /* 86 'V' */ {0x66,0x66,0x66,0x66,0x66,0x3C,0x18,0x00},
        /* 87 'W' */ {0xC6,0xC6,0xC6,0xD6,0xFE,0xEE,0xC6,0x00},
        /* 88 'X' */ {0x66,0x66,0x3C,0x18,0x3C,0x66,0x66,0x00},
        /* 89 'Y' */ {0x66,0x66,0x66,0x3C,0x18,0x18,0x18,0x00},
        /* 90 'Z' */ {0x7E,0x06,0x0C,0x18,0x30,0x60,0x7E,0x00},
        /* 91 '[' */ {0x3C,0x30,0x30,0x30,0x30,0x30,0x3C,0x00},
        /* 92 '\' */ {0x80,0xC0,0x60,0x30,0x18,0x0C,0x06,0x00},
        /* 93 ']' */ {0x3C,0x0C,0x0C,0x0C,0x0C,0x0C,0x3C,0x00},
        /* 94 '^' */ {0x18,0x3C,0x66,0x00,0x00,0x00,0x00,0x00},
        /* 95 '_' */ {0x00,0x00,0x00,0x00,0x00,0x00,0xFE,0x00},
        /* 96 '`' */ {0x30,0x18,0x0C,0x00,0x00,0x00,0x00,0x00},
        /* 97 'a' */ {0x00,0x00,0x3C,0x06,0x3E,0x66,0x3E,0x00},
        /* 98 'b' */ {0x60,0x60,0x7C,0x66,0x66,0x66,0x7C,0x00},
        /* 99 'c' */ {0x00,0x00,0x3C,0x66,0x60,0x66,0x3C,0x00},
        /*100 'd' */ {0x06,0x06,0x3E,0x66,0x66,0x66,0x3E,0x00},
        /*101 'e' */ {0x00,0x00,0x3C,0x66,0x7E,0x60,0x3C,0x00},
        /*102 'f' */ {0x1C,0x30,0x30,0x7C,0x30,0x30,0x30,0x00},
        /*103 'g' */ {0x00,0x00,0x3E,0x66,0x66,0x3E,0x06,0x3C},
        /*104 'h' */ {0x60,0x60,0x7C,0x66,0x66,0x66,0x66,0x00},
        /*105 'i' */ {0x18,0x00,0x38,0x18,0x18,0x18,0x3C,0x00},
        /*106 'j' */ {0x06,0x00,0x0E,0x06,0x06,0x66,0x66,0x3C},
        /*107 'k' */ {0x60,0x60,0x66,0x6C,0x78,0x6C,0x66,0x00},
        /*108 'l' */ {0x38,0x18,0x18,0x18,0x18,0x18,0x3C,0x00},
        /*109 'm' */ {0x00,0x00,0xEC,0xFE,0xD6,0xC6,0xC6,0x00},
        /*110 'n' */ {0x00,0x00,0x7C,0x66,0x66,0x66,0x66,0x00},
        /*111 'o' */ {0x00,0x00,0x3C,0x66,0x66,0x66,0x3C,0x00},
        /*112 'p' */ {0x00,0x00,0x7C,0x66,0x66,0x7C,0x60,0x60},
        /*113 'q' */ {0x00,0x00,0x3E,0x66,0x66,0x3E,0x06,0x06},
        /*114 'r' */ {0x00,0x00,0x7C,0x66,0x60,0x60,0x60,0x00},
        /*115 's' */ {0x00,0x00,0x3E,0x60,0x3C,0x06,0x7C,0x00},
        /*116 't' */ {0x30,0x30,0x7C,0x30,0x30,0x30,0x1C,0x00},
        /*117 'u' */ {0x00,0x00,0x66,0x66,0x66,0x66,0x3E,0x00},
        /*118 'v' */ {0x00,0x00,0x66,0x66,0x66,0x3C,0x18,0x00},
        /*119 'w' */ {0x00,0x00,0xC6,0xC6,0xD6,0xFE,0x6C,0x00},
        /*120 'x' */ {0x00,0x00,0x66,0x3C,0x18,0x3C,0x66,0x00},
        /*121 'y' */ {0x00,0x00,0x66,0x66,0x66,0x3E,0x06,0x3C},
        /*122 'z' */ {0x00,0x00,0x7E,0x0C,0x18,0x30,0x7E,0x00},
        /*123 '{' */ {0x0E,0x18,0x18,0x70,0x18,0x18,0x0E,0x00},
        /*124 '|' */ {0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x00},
        /*125 '}' */ {0x70,0x18,0x18,0x0E,0x18,0x18,0x70,0x00},
        /*126 '~' */ {0x00,0x00,0x36,0x6C,0x00,0x00,0x00,0x00},
    };

    for (int c = 0; c < 95; c++)
        for (int r = 0; r < 8; r++)
            Font8x8[c + 32][r] = fontdata[c][r];
}

/*++

Routine Description:

    Draws a single character from the 8x8 font at the given position.

Arguments:

    x     - X coordinate.
    y     - Y coordinate.
    ch    - ASCII character to draw.
    color - Color index.

Return Value:

    None.

--*/

void DrawChar(int x, int y, char ch, uint8_t color) {
    if ((unsigned char)ch > 127) return;
    uint8_t* g = Font8x8[(int)ch];
    for (int r = 0; r < 8; r++) {
        uint8_t line = g[r];
        for (int c = 0; c < 8; c++) {
            if (line & (1 << (7 - c))) {
                PutPixel(x + c, y + r, color);
            }
        }
    }
}

/*++

Routine Description:

    Draws a null-terminated string, advancing horizontally and wrapping on
    newline characters.

Arguments:

    x     - Starting X coordinate.
    y     - Starting Y coordinate.
    s     - String to draw.
    color - Color index.

Return Value:

    None.

--*/

void DrawString(int x, int y, const char* s, uint8_t color) {
    int cx = x;
    while (*s) {
        if (*s == '\n') {
            y += 8;
            cx = x;
        } else {
            DrawChar(cx, y, *s, color);
            cx += 8;
        }
        s++;
    }
}

/* ========== Desktop + Windows ========== */

typedef struct {
    int x, y, w, h;
    int vx, vy;       /* velocity for Physics */
    const char* title;
    int visible;
    int minimized;
    int dragging;
    int drag_off_x;
    int drag_off_y;
} WINDOW;

WINDOW ShellWin  = { 10, 10, 300, 70, 0, 0, "Shell", 1, 0, 0, 0, 0 };
WINDOW NotesWin  = { 10, 85, 300, 70, 0, 0, "Notes", 1, 0, 0, 0, 0 };
WINDOW SnakeWin  = { 60, 40, 200, 120, 0, 0, "Snake", 0, 0, 0, 0, 0 };

/* 0 = Shell, 1 = Notes, 2 = Snake */
int active_window = 0;

/*++

Routine Description:

    Draws the desktop background and taskbar base.

Arguments:

    None.

Return Value:

    None.

--*/

void DrawDesktop(void) {
    /* Retro Chaos: deep blue background, taskbar */
    FillRect(0, 0, SCR_W, SCR_H, 0x01);
    FillRect(0, SCR_H - 12, SCR_W, 12, 0x08);
    DrawString(4, SCR_H - 10, "Everywhere OS", 0x0F);
}

/*++

Routine Description:

    Draws the window frame including border, body, title bar, close button,
    and minimize button.

Arguments:

    w - Pointer to the WINDOW structure.

Return Value:

    None.

--*/

void DrawWindowFrame(WINDOW* w) {
    if (!w->visible || w->minimized) return;

    /* Border */
    FillRect(w->x - 1, w->y - 1, w->w + 2, w->h + 2, 0x03);
    /* Body */
    FillRect(w->x, w->y, w->w, w->h, 0x00);
    /* Title bar */
    FillRect(w->x, w->y, w->w, 10, 0x0D);
    DrawString(w->x + 3, w->y + 1, w->title, 0x0F);

    /* Close button [X] */
    FillRect(w->x + w->w - 10, w->y + 1, 8, 8, 0x0E);
    DrawChar(w->x + w->w - 9, w->y + 1, 'X', 0x00);

    /* Minimize button [_] */
    FillRect(w->x + w->w - 20, w->y + 1, 8, 8, 0x0E);
    DrawChar(w->x + w->w - 19, w->y + 1, '_', 0x00);
}

/*++

Routine Description:

    Tests whether a point lies within a rectangle.

Arguments:

    x  - Point X coordinate.
    y  - Point Y coordinate.
    rx - Rectangle left edge.
    ry - Rectangle top edge.
    rw - Rectangle width.
    rh - Rectangle height.

Return Value:

    Non-zero if the point is inside the rectangle, zero otherwise.

--*/

int PointInRect(int x, int y, int rx, int ry, int rw, int rh) {
    return (x >= rx && x < rx + rw && y >= ry && y < ry + rh);
}

int mouse_x = SCR_W / 2;
int mouse_y = SCR_H / 2;
int mouse_buttons = 0;
int mouse_prev_buttons = 0;

/*++

Routine Description:

    Waits for the PS/2 controller to be ready for input or output.

Arguments:

    type - 0 to wait for output buffer full, 1 to wait for input buffer empty.

Return Value:

    None.

--*/

void MouseWait(uint8_t type) {
    uint32_t timeout = 100000;
    if (type == 0) {
        while (timeout--) {
            if (inb(0x64) & 1) return;
        }
    } else {
        while (timeout--) {
            if (!(inb(0x64) & 2)) return;
        }
    }
}

/*++

Routine Description:

    Sends a byte to the PS/2 mouse device.

Arguments:

    data - Byte to send.

Return Value:

    None.

--*/

void MouseWrite(uint8_t data) {
    MouseWait(1);
    outb(0x64, 0xD4);
    MouseWait(1);
    outb(0x60, data);
}

/*++

Routine Description:

    Reads a byte from the PS/2 mouse device.

Arguments:

    None.

Return Value:

    Byte read from the mouse.

--*/

uint8_t MouseRead(void) {
    MouseWait(0);
    return inb(0x60);
}

/*++

Routine Description:

    Initializes the PS/2 mouse by enabling the auxiliary device,
    setting defaults, and enabling data reporting.

Arguments:

    None.

Return Value:

    None.

--*/

void InitMouse(void) {
    uint8_t status;

    outb(0x64, 0xA8);
    MouseWait(1);
    outb(0x64, 0x20);
    MouseWait(0);
    status = inb(0x60) | 2;
    MouseWait(1);
    outb(0x64, 0x60);
    MouseWait(1);
    outb(0x60, status);

    MouseWrite(0xF6);
    MouseRead();
    MouseWrite(0xF4);
    MouseRead();

    /* Flush any leftover bytes in the PS/2 output buffer */
    while (inb(0x64) & 1) {
        inb(0x60);
    }
}

/*++

Routine Description:

    Polls the PS/2 controller for a complete 3-byte mouse packet and
    updates the global mouse position and button state.

Arguments:

    None.

Return Value:

    None.

--*/

void UpdateMouse(void) {
    if (!(inb(0x64) & 1)) return;

    /* Only read if bit 5 indicates this is mouse data */
    if (!(inb(0x64) & 0x20)) return;

    static uint8_t packet[3];
    static int idx = 0;

    packet[idx++] = inb(0x60);
    if (idx < 3) return;
    idx = 0;

    int dx = (int8_t)packet[1];
    int dy = (int8_t)packet[2];

    mouse_prev_buttons = mouse_buttons;
    mouse_buttons = packet[0] & 0x07;

    mouse_x += dx;
    mouse_y -= dy;

    if (mouse_x < 0) mouse_x = 0;
    if (mouse_x >= SCR_W) mouse_x = SCR_W - 1;
    if (mouse_y < 0) mouse_y = 0;
    if (mouse_y >= SCR_H) mouse_y = SCR_H - 1;
}

/*++

Routine Description:

    Draws a small triangular mouse cursor at the current mouse position.

Arguments:

    None.

Return Value:

    None.

--*/

void DrawMouseCursor(void) {
    /* Bright red triangle */
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j <= i; j++) {
            PutPixel(mouse_x + j, mouse_y + i, 0x04);
        }
    }
}

int shift_pressed = 0;
uint8_t last_scancode = 0;

/*++

Routine Description:

    Reads a single keypress from the keyboard controller, translating
    scancodes to ASCII. Handles shift state.

Arguments:

    None.

Return Value:

    ASCII character, '\n' for Enter, 0x08 for Backspace, 27 for ESC,
    or 0 if no key is available.

--*/

char GetKeyChar(void) {
    if (!(inb(0x64) & 1)) return 0;

    uint8_t status = inb(0x64);

    /* Bit 5 set means this byte is from the mouse, not the keyboard */
    if (status & 0x20) {
        inb(0x60); /* discard mouse byte here; UpdateMouse handles it */
        return 0;
    }

    uint8_t sc = inb(0x60);
    last_scancode = sc;

    if (sc == 0x2A || sc == 0x36) { shift_pressed = 1; return 0; }
    if (sc == 0xAA || sc == 0xB6) { shift_pressed = 0; return 0; }

    if (sc & 0x80) return 0;

    if (sc == 0x1C) return '\n';
    if (sc == 0x0E) return 0x08;
    if (sc == 0x01) return 27;

    static char Lower[] = {
        0,0,'1','2','3','4','5','6','7','8','9','0','-','=',0,0,
        'q','w','e','r','t','y','u','i','o','p','[',']',0,0,
        'a','s','d','f','g','h','j','k','l',';','\'','`',0,'\\',
        'z','x','c','v','b','n','m',',','.','/',0,'*',0,' '
    };

    static char Upper[] = {
        0,0,'!','@','#','$','%','^','&','*','(',')','_','+',0,0,
        'Q','W','E','R','T','Y','U','I','O','P','{','}',0,0,
        'A','S','D','F','G','H','J','K','L',':','\"','~',0,'|',
        'Z','X','C','V','B','N','M','<','>','?',0,'*',0,' '
    };

    if (sc < sizeof(Lower)) {
        return shift_pressed ? Upper[sc] : Lower[sc];
    }

    return 0;
}

char shell_input[64];
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

    Compares two null-terminated strings for equality.

Arguments:

    a - First string.
    b - Second string.

Return Value:

    Non-zero if the strings are equal, zero otherwise.

--*/

int StrEq(const char* a, const char* b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return (*a == 0 && *b == 0);
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

char notes_buf[256];
int  notes_len = 0;

/*++

Routine Description:

    Draws the notes text inside the notes window.

Arguments:

    None.

Return Value:

    None.

--*/

void NotesDraw(void) {
    if (!NotesWin.visible || NotesWin.minimized) return;
    DrawString(NotesWin.x + 4, NotesWin.y + 14, notes_buf, 0x0F);
}

#define SNAKE_MAX 100
int snake_x[SNAKE_MAX];
int snake_y[SNAKE_MAX];
int snake_len = 5;
int snake_dx = 1;
int snake_dy = 0;
int food_x = 20;
int food_y = 10;

/*++

Routine Description:

    Initializes the snake body positions.

Arguments:

    None.

Return Value:

    None.

--*/

void SnakeInit(void) {
    for (int i = 0; i < snake_len; i++) {
        snake_x[i] = 10 - i;
        snake_y[i] = 10;
    }
}

/*++

Routine Description:

    Advances the snake by one step, handles boundary clamping and
    food collision.

Arguments:

    None.

Return Value:

    None.

--*/

void SnakeStep(void) {
    if (!SnakeWin.visible || SnakeWin.minimized) return;

    for (int i = snake_len - 1; i > 0; i--) {
        snake_x[i] = snake_x[i - 1];
        snake_y[i] = snake_y[i - 1];
    }
    snake_x[0] += snake_dx;
    snake_y[0] += snake_dy;

    if (snake_x[0] < 0) snake_x[0] = 0;
    if (snake_y[0] < 0) snake_y[0] = 0;
    if (snake_x[0] > SnakeWin.w - 10) snake_x[0] = SnakeWin.w - 10;
    if (snake_y[0] > SnakeWin.h - 20) snake_y[0] = SnakeWin.h - 20;

    if (snake_x[0] == food_x && snake_y[0] == food_y) {
        if (snake_len < SNAKE_MAX) snake_len++;
        food_x = (food_x * 7 + 13) % (SnakeWin.w - 10);
        food_y = (food_y * 5 + 11) % (SnakeWin.h - 20);
    }
}

/*++

Routine Description:

    Draws the snake body and food inside the snake window.

Arguments:

    None.

Return Value:

    None.

--*/

void SnakeDraw(void) {
    if (!SnakeWin.visible || SnakeWin.minimized) return;

    FillRect(SnakeWin.x + 2, SnakeWin.y + 12, SnakeWin.w - 4, SnakeWin.h - 14, 0x00);

    for (int i = 0; i < snake_len; i++) {
        PutPixel(SnakeWin.x + 5 + snake_x[i],
                 SnakeWin.y + 15 + snake_y[i], 0x0A);
    }

    PutPixel(SnakeWin.x + 5 + food_x,
             SnakeWin.y + 15 + food_y, 0x04);
}

/*++

Routine Description:

    Applies velocity, friction, and edge-bounce physics to a window.

Arguments:

    w - Pointer to the WINDOW structure.

Return Value:

    None.

--*/

void UpdateWindowPhysics(WINDOW* w) {
    if (!w->visible || w->minimized) return;

    w->x += w->vx;
    w->y += w->vy;

    /* friction */
    w->vx = (w->vx * 9) / 10;
    w->vy = (w->vy * 9) / 10;

    /* bounce on edges */
    if (w->x < 0) { w->x = 0; w->vx = -w->vx / 2; }
    if (w->y < 0) { w->y = 0; w->vy = -w->vy / 2; }
    if (w->x + w->w > SCR_W) { w->x = SCR_W - w->w; w->vx = -w->vx / 2; }
    if (w->y + w->h > SCR_H - 12) { w->y = SCR_H - 12 - w->h; w->vy = -w->vy / 2; }
}

/*++

Routine Description:

    Handles mouse interaction with a window: click-to-focus, title bar
    dragging, close button, and minimize button.

Arguments:

    w      - Pointer to the WINDOW structure.
    win_id - Window identifier for focus tracking.

Return Value:

    None.

--*/

void HandleWindowMouse(WINDOW* w, int win_id) {
    if (!w->visible) return;

    int left_down  = mouse_buttons & 1;
    int left_prev  = mouse_prev_buttons & 1;

    /* Click anywhere in window to focus */
    if (left_down && !left_prev && !w->minimized &&
        PointInRect(mouse_x, mouse_y, w->x, w->y, w->w, w->h)) {
        active_window = win_id;
    }

    /* Click on title bar to drag */
    if (!w->dragging && left_down && !left_prev &&
        PointInRect(mouse_x, mouse_y, w->x, w->y, w->w, 10)) {
        w->dragging = 1;
        w->drag_off_x = mouse_x - w->x;
        w->drag_off_y = mouse_y - w->y;
        w->vx = 0;
        w->vy = 0;
    }

    if (w->dragging && left_down) {
        int new_x = mouse_x - w->drag_off_x;
        int new_y = mouse_y - w->drag_off_y;
        w->vx = new_x - w->x;
        w->vy = new_y - w->y;
        w->x = new_x;
        w->y = new_y;
    }

    if (w->dragging && !left_down && left_prev) {
        w->dragging = 0;
    }

    /* Close button */
    if (left_down && !left_prev &&
        PointInRect(mouse_x, mouse_y, w->x + w->w - 10, w->y + 1, 8, 8)) {
        w->visible = 0;
    }

    /* Minimize button */
    if (left_down && !left_prev &&
        PointInRect(mouse_x, mouse_y, w->x + w->w - 20, w->y + 1, 8, 8)) {
        w->minimized = 1;
    }
}

/*++

Routine Description:

    Handles mouse clicks on taskbar items to restore minimized windows.

Arguments:

    None.

Return Value:

    None.

--*/

void HandleTaskbarClick(void) {
    int left_down = mouse_buttons & 1;
    int left_prev = mouse_prev_buttons & 1;
    if (!(left_down && !left_prev)) return;
    if (!PointInRect(mouse_x, mouse_y, 0, SCR_H - 12, SCR_W, 12)) return;

    int x = 120;
    if (ShellWin.visible && ShellWin.minimized) {
        if (PointInRect(mouse_x, mouse_y, x, SCR_H - 11, 40, 10)) {
            ShellWin.minimized = 0;
            active_window = 0;
            return;
        }
        x += 44;
    }
    if (NotesWin.visible && NotesWin.minimized) {
        if (PointInRect(mouse_x, mouse_y, x, SCR_H - 11, 40, 10)) {
            NotesWin.minimized = 0;
            active_window = 1;
            return;
        }
        x += 44;
    }
    if (SnakeWin.visible && SnakeWin.minimized) {
        if (PointInRect(mouse_x, mouse_y, x, SCR_H - 11, 40, 10)) {
            SnakeWin.minimized = 0;
            active_window = 2;
            return;
        }
        x += 44;
    }
}

/*++

Routine Description:

    Draws the taskbar with the OS name and buttons for minimized windows.

Arguments:

    None.

Return Value:

    None.

--*/

void DrawTaskbar(void) {
    FillRect(0, SCR_H - 12, SCR_W, 12, 0x08);
    DrawString(4, SCR_H - 10, "Everywhere OS", 0x0F);

    int x = 120;
    if (ShellWin.visible && ShellWin.minimized) {
        FillRect(x, SCR_H - 11, 40, 10, 0x03);
        DrawString(x + 2, SCR_H - 10, "Shell", 0x0F);
        x += 44;
    }
    if (NotesWin.visible && NotesWin.minimized) {
        FillRect(x, SCR_H - 11, 40, 10, 0x03);
        DrawString(x + 2, SCR_H - 10, "Notes", 0x0F);
        x += 44;
    }
    if (SnakeWin.visible && SnakeWin.minimized) {
        FillRect(x, SCR_H - 11, 40, 10, 0x03);
        DrawString(x + 2, SCR_H - 10, "Snake", 0x0F);
        x += 44;
    }
}

/*++

Routine Description:

    Displays the boot splash screen with the OS tagline.

Arguments:

    None.

Return Value:

    None.

--*/

void DrawBootScreen(void) {
    FillRect(0, 0, SCR_W, SCR_H, 0x00);
    DrawString(10, 80,
        "Why Go Anywhere When You Are Everywhere?\nEverywhere OS",
        0x0F);
}

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
                } else if (shell_len < (int)sizeof(shell_input) - 1) {
                    shell_input[shell_len++] = ch;
                    shell_input[shell_len] = 0;
                }
            } else if (active_window == 1) {
                /* Notes input */
                if (ch == 0x08) {
                    if (notes_len > 0) notes_buf[--notes_len] = 0;
                } else if (notes_len < (int)sizeof(notes_buf) - 1) {
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

        for (volatile int d = 0; d < 30000; d++) { __asm__ __volatile__("nop"); }
    }
}