/*
 * Arm SCP/MCP Software
 * Copyright (c) 2015-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "juno_clock.h"
#include "juno_scmi.h"

#include <mod_scmi_clock.h>

#include <fwk_id.h>
#include <fwk_macros.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>

static const struct mod_scmi_clock_device agent_device_table_ospm[] = {
    {
        /* Big */
        .element_id =
            FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_CLOCK, JUNO_CLOCK_IDX_BIGCLK),
        .starts_enabled = true,
    },
    {
        /* Little */
        .element_id =
            FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_CLOCK, JUNO_CLOCK_IDX_LITTLECLK),
        .starts_enabled = true,
    },
    {
        /* GPU */
        .element_id =
            FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_CLOCK, JUNO_CLOCK_IDX_GPUCLK),
        .starts_enabled = true,
    },
    {
        /* HDLCD0 */
        .element_id =
            FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_CLOCK, JUNO_CLOCK_IDX_HDLCD0),
        .starts_enabled = true,
    },
    {
        /* HDLCD1 */
        .element_id =
            FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_CLOCK, JUNO_CLOCK_IDX_HDLCD1),
        .starts_enabled = true,
    },
    {
        /* I2S */
        .element_id =
            FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_CLOCK, JUNO_CLOCK_IDX_I2SCLK),
        .starts_enabled = true,
    },
};

static const struct mod_scmi_clock_agent_config ospm_config = {
    .device_table = agent_device_table_ospm,
    .agent_device_count = FWK_ARRAY_SIZE(agent_device_table_ospm),
};

static const struct fwk_element element_table[] = {
    [0] = {
        .name = "",
        .data = &(const struct mod_scmi_clock_agent_config){ 0 },
     },
    [JUNO_SCMI_AGENT_IDX_PSCI] = {
        .name = "",
        .data = &(const struct mod_scmi_clock_agent_config){ 0 },
     },
    [JUNO_SCMI_AGENT_IDX_OSPM] = {
        .name = "ospm",
        .data = &ospm_config,
    },
    [JUNO_SCMI_AGENT_IDX_COUNT] = { 0 },
};

static const struct fwk_element *get_element_table(fwk_id_t module_id)
{
    return element_table;
}

struct fwk_module_config config_scmi_clock = {
    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(get_element_table),
    .data = &((struct mod_scmi_clock_config){
        .max_pending_transactions = 0,
    }),
};
