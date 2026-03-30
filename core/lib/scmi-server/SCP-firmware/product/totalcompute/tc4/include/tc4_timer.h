/*
 * Arm SCP/MCP Software
 * Copyright (c) 2023-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef CONFIG_TIMER_H
#define CONFIG_TIMER_H

enum config_timer_refclk_sub_element_idx {
    TC4_CONFIG_TIMER_DVFS_CPU_GROUP_LITTLE,
    TC4_CONFIG_TIMER_DVFS_CPU_GROUP_MID,
    TC4_CONFIG_TIMER_DVFS_CPU_GROUP_BIG,
    TC4_CONFIG_TIMER_DVFS_GPU,
#ifdef BUILD_HAS_SCMI_PERF_FAST_CHANNELS
    TC4_CONFIG_TIMER_FAST_CHANNEL_TIMER_IDX,
#endif
    TC4_CONFIG_TIMER_SUB_ELEMENT_IDX_COUNT,
};

#endif /* CONFIG_TIMER_H */
