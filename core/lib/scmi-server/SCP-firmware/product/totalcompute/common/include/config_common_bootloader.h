/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef CONFIG_COMMON_BOOTLOADER_H
#define CONFIG_COMMON_BOOTLOADER_H

#include "scp_mmap.h"
#include "tc_sds.h"

#include <mod_bootloader.h>

#include <fwk_module.h>

#define GENERATE_BOOTLOADER_CFG(_dest_base, _dest_size) \
    static const struct mod_bootloader_config bootloader_module_config = { \
        .source_base = SCP_TRUSTED_RAM_BASE, \
        .source_size = 512 * 1024, \
        .destination_base = _dest_base, \
        .destination_size = _dest_size, \
        .sds_struct_id = TC_SDS_BOOTLOADER, \
    };

#endif /* CONFIG_COMMON_BOOTLOADER_H */
