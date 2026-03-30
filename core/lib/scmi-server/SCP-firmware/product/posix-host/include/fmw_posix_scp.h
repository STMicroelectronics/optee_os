/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef FMW_POSIX_SCP_H
#define FMW_POSIX_SCP_H

#include <stdint.h>

typedef enum IRQn {
    Reset_IRQn = -15,
    NonMaskableInt_IRQn = -14,
    HardFault_IRQn = -13,
    MemoryManagement_IRQn = -12,
    BusFault_IRQn = -11,
    UsageFault_IRQn = -10,
    SVCall_IRQn = -5,
    DebugMonitor_IRQn = -4,
    PendSV_IRQn = -2,
    SysTick_IRQn = -1,

    PosixMqueue0_IRQ = 0x10,
    PosixMqueue1_IRQ = 0x11,
    PosixMqueue2_IRQ = 0x12,
    PosixMqueue3_IRQ = 0x13,
    PosixMqueue4_IRQ = 0x14,

    IRQn_MAX = INT16_MAX,
} IRQn_Type;

#endif /* FMW_POSIX_SCP_H */
