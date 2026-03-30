/*
 * Arm SCP/MCP Software
 * Copyright (c) 2023-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config_power_domain.h"
#include "scp_mmap.h"
#include "tc4_core.h"
#include "tc4_ppu_v1.h"

#include <mod_power_domain.h>
#include <mod_ppu_v1.h>
#include <mod_tc4_bl1.h>

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

#define SCP_PPU_SYS0_OPMODE (0)
#define SCP_PPU_SYS1_OPMODE (4)

#define CORE_PPU_ELEMENT_INIT(_core_num) \
    [PPU_V1_ELEMENT_IDX_CORE##_core_num] = { \
        .name = "CORE" #_core_num, \
        .data = &((struct mod_ppu_v1_pd_config){ \
            .pd_type = MOD_PD_TYPE_CORE, \
            .ppu.reg_base = SCP_PPU_CORE_BASE(_core_num), \
            .ppu.irq = FWK_INTERRUPT_NONE, \
            .cluster_id = FWK_ID_ELEMENT_INIT( \
                FWK_MODULE_IDX_PPU_V1, PPU_V1_ELEMENT_IDX_CLUSTER0), \
            .observer_id = FWK_ID_NONE_INIT, \
        }), \
    }

#define CME_PPU_ELEMENT_INIT(_cme_num) \
    [PPU_V1_ELEMENT_IDX_CME##_cme_num] = { \
        .name = "CME" #_cme_num, \
        .data = &((struct mod_ppu_v1_pd_config){ \
            .pd_type = MOD_PD_TYPE_DEVICE, \
            .ppu.reg_base = SCP_PPU_CME_BASE(_cme_num), \
            .ppu.irq = FWK_INTERRUPT_NONE, \
            .cluster_id = FWK_ID_ELEMENT_INIT( \
                FWK_MODULE_IDX_PPU_V1, PPU_V1_ELEMENT_IDX_CLUSTER0), \
            .observer_id = FWK_ID_NONE_INIT, \
        }), \
    }

#define CLUSTER_PPU_ELEMENT_INIT(_cluster_num) \
    [PPU_V1_ELEMENT_IDX_CLUSTER##_cluster_num] = { \
        .name = "CLUSTER" #_cluster_num, \
        .data = &((struct mod_ppu_v1_pd_config){ \
            .pd_type = MOD_PD_TYPE_CLUSTER, \
            .ppu.reg_base = SCP_PPU_CLUSTER_BASE, \
            .ppu.irq = FWK_INTERRUPT_NONE, \
            .observer_id = FWK_ID_NONE_INIT, \
            .opmode = 7, \
        }), \
    }

#define SYSTOP_PPU_ELEMENT_INIT(_systop_num) \
    [PPU_V1_ELEMENT_IDX_SYSTOP##_systop_num] = { \
        .name = "SYSTOP" #_systop_num, \
        .data = &((struct mod_ppu_v1_pd_config){ \
            .pd_type = MOD_PD_TYPE_SYSTEM, \
            .ppu.reg_base = SCP_PPU_SYS##_systop_num##_BASE, \
            .observer_id = FWK_ID_NONE_INIT, \
            .ppu.irq = FWK_INTERRUPT_NONE, \
            /* Force the PPU module to power this on during*/ \
            /* the initialisation stage, this is required */ \
            /* before accessing any registers external to */ \
            /* the SCP */ \
            .default_power_on = true, \
            .opmode = SCP_PPU_SYS##_systop_num##_OPMODE, \
        }), \
    }

static const struct fwk_element ppu_v1_element_table[] = {
    TC4_FOR_EACH_CORE(CORE_PPU_ELEMENT_INIT),
    TC4_FOR_EACH_CLUSTER(CLUSTER_PPU_ELEMENT_INIT),
#if !defined(PLAT_FVP)
    TC4_FOR_EACH_CME(CME_PPU_ELEMENT_INIT),
#endif
    PPU_V1_FOR_EACH_SYSTOP(SYSTOP_PPU_ELEMENT_INIT),
    { 0 }
};

/* Module configuration data */
static struct mod_ppu_v1_config ppu_v1_config_data = {
    .pd_notification_id = FWK_ID_NOTIFICATION_INIT(
        FWK_MODULE_IDX_TC4_BL1,
        MOD_TC4_BL1_NOTIFICATION_IDX_POWER_SYSTOP),
    .pd_source_id = FWK_ID_MODULE_INIT(FWK_MODULE_IDX_TC4_BL1),
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
