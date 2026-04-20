[BITS 32]

section .multiboot
align 4
    dd 0x1BADB002              ; magic
    dd 0x00000004              ; flags (bit 2 = request video mode)
    dd -(0x1BADB002 + 0x4)     ; checksum
    dd 0, 0, 0, 0, 0          ; address fields (unused without bit 16)
    dd 0                       ; mode_type: 0 = linear/graphics
    dd 320                     ; width
    dd 200                     ; height
    dd 8                       ; depth (8 bpp = 256 colors)

section .text
global start
extern kernelMain

start:
    mov esp, 0x90000
    call kernelMain

.hang:
    cli
    hlt
    jmp .hang