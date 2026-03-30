/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fvp_baser_aemv8r_mmap.h"
#include "fvp_baser_irq.h"

#include <mod_sp805.h>

#include <fwk_interrupt.h>
#include <fwk_module.h>

/* Reload value for the WdogLoad register */
#define LOAD_VALUE 0x0fffffff

/*
 * Watch Dog Timer Driver configuration
 */
const struct fwk_module_config config_sp805 = {
    .data =
        &(struct mod_sp805_config){
            .reg_base = FVP_WATCHDOG_BASE,
            .wdt_load_value = LOAD_VALUE,
            .driver_id = FWK_ID_NONE_INIT,
            .sp805_irq = FVP_WATCHDOG_IRQ,
        }
};
