/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef INTERNAL_SCMI_SYS_POWER_REQ_H
#define INTERNAL_SCMI_SYS_POWER_REQ_H

#include <stdint.h>

/*!
 * \brief Flag mask for set state notification
 */
#define STATE_NOTIFY_FLAGS_MASK 0x1U

/*!
 * \brief Notification Payload
 */
struct scmi_sys_power_notification_payload {
    /*! Agent ID receiving notification */
    uint32_t agent_id;

    /*! System Power command flags*/
    uint32_t flags;

    /*! System Power state being transitioned to */
    uint32_t system_state;
};

/*!
 * \brief Notification Subscription Payload
 */
struct scmi_sys_power_req_state_notify_a2p {
    /*! Notifications flags*/
    uint32_t flags;
};

/*
 * Assert that the notification parameters size is more than the
 * size of the state and flags
 */
static_assert(
    (sizeof(((struct scmi_sys_power_notification_payload *)0)->flags) +
     sizeof(((struct scmi_sys_power_notification_payload *)0)->system_state)) <
    FWK_EVENT_PARAMETERS_SIZE);

#endif /* INTERNAL_SCMI_SYS_POWER_REQ_H */
