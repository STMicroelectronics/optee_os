/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Configuration data for module 'atu'.
 */

#include "css_common.h"
#include "mcp_css_mmap.h"

#include <mod_atu.h>

#include <fwk_element.h>
#include <fwk_id.h>
#include <fwk_log.h>
#include <fwk_macros.h>
#include <fwk_module.h>

/* Indices for ATU module elements */
enum cfgd_mod_atu_element_idx {
    CFGD_MOD_ATU_EIDX_ATU0,
    CFGD_MOD_ATU_EIDX_COUNT
};

#define MOD_MCP_ATU_ELEMENT_COUNT (CFGD_MOD_ATU_EIDX_COUNT + 1)

/* Indices for translation regions to be configured in the ATU */
enum atu_regions_idx {
    /* ATU region to access shared RSM SRAM */
    ATU_REGION_IDX_SHARED_SRAM_RSM,
    /* ATU region count */
    ATU_REGION_IDX_COUNT,
};

static struct atu_region_map atu_regions[ATU_REGION_IDX_COUNT] = {
    [ATU_REGION_IDX_SHARED_SRAM_RSM] = {
        .region_owner_id = FWK_ID_MODULE_INIT(FWK_MODULE_IDX_MCP_PLATFORM),
        .log_addr_base = MCP_SHARED_SRAM_RSM_BASE,
        .phy_addr_base = RSM_SHARED_SRAM_BASE,
        .region_size = RSM_SHARED_SRAM_SIZE,
        .attributes = ATU_ENCODE_ATTRIBUTES_SECURE_PAS,
    },
};

static const struct fwk_element element_table[MOD_MCP_ATU_ELEMENT_COUNT] = {
    [CFGD_MOD_ATU_EIDX_ATU0] = {
        .name = "MCP_ATU",
        .data = &(struct mod_atu_device_config) {
            .is_atu_delegated = false,
            .atu_base = MCP_ATU_BASE,
            .atu_region_config_table = atu_regions,
            .atu_region_count = FWK_ARRAY_SIZE(atu_regions),
        },
    },
    [CFGD_MOD_ATU_EIDX_COUNT] = { 0 },
};

static const struct fwk_element *get_element_table(fwk_id_t module_id)
{
    return element_table;
}

struct fwk_module_config config_atu = {
    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(get_element_table),
};
