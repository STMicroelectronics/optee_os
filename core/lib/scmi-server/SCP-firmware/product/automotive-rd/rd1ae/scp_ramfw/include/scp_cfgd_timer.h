/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Definitions for timer module configuration data in SCP firmware.
 */

#ifndef SCP_CFGD_TIMER_H
#define SCP_CFGD_TIMER_H

#define SCP_ALARM_ELEMENT_IDX 0

/* Sub-element indexes (alarms) for SCP timer device */
enum scp_cfgd_mod_timer_alarm_idx {
#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
    SCP_CFGD_SCMI_NOTIFICATION_ALARM_IDX,
#endif
    SCP_CFGD_MOD_TIMER_ALARM_IDX_COUNT,
};

#endif /* SCP_CFGD_TIMER_H */
