/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef FVP_BASER_IRQ_H
#define FVP_BASER_IRQ_H

#include <stdint.h>

typedef enum IRQn {

    /* FVP watchdog interrupt line */
    FVP_WATCHDOG_IRQ = 32,

    /* FVP BaseR Generic Timer */
    FVP_SYSTEM_TIMER_IRQ = 57,

    IRQn_MAX = INT16_MAX,
} IRQn_Type;

#endif /* FVP_BASER_IRQ_H */
