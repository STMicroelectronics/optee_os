/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Common RSM SRAM memory region mapping, shared across SCP, MCP and RSE.
 */

#ifndef RSM_FW_MMAP_H
#define RSM_FW_MMAP_H

#include <fwk_macros.h>

// clang-format off

/*
 *                                             [SCP_ATW0_SHARED_SRAM_RSM_BASE]
 *  +---------------------------------------+  SCP_MCP_SCMI_MSG_PAYLOAD_BASE
 *  |                                       |
 *  |         SCP-MCP SCMI payload          |
 *  | (SCP_MCP_SCMI_MSG_PAYLOAD_SIZE bytes) |
 *  +---------------------------------------+  [SCP_RSE_SCMI_MSG_PAYLOAD_BASE]
 *  |                                       |
 *  |         SCP-RSE SCMI payload          |
 *  | (SCP_RSE_SCMI_MSG_PAYLOAD_SIZE bytes) |
 *  +---------------------------------------+
 *  |                                       |
 *  |                                       |
 *  |                Unused                 |
 *  |                                       |
 *  |                                       |
 *  +---------------------------------------+  [SCP_ATW0_SHARED_SRAM_RSM_BASE +
 *                                              RSM_SHARED_SRAM_SIZE]
 */

#define SCP_MCP_SCMI_MSG_PAYLOAD_OFFSET UINT32_C(0x0)
#define SCP_MCP_SCMI_MSG_PAYLOAD_SIZE   (128)

#define SCP_RSE_SCMI_MSG_PAYLOAD_OFFSET \
    (SCP_MCP_SCMI_MSG_PAYLOAD_OFFSET + SCP_MCP_SCMI_MSG_PAYLOAD_SIZE)
#define SCP_RSE_SCMI_MSG_PAYLOAD_SIZE   (128)

// clang-format on

#endif /* RSM_FW_MMAP_H */
