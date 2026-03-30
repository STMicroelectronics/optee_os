/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Configuration data for module 'safety_island_platform'.
 */

#include "scp_cfgd_transport.h"

#include <mod_safety_island_platform.h>

#include <fwk_id.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>

/* Safety Island Cluster 0 layout */
#define SI_CL0_ID       0U
#define SI_CL0_CORE_NUM 1U
#define SI_CL0_CORE_OFS SI_CL0_CORE_NUM

/* Safety Island Cluster 1 layout */
#define SI_CL1_ID       SI_CL0_ID + 1U
#define SI_CL1_CORE_NUM 2U
#define SI_CL1_CORE_OFS SI_CL0_CORE_NUM + SI_CL1_CORE_NUM

/* Safety Island Cluster 2 layout */
#define SI_CL2_ID       SI_CL1_ID + 1U
#define SI_CL2_CORE_NUM 4U
#define SI_CL2_CORE_OFS SI_CL1_CORE_NUM + SI_CL2_CORE_NUM

enum safety_island_cluster_idx {
    SI_CL0_IDX,
    SI_CL1_IDX,
    SI_CL2_IDX,
    SI_CL_COUNT,
};

static const struct fwk_element
    safety_island_platform_element_table[SI_CL_COUNT + 1] = {
    [SI_CL0_IDX] = {
        .name = "Safety Island Cluster 0",
        .data = &((struct safety_island_cluster_config) {
            .transport_id = FWK_ID_ELEMENT_INIT(
                FWK_MODULE_IDX_TRANSPORT,
                SCP_CFGD_MOD_TRANSPORT_EIDX_BOOT_SI_CLUS0),
            .cluster_layout = {SI_CL0_ID, SI_CL0_CORE_NUM, SI_CL0_CORE_OFS},
        }),
    },
    [SI_CL1_IDX] = {
        .name = "Safety Island Cluster 1",
        .data = &((struct safety_island_cluster_config) {
            .transport_id = FWK_ID_ELEMENT_INIT(
                FWK_MODULE_IDX_TRANSPORT,
                SCP_CFGD_MOD_TRANSPORT_EIDX_BOOT_SI_CLUS1),
            .cluster_layout = {SI_CL1_ID, SI_CL1_CORE_NUM, SI_CL1_CORE_OFS},
        }),
    },
    [SI_CL2_IDX] = {
        .name = "Safety Island Cluster 2",
        .data = &((struct safety_island_cluster_config) {
            .transport_id = FWK_ID_ELEMENT_INIT(
                FWK_MODULE_IDX_TRANSPORT,
                SCP_CFGD_MOD_TRANSPORT_EIDX_BOOT_SI_CLUS2),
            .cluster_layout = {SI_CL2_ID, SI_CL2_CORE_NUM, SI_CL2_CORE_OFS},
        }),
    },
    [SI_CL_COUNT] = { 0 },
};

static const struct fwk_element *get_safety_island_platform_element_table(
    fwk_id_t module_id)
{
    return safety_island_platform_element_table;
}

const struct fwk_module_config config_safety_island_platform = {
    .elements =
        FWK_MODULE_DYNAMIC_ELEMENTS(get_safety_island_platform_element_table),
};
