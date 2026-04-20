/*++

Copyright (c) 2026  The EverywhereOS Authors

Module Name:

    halinit.c

Abstract:

    Hardware Abstraction Layer initialization. Remaps the 8259 PIC,
    sets up the IDT, and installs hardware interrupt handlers.

Author:

    Noah Juopperi <nipfswd@gmail.com>
    Clay Sanders (made the first version of the kernel) <claylikepython@yahoo.com>

Environment:

    Kernel-mode only (HAL)

--*/

#include "ke.h"

/* IDT gate descriptor */
struct idt_entry {
    uint16_t offset_lo;
    uint16_t selector;
    uint8_t  zero;
    uint8_t  type_attr;
    uint16_t offset_hi;
} __attribute__((packed));

/* IDT pointer for LIDT */
struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

static struct idt_entry idt[256];
static struct idt_ptr   idtp;

/*++

Routine Description:

    Sets a single IDT gate entry.

Arguments:

    num       - Interrupt vector number (0-255).
    handler   - Address of the ISR stub.
    selector  - Code segment selector (0x08 for flat 32-bit).
    type_attr - Type and attributes (0x8E = 32-bit interrupt gate, ring 0).

Return Value:

    None.

--*/

static void IdtSetGate(uint8_t num, uint32_t handler, uint16_t selector, uint8_t type_attr) {
    idt[num].offset_lo = handler & 0xFFFF;
    idt[num].offset_hi = (handler >> 16) & 0xFFFF;
    idt[num].selector  = selector;
    idt[num].zero      = 0;
    idt[num].type_attr = type_attr;
}

/*++

Routine Description:

    Remaps the 8259 PIC so that IRQ 0-7 map to vectors 0x20-0x27
    and IRQ 8-15 map to vectors 0x28-0x2F, preventing collision
    with CPU exception vectors (0x00-0x1F).

Arguments:

    None.

Return Value:

    None.

--*/

static void PicRemap(void) {
    uint8_t mask1 = inb(0x21);
    uint8_t mask2 = inb(0xA1);

    outb(0x20, 0x11);
    outb(0xA0, 0x11);

    outb(0x21, 0x20);
    outb(0xA1, 0x28);

    outb(0x21, 0x04);
    outb(0xA1, 0x02);

    outb(0x21, 0x01);
    outb(0xA1, 0x01);

    outb(0x21, mask1);
    outb(0xA1, mask2);
}

/*++

Routine Description:

    Sends End-Of-Interrupt to the PIC(s) for the given IRQ.

Arguments:

    irq - IRQ number (0-15).

Return Value:

    None.

--*/

void HalEndOfInterrupt(uint8_t irq) {
    if (irq >= 8) {
        outb(0xA0, 0x20);
    }
    outb(0x20, 0x20);
}

/*++

Routine Description:

    Initializes the HAL: remaps the PIC, sets up the IDT with the
    mouse IRQ12 handler, unmasks IRQ2 (cascade) and IRQ12, loads
    the IDT, and enables hardware interrupts.

Arguments:

    None.

Return Value:

    None.

--*/

void HalInitInterrupts(void) {
    extern void Irq12Stub(void);
    extern void IrqIgnoreStub(void);

    /* Install a default ignore handler for all 256 vectors
       so any unexpected interrupt won't triple-fault */
    for (int i = 0; i < 256; i++) {
        IdtSetGate(i, (uint32_t)IrqIgnoreStub, 0x08, 0x8E);
    }

    PicRemap();

    /* Mask ALL IRQs first, then only unmask what we need */
    outb(0x21, 0xFF);
    outb(0xA1, 0xFF);

    /* Install mouse ISR (IRQ12 = vector 0x2C) */
    IdtSetGate(0x2C, (uint32_t)Irq12Stub, 0x08, 0x8E);

    /* Load IDT */
    idtp.limit = sizeof(idt) - 1;
    idtp.base  = (uint32_t)&idt;
    __asm__ __volatile__("lidt %0" : : "m"(idtp));

    /* Unmask IRQ2 (cascade) and IRQ12 (mouse) only */
    outb(0x21, 0xFB);   /* master: only IRQ2 unmasked */
    outb(0xA1, 0xEF);   /* slave:  only IRQ12 unmasked */

    /* Enable interrupts */
    __asm__ __volatile__("sti");
}
