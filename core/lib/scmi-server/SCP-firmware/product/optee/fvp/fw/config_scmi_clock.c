/*
 * Arm SCP/MCP Software
 * Copyright (c) 2022-2025, Linaro Limited and Contributors. All rights
 * reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "clock_devices.h"

#include <scmi_agents.h>

#include <mod_scmi_clock.h>

#include <fwk_module.h>
#include <fwk_module_idx.h>

static const struct mod_scmi_clock_device agent_device_table_ospm[] = {
    {
        /* MOCK_0 */
        .element_id =
            FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_CLOCK, CLOCK_DEV_IDX_MOCK_0),
        .starts_enabled = true,
    },
    {
        /* MOCK_1 */
        .element_id =
            FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_CLOCK, CLOCK_DEV_IDX_MOCK_1),
        .starts_enabled = true,
    },
    {
        /* VPU */
        .element_id =
            FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_CLOCK, CLOCK_DEV_IDX_VPU),
        .starts_enabled = true,
    },
    {
        /* MOCK_3 */
        .element_id =
            FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_CLOCK, CLOCK_DEV_IDX_MOCK_3),
        .starts_enabled = true,
    },
    {
        /* DPU */
        .element_id =
            FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_CLOCK, CLOCK_DEV_IDX_DPU),
        .starts_enabled = true,
    },
    {
        /* PIXEL_0 */
        .element_id =
            FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_CLOCK, CLOCK_DEV_IDX_PIXEL_0),
        .starts_enabled = true,
    },
    {
        /* PIXEL_1 */
        .element_id =
            FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_CLOCK, CLOCK_DEV_IDX_PIXEL_1),
        .starts_enabled = true,
    },
};

static const struct mod_scmi_clock_device agent_device_table_perf[] = {
    {
        /* MOCK_0 */
        .element_id =
            FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_CLOCK, CLOCK_DEV_IDX_MOCK_0),
    },
    {
        /* MOCK_2 */
        .element_id =
            FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_CLOCK, CLOCK_DEV_IDX_MOCK_2),
    },
    {
        /* MOCK_3 */
        .element_id =
            FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_CLOCK, CLOCK_DEV_IDX_MOCK_3),
    },
    {
        /* PIXEL_0 */
        .element_id =
            FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_CLOCK, CLOCK_DEV_IDX_PIXEL_0),
    },
    {
        /* PIXEL_1 */
        .element_id =
            FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_CLOCK, CLOCK_DEV_IDX_PIXEL_1),
    },
};

static const struct mod_scmi_clock_agent_config ospm_config = {
    .device_table = agent_device_table_ospm,
    .agent_device_count = FWK_ARRAY_SIZE(agent_device_table_ospm),
};

static const struct mod_scmi_clock_agent_config perf_config = {
    .device_table = agent_device_table_perf,
    .agent_device_count = FWK_ARRAY_SIZE(agent_device_table_perf),
};

static const struct fwk_element element_table[] = {
    [0] = {
        .name = "",
        .data = &(const struct mod_scmi_clock_agent_config){ 0 },
     },
    [SCMI_AGENT_ID_PSCI] = {
        .name = "",
        .data = &(const struct mod_scmi_clock_agent_config){ 0 },
     },
    [SCMI_AGENT_ID_OSPM] = {
        .name = "ospm",
        .data = &ospm_config,
    },
    [SCMI_AGENT_ID_PERF] = {
        .name = "perf",
        .data = &perf_config,
    },
    [SCMI_AGENT_ID_COUNT] = { 0 },
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
