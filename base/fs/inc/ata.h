/*++

Module Name:

    ata.h

Abstract:

    Declarations for the ATA/IDE PIO block device driver.
    Targets the primary IDE channel, master drive.

Author:

    Noah Juopperi <nipfswd@gmail.com>

--*/

#pragma once

#include "../../kernel/inc/types.h"

/*
 * ATA primary channel I/O base and control ports.
 */
#define ATA_PRIMARY_IO    0x1F0
#define ATA_PRIMARY_CTRL  0x3F6

/*
 * ATA register offsets from IO base.
 */
#define ATA_REG_DATA      0x00
#define ATA_REG_ERROR     0x01
#define ATA_REG_SECCOUNT  0x02
#define ATA_REG_LBA0      0x03
#define ATA_REG_LBA1      0x04
#define ATA_REG_LBA2      0x05
#define ATA_REG_DRIVE     0x06
#define ATA_REG_STATUS    0x07
#define ATA_REG_CMD       0x07

/*
 * ATA status bits.
 */
#define ATA_SR_BSY        0x80
#define ATA_SR_DRDY       0x40
#define ATA_SR_DRQ        0x08
#define ATA_SR_ERR        0x01

/*
 * ATA commands.
 */
#define ATA_CMD_READ_PIO  0x20
#define ATA_CMD_WRITE_PIO 0x30

/*
 * Sector size in bytes (standard).
 */
#define ATA_SECTOR_SIZE   512

/*
 * AtaReadSectors / AtaWriteSectors return codes.
 */
#define ATA_OK            0
#define ATA_ERR_TIMEOUT  -1
#define ATA_ERR_FAULT    -2

INT AtaReadSectors  ( uint32_t Lba, uint8_t Count, void* Buffer );
INT AtaWriteSectors ( uint32_t Lba, uint8_t Count, const void* Buffer );