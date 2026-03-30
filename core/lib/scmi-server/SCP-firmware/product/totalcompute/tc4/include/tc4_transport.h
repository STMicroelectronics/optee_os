/*
 * Arm SCP/MCP Software
 * Copyright (c) 2023-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Definitions for TRANSPORT module configurations.
 */

#ifndef SCP_TC4_TRANSPORT_H
#define SCP_TC4_TRANSPORT_H

/* Transport service indexes */
enum scp_tc4_transport_service_idx {
    SCP_TC4_TRANSPORT_SERVICE_IDX_PSCI,
    SCP_TC4_TRANSPORT_SERVICE_IDX_OSPM_A2P,
#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
    SCP_TC4_TRANSPORT_SERVICE_IDX_OSPM_P2A,
#endif
    SCP_TC4_TRANSPORT_SERVICE_IDX_SCP2RSS,
    SCP_TC4_TRANSPORT_SERVICE_IDX_COUNT,
};

#endif /* SCP_TC4_TRANSPORT_H */
