/*
 * Arm SCP/MCP Software
 * Copyright (c) 2023-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config_power_domain.h"
#include "tc_ppu_v1.h"
#include "scp_mmap.h"

#include <mod_power_domain.h>
#include <mod_ppu_v1.h>
#include <mod_tc3_bl1.h>

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

#define PPU_CLUSTER_ELEMENT_IDX (1)

/* SCP BL1 only considers boot core and cluster */
static const struct fwk_element ppu_v1_element_table[] = {
    CORE_PPU_ELEMENT_INIT(0, PPU_CLUSTER_ELEMENT_IDX),
    CLUSTER_PPU_ELEMENT_INIT(0),
    { 0 }
};

static const struct fwk_element *tc_ppu_v1_get_element_table(
    fwk_id_t module_id)
{
   return ppu_v1_element_table;
}

/*
 * Power module configuration data
 */
struct fwk_module_config config_ppu_v1 = {
    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(tc_ppu_v1_get_element_table),
    .data =
        &(struct mod_ppu_v1_config){
            .pd_notification_id = FWK_ID_NOTIFICATION_INIT(
                FWK_MODULE_IDX_TC3_BL1,
                MOD_TC3_BL1_NOTIFICATION_IDX_POWER_SYSTOP),
            .pd_source_id = FWK_ID_MODULE_INIT(FWK_MODULE_IDX_TC3_BL1),
        },
};
