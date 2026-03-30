/*
 * Arm SCP/MCP Software
 * Copyright (c) 2023-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <mod_atu.h>
#ifdef BUILD_HAS_ATU_DELEGATE
#    include "scp_platform_transport.h"

#    include <mod_transport.h>
#endif

#include <fwk_element.h>
#include <fwk_id.h>
#include <fwk_macros.h>
#include <fwk_module.h>

#ifdef BUILD_HAS_ATU_MANAGE
#    include "scp_mmap.h"
#    include "tc3_atu.h"

#    include <fmw_cmsis.h>

static const struct atu_region_map atu_regions[] = {
    {
        .region_owner_id = FWK_ID_MODULE_INIT(FWK_MODULE_IDX_ATU),
        .log_addr_base = SCP_ATU_LOG_ADDR_REFCLK,
        .phy_addr_base = SCP_ATU_PHY_ADDR_REFCLK,
        .region_size = SCP_ATU_REG_SIZE_REFCLK,
        .attributes = ATU_ENCODE_ATTRIBUTES_SECURE_PAS,
    },
    {
        .region_owner_id = FWK_ID_MODULE_INIT(FWK_MODULE_IDX_ATU),
        .log_addr_base = SCP_ATU_LOG_ADDR_REFCLK_AP,
        .phy_addr_base = SCP_ATU_PHY_ADDR_REFCLK_AP,
        .region_size = SCP_ATU_REG_SIZE_REFCLK_AP,
        .attributes = ATU_ENCODE_ATTRIBUTES_SECURE_PAS,
    },
    {
        .region_owner_id = FWK_ID_MODULE_INIT(FWK_MODULE_IDX_ATU),
        .log_addr_base = SCP_ATU_LOG_ADDR_UART,
        .phy_addr_base = SCP_ATU_PHY_ADDR_UART,
        .region_size = SCP_ATU_REG_SIZE_UART,
        .attributes = ATU_ENCODE_ATTRIBUTES_SECURE_PAS,
    },
    {
        .region_owner_id = FWK_ID_MODULE_INIT(FWK_MODULE_IDX_ATU),
        .log_addr_base = SCP_ATU_LOG_ADDR_CLUSTER_UTIL,
        .phy_addr_base = SCP_ATU_PHY_ADDR_CLUSTER_UTIL,
        .region_size = SCP_ATU_REG_SIZE_CLUSTER_UTIL,
        .attributes = ATU_ENCODE_ATTRIBUTES_SECURE_PAS,
    },
    {
        .region_owner_id = FWK_ID_MODULE_INIT(FWK_MODULE_IDX_ATU),
        .log_addr_base = SCP_ATU_LOG_ADDR_SHARED_SRAM,
        .phy_addr_base = SCP_ATU_PHY_ADDR_SHARED_SRAM,
        .region_size = SCP_ATU_REG_SIZE_SHARED_SRAM,
        .attributes = ATU_ENCODE_ATTRIBUTES_SECURE_PAS,
    },
    {
        .region_owner_id = FWK_ID_MODULE_INIT(FWK_MODULE_IDX_ATU),
        .log_addr_base = SCP_ATU_LOG_ADDR_SHARED_NSRAM,
        .phy_addr_base = SCP_ATU_PHY_ADDR_SHARED_NSRAM,
        .region_size = SCP_ATU_REG_SIZE_SHARED_NSRAM,
        .attributes = ATU_ENCODE_ATTRIBUTES_NON_SECURE_PAS,
    },
    {
        .region_owner_id = FWK_ID_MODULE_INIT(FWK_MODULE_IDX_ATU),
        .log_addr_base = SCP_ATU_LOG_ADDR_ROS_SOC_BASE,
        .phy_addr_base = SCP_ATU_PHY_ADDR_ROS_SOC,
        .region_size = SCP_ATU_REG_SIZE_ROS_SOC,
        .attributes = ATU_ENCODE_ATTRIBUTES_SECURE_PAS,
    },
    {
        .region_owner_id = FWK_ID_MODULE_INIT(FWK_MODULE_IDX_ATU),
        .log_addr_base = SCP_ATU_LOG_ADDR_PLL,
        .phy_addr_base = SCP_ATU_PHY_ADDR_PLL,
        .region_size = SCP_ATU_REG_SIZE_PLL,
        .attributes = ATU_ENCODE_ATTRIBUTES_SECURE_PAS,
    },
};
#endif

static const struct fwk_element element_table[2] = {
    [0] = {
            .name = "SCP_ATU",
            .data = &(struct mod_atu_device_config) {
#ifdef BUILD_HAS_ATU_MANAGE
                .is_atu_delegated = false,
                .atu_base = SCP_ATU_BASE,
                .atu_region_config_table = atu_regions,
                .atu_region_count = FWK_ARRAY_SIZE(atu_regions),
#endif
            },
        },
        [1] = { 0 },
};

static const struct fwk_element *get_element_table(fwk_id_t module_id)
{
    return element_table;
}

struct fwk_module_config config_atu = {
    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(get_element_table),
};
