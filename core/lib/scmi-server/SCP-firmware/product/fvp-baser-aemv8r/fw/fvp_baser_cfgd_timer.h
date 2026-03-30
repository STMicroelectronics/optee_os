/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Definitions for timer module configuration data in SCP firmware.
 */

#ifndef FVP_BASER_CFGD_TIMER_H
#define FVP_BASER_CFGD_TIMER_H

/* Alarm sub-element indices */
enum timer_alarm_idx {
    FVP_BASE_R_TIMER_ALARM_IDX_DEBUGGER,
    FVP_BASE_R_TIMER_ALARM_IDX_TEST_TIMER,
    FVP_BASE_R_TIMER_ALARM_IDX_COUNT,
};

#endif /* FVP_BASER_CFGD_TIMER_H */
