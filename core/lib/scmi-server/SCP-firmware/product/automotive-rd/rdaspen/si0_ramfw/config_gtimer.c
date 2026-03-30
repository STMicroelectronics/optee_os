/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Configuration data for module 'gtimer'.
 */

#include "si0_mmap.h"

#include <mod_gtimer.h>

#include <fwk_element.h>
#include <fwk_id.h>
#include <fwk_macros.h>
#include <fwk_module.h>
#include <fwk_time.h>

/* REF_CLK input clock speed */
#define CLOCK_RATE_REFCLK (125UL * FWK_MHZ)

/*
 * System Counter per-tick increment value required for 1GHz clock speed
 * (1GHz / CLOCK_RATE_REFCLK) = 8.
 */
#define SYSCNT_INCR 8

/*
 * Offsets of the system counter implementation defined registers.
 */
#define SYSCNT_IMPDEF0_CNTENCR 0xC0
#define SYSCNT_IMPDEF0_CNTINCR 0xD0

/*
 * System counter implementation defined register config data.
 */
static struct mod_gtimer_syscounter_impdef_config syscnt_impdef_cfg[] = {
    {
        .offset = SYSCNT_IMPDEF0_CNTENCR,
        .value = 0,
    },
    {
        .offset = SYSCNT_IMPDEF0_CNTINCR,
        .value = SYSCNT_INCR,
    }
};

/* Generic timer driver config */
static const struct fwk_element gtimer_dev_table[] = {
    [0] = { .name = "REFCLK",
            .data = &((struct mod_gtimer_dev_config){
                .hw_timer = SI0_REFCLK_CNT_BASE,
                .hw_counter = SI0_REFCLK_CNTCTL_BASE,
                .control = SI0_REFCLK_CNTCONTROL_BASE,
                .frequency = CLOCK_RATE_REFCLK,
                .clock_id = FWK_ID_NONE_INIT,
                .syscnt_impdef_cfg = syscnt_impdef_cfg,
                .syscnt_impdef_cfg_cnt = FWK_ARRAY_SIZE(syscnt_impdef_cfg),
            }) },
    [1] = { .name = "SI0_TIMER",
            .data = &((struct mod_gtimer_dev_config){
                .hw_timer = SI0_TIMER_HW_TIMER,
                .hw_counter = SI0_TIMER_HW_COUNTER,
                .skip_cntcontrol_init = true,
                .frequency = CLOCK_RATE_REFCLK,
                .clock_id = FWK_ID_NONE_INIT,
            }) },
    [2] = { 0 },
};

const struct fwk_module_config config_gtimer = {
    .elements = FWK_MODULE_STATIC_ELEMENTS_PTR(gtimer_dev_table),
};

struct fwk_time_driver fmw_time_driver(const void **ctx)
{
    return mod_gtimer_driver(ctx, config_gtimer.elements.table[0].data);
}
