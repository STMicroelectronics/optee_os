/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Configuration data for module 'timer'.
 */

#include <fvp_baser_cfgd_timer.h>
#include <fvp_baser_irq.h>

#include <mod_timer.h>

#include <fwk_module.h>

static const struct fwk_element timer_elements[] = {
    {
        .name = "TIMER0",
        .data = &((struct mod_timer_dev_config){
            .id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_GTIMER, 0),
            .timer_irq = FVP_SYSTEM_TIMER_IRQ,
        }),
        .sub_element_count = FVP_BASE_R_TIMER_ALARM_IDX_COUNT,
    },
    { 0 }
};

struct fwk_module_config config_timer = {
    .elements = FWK_MODULE_STATIC_ELEMENTS_PTR(timer_elements),
};
