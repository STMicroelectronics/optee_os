/*
 * Arm SCP/MCP Software
 * Copyright (c) 2023-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "clock_soc.h"
#include "tc3_dvfs.h"
#include "tc3_psu.h"
#include "tc3_timer.h"
#include "tc_core.h"

#include <mod_dvfs.h>
#include <mod_scmi_perf.h>

#include <fwk_element.h>
#include <fwk_macros.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>

/*
 * The power cost figures in this file are built using the dynamic power
 * consumption formula (P = CfV^2), where C represents the capacitance of one
 * processing element in the domain (a core or shader core). This power figure
 * is scaled linearly with the number of processing elements in the performance
 * domain to give a rough representation of the overall power draw. The
 * capacitance constants are given in mW/MHz/V^2 and were taken from the Linux
 * device trees, which provide a dynamic-power-coefficient field in uW/MHz/V^2.
 * This conversion of units, from uW/MHz/V^2 to mW/MHz/V^2, is done by dividing
 * by 1000.
 */

/* dynamic-power-coeffient/1000 */
#define GROUP_LITTLE_DPC 0.230
#define GROUP_MID_DPC    0.495
#define GROUP_BIG_DPC    1.054

static struct mod_dvfs_opp operating_points_group_little[6] = {
    {
        .level = 768 * 1000000UL,
        .frequency = 768 * FWK_KHZ,
        .voltage = 550,
        .power = (uint32_t)(GROUP_LITTLE_DPC * 768 * 0.550 * 0.550),
    },
    {
        .level = 1153 * 1000000UL,
        .frequency = 1153 * FWK_KHZ,
        .voltage = 650,
        .power = (uint32_t)(GROUP_LITTLE_DPC * 1153 * 0.650 * 0.650),
    },
    {
        .level = 1537 * 1000000UL,
        .frequency = 1537 * FWK_KHZ,
        .voltage = 750,
        .power = (uint32_t)(GROUP_LITTLE_DPC * 1537 * 0.750 * 0.750),
    },
    {
        .level = 1844 * 1000000UL,
        .frequency = 1844 * FWK_KHZ,
        .voltage = 850,
        .power = (uint32_t)(GROUP_LITTLE_DPC * 1844 * 0.850 * 0.850),
    },
    {
        .level = 2152 * 1000000UL,
        .frequency = 2152 * FWK_KHZ,
        .voltage = 950,
        .power = (uint32_t)(GROUP_LITTLE_DPC * 2152 * 0.950 * 0.950),
    },
    { 0 }
};

static struct mod_dvfs_opp operating_points_group_mid[6] = {
    {
        .level = 946 * 1000000UL,
        .frequency = 946 * FWK_KHZ,
        .voltage = 550,
        .power = (uint32_t)(GROUP_MID_DPC * 946 * 0.550 * 0.550),
    },
    {
        .level = 1419 * 1000000UL,
        .frequency = 1419 * FWK_KHZ,
        .voltage = 650,
        .power = (uint32_t)(GROUP_MID_DPC * 1419 * 0.650 * 0.650),
    },
    {
        .level = 1893 * 1000000UL,
        .frequency = 1893 * FWK_KHZ,
        .voltage = 750,
        .power = (uint32_t)(GROUP_MID_DPC * 1893 * 0.750 * 0.750),
    },
    {
        .level = 2271 * 1000000UL,
        .frequency = 2271 * FWK_KHZ,
        .voltage = 850,
        .power = (uint32_t)(GROUP_MID_DPC * 2271 * 0.850 * 0.850),
    },
    {
        .level = 2650 * 1000000UL,
        .frequency = 2650 * FWK_KHZ,
        .voltage = 950,
        .power = (uint32_t)(GROUP_MID_DPC * 2650 * 0.950 * 0.950),
    },
    { 0 }
};

static struct mod_dvfs_opp operating_points_group_big[6] = {
    {
        .level = 1088 * 1000000UL,
        .frequency = 1088 * FWK_KHZ,
        .voltage = 550,
        .power = (uint32_t)(GROUP_BIG_DPC * 1088 * 0.550 * 0.550),
    },
    {
        .level = 1632 * 1000000UL,
        .frequency = 1632 * FWK_KHZ,
        .voltage = 650,
        .power = (uint32_t)(GROUP_BIG_DPC * 1632 * 0.650 * 0.650),
    },
    {
        .level = 2176 * 1000000UL,
        .frequency = 2176 * FWK_KHZ,
        .voltage = 750,
        .power = (uint32_t)(GROUP_BIG_DPC * 2176 * 0.750 * 0.750),
    },
    {
        .level = 2612 * 1000000UL,
        .frequency = 2612 * FWK_KHZ,
        .voltage = 850,
        .power = (uint32_t)(GROUP_BIG_DPC * 2612 * 0.850 * 0.850),
    },
    {
        .level = 3047 * 1000000UL,
        .frequency = 3047 * FWK_KHZ,
        .voltage = 950,
        .power = (uint32_t)(GROUP_BIG_DPC * 3047 * 0.950 * 0.950),
    },
    { 0 }
};

static struct mod_dvfs_opp operating_points_gpu[5] = {
    {
        .level = 350 * 1000000UL,
        .frequency = 350 * FWK_KHZ,
        .voltage = 550,
    },
    {
        .level = 680 * 1000000UL,
        .frequency = 680 * FWK_KHZ,
        .voltage = 650,
    },
    {
        .level = 1000 * 1000000UL,
        .frequency = 1000 * FWK_KHZ,
        .voltage = 750,
    },
    {
        .level = 1260 * 1000000UL,
        .frequency = 1260 * FWK_KHZ,
        .voltage = 850,
    },
    { 0 }
};

static const struct mod_dvfs_domain_config cpu_group_group_little = {
    .psu_id =
        FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_PSU, PSU_ELEMENT_IDX_GROUP_LITTLE),
    .clock_id = FWK_ID_ELEMENT_INIT(
        FWK_MODULE_IDX_CLOCK,
        CLOCK_IDX_CPU_GROUP_GROUP_LITTLE),
    .alarm_id = FWK_ID_SUB_ELEMENT_INIT(
        FWK_MODULE_IDX_TIMER,
        0,
        TC3_CONFIG_TIMER_DVFS_CPU_GROUP_LITTLE),
    .retry_us = 1000,
    .latency = 1200,
    .sustained_idx = 2,
    .opps = operating_points_group_little,
};

static const struct mod_dvfs_domain_config cpu_group_group_mid = {
    .psu_id =
        FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_PSU, PSU_ELEMENT_IDX_GROUP_MID),
    .clock_id = FWK_ID_ELEMENT_INIT(
        FWK_MODULE_IDX_CLOCK,
        CLOCK_IDX_CPU_GROUP_GROUP_MID),
    .alarm_id = FWK_ID_SUB_ELEMENT_INIT(
        FWK_MODULE_IDX_TIMER,
        0,
        TC3_CONFIG_TIMER_DVFS_CPU_GROUP_MID),
    .retry_us = 1000,
    .latency = 1200,
    .sustained_idx = 2,
    .opps = operating_points_group_mid,
};

static const struct mod_dvfs_domain_config cpu_group_group_big = {
    .psu_id =
        FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_PSU, PSU_ELEMENT_IDX_GROUP_BIG),
    .clock_id = FWK_ID_ELEMENT_INIT(
        FWK_MODULE_IDX_CLOCK,
        CLOCK_IDX_CPU_GROUP_GROUP_BIG),
    .alarm_id = FWK_ID_SUB_ELEMENT_INIT(
        FWK_MODULE_IDX_TIMER,
        0,
        TC3_CONFIG_TIMER_DVFS_CPU_GROUP_BIG),
    .retry_us = 1000,
    .latency = 1200,
    .sustained_idx = 2,
    .opps = operating_points_group_big,
};

static const struct mod_dvfs_domain_config gpu = {
    .psu_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_PSU, PSU_ELEMENT_IDX_GPU),
    .clock_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_CLOCK, CLOCK_IDX_GPU),
    .alarm_id = FWK_ID_SUB_ELEMENT_INIT(
        FWK_MODULE_IDX_TIMER,
        0,
        TC3_CONFIG_TIMER_DVFS_GPU),
    .retry_us = 1000,
    .latency = 1200,
    .sustained_idx = 2,
    .opps = operating_points_gpu,
};

static const struct fwk_element element_table[DVFS_ELEMENT_IDX_COUNT + 1] = {
    [DVFS_ELEMENT_IDX_GROUP_LITTLE] =
        {
            .name = "CPU_GROUP_" TC_GROUP_LITTLE_NAME,
            .data = &cpu_group_group_little,
        },
    [DVFS_ELEMENT_IDX_GROUP_MID] =
        {
            .name = "CPU_GROUP_" TC_GROUP_MID_NAME,
            .data = &cpu_group_group_mid,
        },
    [DVFS_ELEMENT_IDX_GROUP_BIG] =
        {
            .name = "CPU_GROUP_" TC_GROUP_BIG_NAME,
            .data = &cpu_group_group_big,
        },
    [DVFS_ELEMENT_IDX_GPU] =
        {
            .name = "GPU",
            .data = &gpu,
        },
    { 0 },
};

static const struct fwk_element *dvfs_get_element_table(fwk_id_t module_id)
{
    return element_table;
}

const struct fwk_module_config config_dvfs = {
    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(dvfs_get_element_table),
};
