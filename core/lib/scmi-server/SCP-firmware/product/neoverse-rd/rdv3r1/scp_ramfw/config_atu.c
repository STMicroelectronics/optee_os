/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Configuration data for module 'atu'.
 */

#include "scp_atw0_mmap.h"
#include "scp_atw1_mmap.h"
#include "scp_css_mmap.h"

#include <mod_atu.h>
#include <mod_sid.h>

#include <fwk_element.h>
#include <fwk_id.h>
#include <fwk_macros.h>
#include <fwk_module.h>

/* Peripheral address space size per chip */
#define CHIP_ADDR_SIZE (64ULL * FWK_GIB)
/* Return peripheral address space offset based on chip id */
#define CHIP_ADDR_OFFSET(chip_id) (chip_id * CHIP_ADDR_SIZE)

/* Indices for ATU module elements */
enum cfgd_mod_atu_element_idx {
    CFGD_MOD_ATU_EIDX_ATU0,
    CFGD_MOD_ATU_EIDX_COUNT
};

/* Indices for translation regions to be configured in the ATU */
enum atu_regions_idx {
    /* ATU region to access CMN CFGM */
    ATU_REGION_IDX_CMN = 0,
    /* ATU region to access AP Cluster Utility space */
    ATU_REGION_IDX_CLUSTER_UTIL,
    /* ATU region to access LCP Cluster space */
    ATU_REGION_IDX_CLUSTER_LCP,
    /* ATU region to access AP shared SRAM */
    ATU_REGION_IDX_SHARED_SRAM,
    /* ATU region count */
    ATU_REGION_IDX_COUNT,
};

struct atu_region_map atu_regions[ATU_REGION_IDX_COUNT] = {
    [ATU_REGION_IDX_CMN] = {
        .region_owner_id = FWK_ID_MODULE_INIT(FWK_MODULE_IDX_SCP_PLATFORM),
        .log_addr_base = SCP_ATW1_CMN_BASE,
        .phy_addr_base = 0x100000000,
        .region_size = SCP_ATW1_CMN_SIZE,
        .attributes = ATU_ENCODE_ATTRIBUTES_ROOT_PAS,
    },
    [ATU_REGION_IDX_CLUSTER_UTIL] = {
        .region_owner_id = FWK_ID_MODULE_INIT(FWK_MODULE_IDX_SCP_PLATFORM),
        .log_addr_base = SCP_ATW0_CU_AP_PERIPH_REGION_BASE,
        .phy_addr_base = 0x140000000,
        .region_size = SCP_ATW0_CU_AP_PERIPH_REGION_SIZE,
        .attributes = ATU_ENCODE_ATTRIBUTES_ROOT_PAS,
    },
    [ATU_REGION_IDX_CLUSTER_LCP] = {
        .region_owner_id = FWK_ID_MODULE_INIT(FWK_MODULE_IDX_SCP_PLATFORM),
        .log_addr_base = SCP_ATW0_CU_LCP_PERIPH_REGION_BASE,
        .phy_addr_base = 0x160000000,
        .region_size = SCP_ATW0_CU_LCP_PERIPH_REGION_SIZE,
        .attributes = ATU_ENCODE_ATTRIBUTES_ROOT_PAS,
    },
    [ATU_REGION_IDX_SHARED_SRAM] = {
        .region_owner_id = FWK_ID_MODULE_INIT(FWK_MODULE_IDX_SCP_PLATFORM),
        .log_addr_base = SCP_ATW0_AP_PERIPHERAL_SRAM_BASE,
        .phy_addr_base = 0x00000000,
        .region_size = SCP_ATW0_AP_PERIPHERAL_SRAM_SIZE,
        .attributes = ATU_ENCODE_ATTRIBUTES_ROOT_PAS,
    },
};

static const struct fwk_element element_table[] = {
    [CFGD_MOD_ATU_EIDX_ATU0] = {
        .name = "SCP_ATU",
        .data = &(struct mod_atu_device_config) {
            .is_atu_delegated = false,
            .atu_base = SCP_ATU_BASE,
            .atu_region_config_table = atu_regions,
            .atu_region_count = FWK_ARRAY_SIZE(atu_regions),
        },
    },
    [CFGD_MOD_ATU_EIDX_COUNT] = { 0 },
};

static const struct fwk_element *get_element_table(fwk_id_t module_id)
{
    int status;
    uint8_t i;
    uint8_t chip_id;
    const struct mod_sid_info *system_info;

    /* Get system info from the SID driver */
    status = mod_sid_get_system_info(&system_info);
    if (status != FWK_SUCCESS) {
        fwk_trap();
    }

    /* Node Number indicates the chip id in a multichip system */
    chip_id = system_info->node_number;

    /*
     * For each ATU region, add chip address space offset to the physical
     * address, based on the current chip id.
     */
    for (i = 0; i < FWK_ARRAY_SIZE(atu_regions); i++) {
        atu_regions[i].phy_addr_base += CHIP_ADDR_OFFSET(chip_id);
    }

    return element_table;
}

struct fwk_module_config config_atu = {
    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(get_element_table),
};
