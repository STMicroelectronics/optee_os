/*
 * Arm SCP/MCP Software
 * Copyright (c) 2023-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config_power_domain.h"
#include "scp_mmap.h"
#include "tc_core.h"
#include "tc_ppu_v1.h"

#include <mod_power_domain.h>
#include <mod_ppu_v1.h>

#include <fwk_element.h>
#include <fwk_id.h>
#include <fwk_interrupt.h>
#include <fwk_macros.h>
#include <fwk_mm.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>
#include <fwk_string.h>

#include <stdio.h>
#include <string.h>

#define PPU_CLUSTER_ELEMENT_IDX (PPU_V1_ELEMENT_IDX_CLUSTER0)
#define CORE_PPU_ELEMENT_INIT_WITH_IDX_AND_CLUSTER(_core_num) \
    CORE_PPU_ELEMENT_INIT_WITH_IDX(_core_num, PPU_CLUSTER_ELEMENT_IDX)

static const struct fwk_element ppu_v1_element_table[] = {
    TC_FOR_EACH_CORE(CORE_PPU_ELEMENT_INIT_WITH_IDX_AND_CLUSTER),
    TC_FOR_EACH_CLUSTER(CLUSTER_PPU_ELEMENT_INIT_WITH_IDX),
    { 0 }
};

/* Module configuration data */
static struct mod_ppu_v1_config ppu_v1_config_data = {
    .pd_notification_id = FWK_ID_NOTIFICATION_INIT(
        FWK_MODULE_IDX_POWER_DOMAIN,
        MOD_PD_NOTIFICATION_IDX_POWER_STATE_TRANSITION),
    .pd_source_id = FWK_ID_ELEMENT_INIT(
        FWK_MODULE_IDX_POWER_DOMAIN,
        TC_NUMBER_OF_CORES + TC_NUMBER_OF_CLUSTERS +
        PD_STATIC_DEV_IDX_SYSTOP),
};

static const struct fwk_element *ppu_v1_get_element_table(fwk_id_t module_id)
{
    return ppu_v1_element_table;
}

/*
 * Power module configuration data
 */
const struct fwk_module_config config_ppu_v1 = {
    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(ppu_v1_get_element_table),
    .data = &ppu_v1_config_data,
};
