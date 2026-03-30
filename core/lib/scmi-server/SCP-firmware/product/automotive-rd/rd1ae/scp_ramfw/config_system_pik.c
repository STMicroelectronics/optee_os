/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Configuration data for module 'system_pik'.
 */

#include <mod_system_pik.h>

#include <fwk_module.h>

#include <stdint.h>

static const struct mod_system_pik_device_config system_pik_data = {
    .system_pik_base = (uintptr_t)SYSTEM_PIK_PTR,
    .l0_gpt_size = L0GPTSZ_16GIB,
};

struct fwk_module_config config_system_pik = {
    .data = &system_pik_data,
};
