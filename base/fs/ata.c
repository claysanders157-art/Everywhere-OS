/*++

Module Name:

    ata.c

Abstract:

    This module implements a simple ATA/IDE PIO driver for the primary
    channel master drive using 28-bit LBA addressing.

Author:

    Noah Juopperi <nipfswd@gmail.com>

Environment:

    x86 bare-metal, primary IDE channel (0x1F0), PIO mode.

--*/

#include "../inc/ata.h"
#include "../../kernel/inc/io.h"

/*
 * Timeout spin limit.
 */
#define ATA_TIMEOUT 0x100000

/*++

Routine Description:

    Waits until the drive is not busy, or times out.

Arguments:

    None.

Return Value:

    ATA_OK on success, ATA_ERR_TIMEOUT if BSY never cleared.

--*/

static INT
AtaWaitBsy (
    VOID
    )
{
    volatile int Guard;

    Guard = ATA_TIMEOUT;

    while ((inb(ATA_PRIMARY_IO + ATA_REG_STATUS) & ATA_SR_BSY) && Guard--) {
    }

    return (Guard > 0) ? ATA_OK : ATA_ERR_TIMEOUT;
}

/*++

Routine Description:

    Waits until DRQ (data request) is set, or times out.

Arguments:

    None.

Return Value:

    ATA_OK, ATA_ERR_TIMEOUT, or ATA_ERR_FAULT on drive error.

--*/

static INT
AtaWaitDrq (
    VOID
    )
{
    volatile int Guard;
    uint8_t      Status;

    Guard = ATA_TIMEOUT;

    for (;;) {
        Status = inb(ATA_PRIMARY_IO + ATA_REG_STATUS);

        if (Status & ATA_SR_ERR) {
            return ATA_ERR_FAULT;
        }

        if (Status & ATA_SR_DRQ) {
            return ATA_OK;
        }

        if (--Guard <= 0) {
            return ATA_ERR_TIMEOUT;
        }
    }
}

/*++

Routine Description:

    Selects the master drive and programs LBA + sector count registers.

Arguments:

    Lba   - 28-bit logical block address.
    Count - Number of sectors (1-255).

Return Value:

    ATA_OK or ATA_ERR_TIMEOUT.

--*/

static INT
AtaSetup (
    uint32_t Lba,
    uint8_t  Count
    )
{
    INT Result;

    Result = AtaWaitBsy();
    if (Result != ATA_OK) {
        return Result;
    }

    /* Select master drive, upper 4 LBA bits in low nibble. */
    outb(ATA_PRIMARY_IO + ATA_REG_DRIVE,
         (uint8_t)(0xE0 | ((Lba >> 24) & 0x0F)));

    outb(ATA_PRIMARY_IO + ATA_REG_SECCOUNT, Count);
    outb(ATA_PRIMARY_IO + ATA_REG_LBA0, (uint8_t)(Lba & 0xFF));
    outb(ATA_PRIMARY_IO + ATA_REG_LBA1, (uint8_t)((Lba >> 8)  & 0xFF));
    outb(ATA_PRIMARY_IO + ATA_REG_LBA2, (uint8_t)((Lba >> 16) & 0xFF));

    return ATA_OK;
}

/*++

Routine Description:

    Reads one or more 512-byte sectors from the disk into a buffer.

Arguments:

    Lba    - Starting 28-bit LBA sector address.
    Count  - Number of sectors to read (1-255).
    Buffer - Destination buffer (must be Count * 512 bytes).

Return Value:

    ATA_OK, ATA_ERR_TIMEOUT, or ATA_ERR_FAULT.

--*/

INT
AtaReadSectors (
    uint32_t Lba,
    uint8_t  Count,
    void*    Buffer
    )
{
    uint16_t* Words;
    uint8_t   Sector;
    int       Word;
    INT       Result;

    Result = AtaSetup(Lba, Count);
    if (Result != ATA_OK) {
        return Result;
    }

    outb(ATA_PRIMARY_IO + ATA_REG_CMD, ATA_CMD_READ_PIO);

    Words = (uint16_t*)Buffer;

    for (Sector = 0; Sector < Count; Sector++) {
        Result = AtaWaitDrq();
        if (Result != ATA_OK) {
            return Result;
        }

        for (Word = 0; Word < 256; Word++) {
            *Words++ = inw(ATA_PRIMARY_IO + ATA_REG_DATA);
        }
    }

    return ATA_OK;
}

/*++

Routine Description:

    Writes one or more 512-byte sectors from a buffer to the disk.

Arguments:

    Lba    - Starting 28-bit LBA sector address.
    Count  - Number of sectors to write (1-255).
    Buffer - Source buffer (must be Count * 512 bytes).

Return Value:

    ATA_OK, ATA_ERR_TIMEOUT, or ATA_ERR_FAULT.

--*/

INT
AtaWriteSectors (
    uint32_t    Lba,
    uint8_t     Count,
    const void* Buffer
    )
{
    const uint16_t* Words;
    uint8_t         Sector;
    int             Word;
    INT             Result;

    Result = AtaSetup(Lba, Count);
    if (Result != ATA_OK) {
        return Result;
    }

    outb(ATA_PRIMARY_IO + ATA_REG_CMD, ATA_CMD_WRITE_PIO);

    Words = (const uint16_t*)Buffer;

    for (Sector = 0; Sector < Count; Sector++) {
        Result = AtaWaitDrq();
        if (Result != ATA_OK) {
            return Result;
        }

        for (Word = 0; Word < 256; Word++) {
            outw(ATA_PRIMARY_IO + ATA_REG_DATA, *Words++);
        }

        /* Flush write cache after each sector. */
        outb(ATA_PRIMARY_IO + ATA_REG_CMD, 0xE7);
        AtaWaitBsy();
    }

    return ATA_OK;
}