/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOD_SYSTEM_PIK_H
#define MOD_SYSTEM_PIK_H

#include "system_pik.h"

#include <stdint.h>

#define TCU_L0GPTSZ_SHIFT 2UL
#define L0GPTSZ_16GIB     0x04UL

struct mod_system_pik_device_config {
    /*! Base address of the system PIK register */
    uintptr_t system_pik_base;
    /*! Level 0 Granule Protection Table size */
    uint32_t l0_gpt_size;
};

#endif /* MOD_SYSTEM_PIK_H */
