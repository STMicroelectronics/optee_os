/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Base address and size definitions for the various SCP's firmware defined
 *     memory carveouts.
 */

#ifndef SCP_FW_MMAP_H
#define SCP_FW_MMAP_H

#include "scp_mmap.h"

/* AP Peripheral trusted SRAM base in SCP's memory map (4KB) */
#define SCP_AP_PERIPHERAL_SRAM_TRUSTED_BASE SCP_ATW0_AP_PERIPHERAL_SRAM_BASE

/* Secure Shared memory between AP and SCP */
#define SCP_AP_PERIPHERAL_SRAM_SHARED_SECURE_BASE \
    (SCP_AP_PERIPHERAL_SRAM_TRUSTED_BASE)
#define SCP_AP_PERIPHERAL_SRAM_SHARED_SECURE_SIZE (4 * FWK_KIB)

/*
 * AP Context Memory Region inside Secure AP Peripheral SRAM that is shared
 * between AP and SCP.
 */
#define SCP_AP_CONTEXT_SIZE (64)
#define SCP_AP_CONTEXT_BASE \
    (SCP_AP_PERIPHERAL_SRAM_SHARED_SECURE_BASE + \
     SCP_AP_PERIPHERAL_SRAM_SHARED_SECURE_SIZE - SCP_AP_CONTEXT_SIZE)

/*
 * SDS Memory Region inside Secure AP Peripheral SRAM that is shared between
 * AP and SCP.
 */
#define SCP_SDS_SECURE_BASE (SCP_AP_PERIPHERAL_SRAM_SHARED_SECURE_BASE)
#define SCP_SDS_SECURE_SIZE (3520)

/* SCMI Secure Payload Area */
#define SCP_SCMI_PAYLOAD_S_A2P_BASE (SCP_SDS_SECURE_BASE + SCP_SDS_SECURE_SIZE)
#define SCP_SCMI_PAYLOAD_SIZE       (128)

/* Payload Area for SCP-RSE outband message */
#define SCP_RSE_TRANSPORT_PAYLOAD_BASE (SCP_SHARED_SRAM_RSM_BASE)
#define SCP_RSE_TRANSPORT_PAYLOAD_SIZE (128)

#endif /* SCP_FW_MMAP_H */
