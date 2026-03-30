/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Base address and size definitions for the various MCP's firmware defined
 *     memory carveouts.
 */

#ifndef MCP_FW_MMAP_H
#define MCP_FW_MMAP_H

#include "mcp_css_mmap.h"
#include "rsm_fw_mmap.h"

/*
 * RSM SRAM in the AP memory map with base address of 0x2F000000 is mapped in
 * the MCP's address translation window 0 (0x60000000 - 0x9FFFFFFF) at the
 * offset 'MCP_SHARED_SRAM_RSM_BASE' via ATU configuration.
 */

/* MCP-SCP SCMI message payload base address (in RSM SRAM) */
#define MCP_SCP_SCMI_MSG_PAYLOAD_BASE \
    (MCP_SHARED_SRAM_RSM_BASE + SCP_MCP_SCMI_MSG_PAYLOAD_OFFSET)

#endif /* MCP_FW_MMAP_H */
