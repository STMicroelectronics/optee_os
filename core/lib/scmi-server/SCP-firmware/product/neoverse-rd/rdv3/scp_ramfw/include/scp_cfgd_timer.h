/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Definitions for timer module configuration data in SCP firmware.
 */

#ifndef SCP_CFGD_TIMER_H
#define SCP_CFGD_TIMER_H

/* Element indexes for SCP timer device */
enum scp_cfgd_mod_timer_element_idx {
    SCP_TIMER_ALARM_EIDX,
    SCP_TIMER_ELEMENT_COUNT
};

/* Sub-element indexes (alarms) for SCP timer device */
enum scp_cfgd_mod_timer_alarm_idx {
#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
    SCP_CFGD_SCMI_SYSPWR_MGMT_NOTIFY_ALARM_IDX,
#endif
    SCP_CFGD_MOD_TIMER_ALARM_IDX_COUNT,
};

#endif /* SCP_CFGD_TIMER_H */
