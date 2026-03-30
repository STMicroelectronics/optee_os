/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Configuration data for module 'mhu3'.
 */

#include "mcp_cfgd_mhu3.h"
#include "mcp_css_mmap.h"

#include <mod_mhu3.h>

#include <fwk_element.h>
#include <fwk_id.h>
#include <fwk_module.h>

#include <fmw_cmsis.h>

/* MCP<-->SCP Secure MHUv3 doorbell channel configuration */
static struct mod_mhu3_channel_config mcp2scp_s_dbch_config[] = {
    /* PBX CH 0, FLAG 0, MBX CH 0, FLAG 0 */
    MOD_MHU3_INIT_DBCH(0, 0, 0, 0),
};

/* Module element table */
static const struct fwk_element element_table[] = {
    [MCP_CFGD_MOD_MHU3_EIDX_MCP_SCP_S] = {
        .name = "MCP2SCP_S_MHU_DBCH",
        .sub_element_count = 1,
        .data = &(struct mod_mhu3_device_config) {
            .irq = (unsigned int) MHU3_SCP2MCP_IRQ_S,
            .in = MCP_SCP2MCP_MHUV3_RCV_S_BASE,
            .out = MCP_MCP2SCP_MHUV3_SEND_S_BASE,
            .channels = &mcp2scp_s_dbch_config[0],
        },
    },
    [MCP_CFGD_MOD_MHU3_EIDX_COUNT] = { 0 },
};

static const struct fwk_element *get_element_table(fwk_id_t module_id)
{
    return element_table;
}

struct fwk_module_config config_mhu3 = {
    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(get_element_table),
};
