/*++

Module Name:

    io.h

Abstract:

    Declarations for port I/O and system control routines.

Author:

    Noah Juopperi <nipfswd@gmail.com>

--*/

#pragma once

#include "types.h"

VOID    outb         ( uint16_t Port, uint8_t Data );
uint8_t inb          ( uint16_t Port );
VOID    RebootSystem ( VOID );