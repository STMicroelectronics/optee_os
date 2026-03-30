/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SCMI_SYSPWR_H
#define SCMI_SYSPWR_H

#include <stdint.h>

#define SCMI_PROTOCOL_VERSION_SYS_POWER UINT32_C(0x10000)

/* Notifications supported by SCMI system power protocol as per SCMI 3.2 spec */
enum scmi_sys_power_notification_id {
    SCMI_SYS_POWER_STATE_SET_NOTIFY = 0x000,
    SCMI_SYS_POWER_NOTIFICATION_COUNT,
};

/*
 * SYSTEM_POWER_STATE_NOTIFY
 */

#define STATE_NOTIFY_FLAGS_MASK 0x1U

struct scmi_sys_power_state_notify_a2p {
    uint32_t flags;
};

struct scmi_sys_power_state_notify_p2a {
    int32_t status;
};

/*
 * SYSTEM_POWER_STATE_NOTIFIER
 */

struct scmi_sys_power_state_notifier {
    uint32_t agent_id;
    uint32_t flags;
    uint32_t system_state;
};

#endif /* SCMI_SYSPWR_H */
