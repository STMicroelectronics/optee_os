/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Configuration data for module 'gtimer'.
 */

#include <fvp_baser_aemv8r_mmap.h>

#include <mod_gtimer.h>

#include <fwk_element.h>
#include <fwk_id.h>
#include <fwk_module.h>
#include <fwk_time.h>

#define GTIMER_FREQ_HZ (100UL * FWK_MHZ)

static const struct fwk_element gtimer_dev_table[] = {
    [0] = {
        .name = "CLI_TIMER",
        .data = &((struct mod_gtimer_dev_config){
            .hw_timer = FVP_HW_TIMER_BASE,
            .hw_counter = FVP_HW_COUNTER_BASE,
            .control = FVP_CNTCONTROL_BASE,
            .frequency = GTIMER_FREQ_HZ,
            .clock_id = FWK_ID_NONE_INIT,
        }),
    },
    { 0 }
};

struct fwk_module_config config_gtimer = {
    .elements = FWK_MODULE_STATIC_ELEMENTS_PTR(gtimer_dev_table),
};

struct fwk_time_driver fwk_time_driver(const void **ctx)
{
    return mod_gtimer_driver(ctx, config_gtimer.elements.table[0].data);
}
