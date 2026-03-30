/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Definitions for SCMI module configuration data in MCP firmware.
 */

#ifndef MCP_CFGD_SCMI_H
#define MCP_CFGD_SCMI_H

/* SCMI agent identifier indexes in the SCMI agent table */
enum mcp_scmi_agent_idx {
    /* 0 is reserved for the platform */
    MCP_SCMI_AGENT_IDX_SCP = 1,
    MCP_SCMI_AGENT_IDX_COUNT,
};

/* Module 'scmi' element indexes (SCMI services supported) */
enum mcp_cfgd_mod_scmi_element_idx {
#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
    MCP_CFGD_MOD_SCMI_EIDX_SCP_SCMI_SEND,
#endif
    MCP_CFGD_MOD_SCMI_EIDX_SCP_SCMI_RECV,
    MCP_CFGD_MOD_SCMI_EIDX_COUNT,
};

#endif /* MCP_CFGD_SCMI_H */
