/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SI0_IRQ_H
#define SI0_IRQ_H

#include <stdint.h>

typedef enum IRQn {
    /* Safety Island CL0 SPI */
    CL0_SYSTEM_TIMER_IRQ = 34,
    CL0_MHU3_AP2SI0_S_IRQ = 99,
    CL0_MHU3_RSE2SI0_IRQ = 105,
    CL0_FMU_CRITICAL = 128,
    CL0_FMU_NON_CRITICAL = 129,

    IRQn_MAX = INT16_MAX,
} IRQn_Type;

#endif /* SI0_IRQ_H */
