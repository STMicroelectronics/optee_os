/*
 * Arm SCP/MCP Software
 * Copyright (c) 2023-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef TC3_TIMER_H
#define TC3_TIMER_H

enum config_timer_refclk_sub_element_idx {
    TC3_CONFIG_TIMER_DVFS_CPU_GROUP_LITTLE,
    TC3_CONFIG_TIMER_DVFS_CPU_GROUP_MID,
    TC3_CONFIG_TIMER_DVFS_CPU_GROUP_BIG,
    TC3_CONFIG_TIMER_DVFS_GPU,
#ifdef BUILD_HAS_SCMI_PERF_FAST_CHANNELS
    TC3_CONFIG_TIMER_FAST_CHANNEL_TIMER_IDX,
#endif
    TC3_CONFIG_TIMER_SUB_ELEMENT_IDX_COUNT,
};

#endif /* TC3_TIMER_H */
