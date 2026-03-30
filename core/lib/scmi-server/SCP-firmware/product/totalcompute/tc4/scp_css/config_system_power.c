/*
 * Arm SCP/MCP Software
 * Copyright (c) 2023-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "tc4_ppu_v1.h"

#include <mod_mock_ppu.h>
#include <mod_power_domain.h>
#include <mod_ppu_v1.h>
#include <mod_system_power.h>
#include <mod_tc_system.h>

#include <fwk_element.h>
#include <fwk_id.h>
#include <fwk_macros.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>

#include <fmw_cmsis.h>

#include <stdint.h>

static const uint8_t
    system_power_to_sys_ppu0_state[MOD_SYSTEM_POWER_POWER_STATE_COUNT] = {
        [MOD_PD_STATE_ON] = (uint8_t)MOD_PD_STATE_ON,
        [MOD_SYSTEM_POWER_POWER_STATE_SLEEP0] = (uint8_t)MOD_PD_STATE_OFF,
        [MOD_PD_STATE_OFF] = (uint8_t)MOD_PD_STATE_OFF,
    };

static struct fwk_element system_power_element_table[] = {
    [0] =
        {
            .name = "SYS-PPU-0",
            .data = &((struct mod_system_power_dev_config){
                .sys_ppu_id = FWK_ID_ELEMENT_INIT(
                    FWK_MODULE_IDX_PPU_V1,
                    PPU_V1_ELEMENT_IDX_SYSTOP0),
                .api_id = FWK_ID_API_INIT(
                    FWK_MODULE_IDX_PPU_V1,
                    MOD_PPU_V1_API_IDX_POWER_DOMAIN_DRIVER),
                .sys_state_table = system_power_to_sys_ppu0_state,
            }),
        },
    [1] =
        {
            .name = "SYS-PPU-1",
            .data = &((struct mod_system_power_dev_config){
                .sys_ppu_id = FWK_ID_ELEMENT_INIT(
                    FWK_MODULE_IDX_PPU_V1,
                    PPU_V1_ELEMENT_IDX_SYSTOP1),
                .api_id = FWK_ID_API_INIT(
                    FWK_MODULE_IDX_PPU_V1,
                    MOD_PPU_V1_API_IDX_POWER_DOMAIN_DRIVER),
                .sys_state_table = system_power_to_sys_ppu0_state,
            }),
        },
    { 0 }, /* Termination description */
};

static struct mod_system_power_config system_power_config = {
    .soc_wakeup_irq = SOC_WAKEUP0_IRQ,

    /* System driver */
    .driver_id = FWK_ID_MODULE_INIT(FWK_MODULE_IDX_TC_SYSTEM),
    .driver_api_id = FWK_ID_API_INIT(
        FWK_MODULE_IDX_TC_SYSTEM,
        MOD_TC_SYSTEM_API_IDX_SYSTEM_POWER_DRIVER),

    /* Initial system state */
    .initial_system_power_state = MOD_PD_STATE_OFF,
};

static const struct fwk_element *tc_system_get_element_table(fwk_id_t unused)
{
    return system_power_element_table;
}

const struct fwk_module_config config_system_power = {
    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(tc_system_get_element_table),
    .data = &system_power_config,
};
