/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Configuration data for module 'timer'.
 */

#include "si0_cfgd_timer.h"
#include "si0_irq.h"

#include <mod_timer.h>

#include <fwk_element.h>
#include <fwk_id.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>

/* Timer HAL config */
static const struct fwk_element timer_dev_table[] = {
    [0] = {
        .name = "REFCLK",
        .data = &((struct mod_timer_dev_config) {
            .id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_GTIMER, 0),
        }),
        .sub_element_count =
            SI0_CFGD_MOD_TIMER_REFCLK_ALARM_IDX_COUNT, /* Number of alarms */
    },
    [1] = {
        .name = "SI0_TIMER",
        .data = &((struct mod_timer_dev_config) {
            .id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_GTIMER, 1),
            .timer_irq = CL0_SYSTEM_TIMER_IRQ,
        }),
        .sub_element_count =
            SI0_CFGD_MOD_TIMER_SI0_TIMER_ALARM_IDX_COUNT, /* Number of alarms */
    },
    [2] = { 0 },
};

const struct fwk_module_config config_timer = {
    .elements = FWK_MODULE_STATIC_ELEMENTS_PTR(timer_dev_table),
};
