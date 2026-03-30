/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     SCMI payload configuration data for Neoverse RD platform.
 */

#ifndef NRD_SCMI_H
#define NRD_SCMI_H

#include <stdint.h>

// clang-format off

#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
/*
 * System power states for Neoverse RD platform
 */
enum nrd_scmi_system_state {
    NRD_SCMI_SYSTEM_STATE_SHUTDOWN,
    NRD_SCMI_SYSTEM_STATE_COLD_RESET,
    NRD_SCMI_SYSTEM_STATE_WARM_RESET,
    NRD_SCMI_SYSTEM_STATE_COUNT,
};

/*
 * SCMI payload for scmi sytem power protocol, message id
 * SYSTEM_POWER_STATE_SET
 * This will be removed once SCMI support is enabled at RSE
 */
struct scp_cfgd_scmi_sys_power_state_set_payload {
    uint32_t flags;
    uint32_t system_state;
};

/*
 * SCMI payload for scmi sytem power protocol, message id
 * SYSTEM_POWER_STATE_NOTIFY
 */
struct mcp_cfgd_scmi_sys_power_state_notify_payload {
    uint32_t flags;
};
#endif

// clang-format on

#endif /* NRD_SCMI_H */
