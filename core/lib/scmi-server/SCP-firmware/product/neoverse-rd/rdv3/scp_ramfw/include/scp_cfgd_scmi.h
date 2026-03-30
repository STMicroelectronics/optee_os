/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Definitions for SCMI module configuration data in SCP firmware.
 */

#ifndef SCP_CFGD_SCMI_H
#define SCP_CFGD_SCMI_H

#include <stdint.h>

/* SCMI agent identifier indexes in the SCMI agent table */
enum scp_scmi_agent_idx {
    /* 0 is reserved for the platform */
    SCP_SCMI_AGENT_IDX_PSCI = 1,
    SCP_SCMI_AGENT_IDX_RSE,
    SCP_SCMI_AGENT_IDX_MCP,
    SCP_SCMI_AGENT_IDX_COUNT,
};

/* Module 'scmi' element indexes (SCMI services supported) */
enum scp_cfgd_mod_scmi_element_idx {
    SCP_CFGD_MOD_SCMI_EIDX_PSCI,
    SCP_CFGD_MOD_SCMI_EIDX_MCP_SCMI_SEND,
    SCP_CFGD_MOD_SCMI_EIDX_RSE_SCMI_SEND,
#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
    SCP_CFGD_MOD_SCMI_EIDX_MCP_SCMI_RECV,
    SCP_CFGD_MOD_SCMI_EIDX_RSE_SCMI_RECV,
#endif
    SCP_CFGD_MOD_SCMI_EIDX_COUNT,
};

/* SCMI protocol requester agents */
enum scmi_protocol_requester {
    SCMI_PROTOCOL_REQUESTER_MCP,
    SCMI_PROTOCOL_REQUESTER_RSE,
    SCMI_PROTOCOL_REQUESTER_COUNT
};

#endif /* SCP_CFGD_SCMI_H */
