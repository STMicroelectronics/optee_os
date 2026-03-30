/*
 * Arm SCP/MCP Software
 * Copyright (c) 2023-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <config_common_bootloader.h>

GENERATE_BOOTLOADER_CFG(SCP_RUNTIME_BASE, SCP_RUNTIME_SIZE)

struct fwk_module_config config_bootloader = {
    .data = &bootloader_module_config,
};
