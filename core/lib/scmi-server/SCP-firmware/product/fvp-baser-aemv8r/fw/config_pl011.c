/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fvp_baser_aemv8r_mmap.h"

#include <mod_pl011.h>

#include <fwk_element.h>
#include <fwk_id.h>
#include <fwk_macros.h>
#include <fwk_module.h>

static const struct fwk_element pl011_table[] = {
    {
        .name = "scp_uart",
        .data =
            &(struct mod_pl011_element_cfg){
                .reg_base = FVP_UART_BASE,
                .baud_rate_bps = 115200,
                .clock_rate_hz = 24 * FWK_MHZ,
            },
    },
    { 0 },
};

const struct fwk_module_config config_pl011 = {
    .elements = FWK_MODULE_STATIC_ELEMENTS_PTR(pl011_table),
};
