/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOD_SCMI_SYS_POWER_H
#define MOD_SCMI_SYS_POWER_H

#include <fwk_id.h>
#include <fwk_module_idx.h>

#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
#    ifdef BUILD_HAS_NOTIFICATION
/*!
 * \brief Indices of the notification sent by the module.
 */
enum mod_scmi_sys_power_notification_idx {
    /*! Power state transition */
    MOD_SCMI_SYSTEM_POWER_NOTIFICATION_IDX_SYSTEM_POWER_DOWN,

    /*! Number of notifications defined */
    MOD_SCMI_SYSTEM_POWER_NOTIFICATION_COUNT
};
#    endif
#else
/*!
 * \brief Indices of the notification sent by the module.
 */
enum mod_scmi_sys_power_notification_idx {
    /*! Number of notifications defined */
    MOD_SCMI_SYSTEM_POWER_NOTIFICATION_COUNT = 0,
};
#endif

#endif /* MOD_SCMI_SYS_POWER_H */
