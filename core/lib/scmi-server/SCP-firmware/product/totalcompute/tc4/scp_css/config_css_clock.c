/*
 * Arm SCP/MCP Software
 * Copyright (c) 2023-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "clock_soc.h"

#include <mod_css_clock.h>
#include <mod_pik_clock.h>
#include <mod_system_pll.h>

#include <fwk_element.h>
#include <fwk_id.h>
#include <fwk_macros.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>

static const fwk_id_t member_table_dpu[1] = {
    FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_PIK_CLOCK, CLOCK_PIK_IDX_DPU),
};

static const struct fwk_element css_clock_element_table[
    CLOCK_CSS_IDX_COUNT + 1] = {
    [CLOCK_CSS_IDX_DPU] =
        {
            .name = "DPU",
            .data = &((struct mod_css_clock_dev_config){
                .clock_type = MOD_CSS_CLOCK_TYPE_NON_INDEXED,
                .clock_default_source =
                    MOD_PIK_CLOCK_ACLKDPU_SOURCE_DISPLAYPLLCLK,
                .clock_switching_source =
                    MOD_PIK_CLOCK_ACLKDPU_SOURCE_SYSREFCLK,
                .pll_id = FWK_ID_ELEMENT_INIT(
                    FWK_MODULE_IDX_SYSTEM_PLL,
                    CLOCK_PLL_IDX_DPU),
                .pll_api_id = FWK_ID_API_INIT(
                    FWK_MODULE_IDX_SYSTEM_PLL,
                    MOD_SYSTEM_PLL_API_TYPE_DEFAULT),
                .member_table = member_table_dpu,
                .member_count = FWK_ARRAY_SIZE(member_table_dpu),
                .member_api_id = FWK_ID_API_INIT(
                    FWK_MODULE_IDX_PIK_CLOCK,
                    MOD_PIK_CLOCK_API_TYPE_CSS),
                .initial_rate = 600 * FWK_MHZ,
                .modulation_supported = false,
            }),
        },
    [CLOCK_CSS_IDX_COUNT] = { 0 }, /* Termination description. */
};

static const struct fwk_element *css_clock_get_element_table(fwk_id_t module_id)
{
    return css_clock_element_table;
}

const struct fwk_module_config config_css_clock = {
    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(css_clock_get_element_table),
};
