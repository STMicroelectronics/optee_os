/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Configuration data for module 'atu_mmio'.
 */

#include "scp_mmap.h"

#include <mod_atu.h>
#include <mod_atu_mmio.h>

#include <fwk_id.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>

static struct mod_atu_mmio_config atu_mmio_module_config = {
    .window_address = SCP_ATW0_ATU_MMIO_BASE,
    .map_size = SCP_ATW0_ATU_MMIO_SIZE,
    .atu_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_ATU, 0),
    .atu_api_id = FWK_ID_API(FWK_MODULE_IDX_ATU, MOD_ATU_API_IDX_ATU),
};

struct fwk_module_config config_atu_mmio = {
    .data = &atu_mmio_module_config,
};
