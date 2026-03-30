/*
 * Arm SCP/MCP Software
 * Copyright (c) 2014-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <mod_posix_transport.h>
#include <mod_scmi.h>

#include <fwk_element.h>
#include <fwk_id.h>
#include <fwk_macros.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>

static const struct fwk_element element_table[] = {
    [0] = {
        .name = "OSPM0",
        .data = &(struct mod_scmi_service_config) {
            .transport_id = FWK_ID_ELEMENT_INIT(
                FWK_MODULE_IDX_POSIX_TRANSPORT,
                0),
            .transport_api_id = FWK_ID_API_INIT(
                FWK_MODULE_IDX_POSIX_TRANSPORT,
                0),
            .transport_notification_init_id = FWK_ID_NONE_INIT,
            .scmi_agent_id = (unsigned int) 1,
            .scmi_p2a_id = FWK_ID_NONE_INIT,
        },
    },

    [1] = { 0 },
};

static const struct fwk_element *get_element_table(fwk_id_t module_id)
{
    return element_table;
}

static const struct mod_scmi_agent agent_table[] = {
    [1] = {
        .type = SCMI_AGENT_TYPE_OSPM,
        .name = "OSPM",
    },
};

struct fwk_module_config config_scmi = {
    .data =
        &(struct mod_scmi_config){
            .protocol_count_max = 8,
            .agent_count = FWK_ARRAY_SIZE(agent_table) - 1,
            .agent_table = agent_table,
            .vendor_identifier = "arm",
            .sub_vendor_identifier = "arm",
        },

    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(get_element_table),
};
