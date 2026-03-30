/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Configuration data for module 'ros_clock'.
 */

#include "si0_clock.h"
#include "si0_ros_clock.h"

#include <mod_ros_clock.h>

#include <fwk_element.h>
#include <fwk_id.h>
#include <fwk_macros.h>
#include <fwk_module.h>

/* Cluster clock rate table */
static const struct mod_ros_clock_rate rate_table_clusterclk[] = {
    {
        .rate = CLOCK_RATE_CLUSTERROSCLK,
        .source = MOD_ROS_CLOCK_CLUSTER_CLK_SOURCE_CLUSTERPLLCLK,
        .divider = CLOCK_RATE_CLUSTERPLLCLK / CLOCK_RATE_CLUSTERROSCLK,
    },
};

/* Core clock rate table */
static const struct mod_ros_clock_rate rate_table_coreclk[] = {
    {
        .rate = CLOCK_RATE_COREROSCLK,
        .source = MOD_ROS_CLOCK_CORE_CLK_SOURCE_COREPLL0CLK,
        .divider = CLOCK_RATE_COREPLL0CLK / CLOCK_RATE_COREROSCLK,
    },
};

/* System clock rate table */
static const struct mod_ros_clock_rate rate_table_sysclk[] = {
    {
        .rate = CLOCK_RATE_SYSROSCLK,
        .source = MOD_ROS_CLOCK_SYS_CLK_SOURCE_SYSPLLCLK,
        .divider = CLOCK_RATE_SYSPLLCLK / CLOCK_RATE_SYSROSCLK,
    },
};

/* GIC clock rate table */
static const struct mod_ros_clock_rate rate_table_gicclk[] = {
    {
        .rate = CLOCK_RATE_GICROSCLK,
        .source = MOD_ROS_CLOCK_GIC_CLK_SOURCE_GICPLLCLK,
        .divider = CLOCK_RATE_GICPLLCLK / CLOCK_RATE_GICROSCLK,
    },
};

/* IO clock rate table */
static const struct mod_ros_clock_rate rate_table_ioclk[] = {
    {
        .rate = CLOCK_RATE_IOROSCLK,
        .source = MOD_ROS_CLOCK_IO_CLK_SOURCE_IOPLLCLK,
        .divider = CLOCK_RATE_IOPLLCLK / CLOCK_RATE_IOROSCLK,
    },
};

/* Periph clock rate table */
static const struct mod_ros_clock_rate rate_table_periphclk[] = {
    {
        .rate = CLOCK_RATE_PERIPHROSCLK,
        .source = MOD_ROS_CLOCK_PERIPH_CLK_SOURCE_PERIPHPLLCLK,
        .divider = CLOCK_RATE_PERIPHPLLCLK / CLOCK_RATE_PERIPHROSCLK,
    },
};

/* RSE clock rate table */
static const struct mod_ros_clock_rate rate_table_rseclk[] = {
    {
        .rate = CLOCK_RATE_RSEROSCLK,
        .source = MOD_ROS_CLOCK_RSE_CLK_SOURCE_RSEPLLCLK,
        .divider = CLOCK_RATE_RSEPLLCLK / CLOCK_RATE_RSEROSCLK,
    },
};

/* SI clock rate table */
static const struct mod_ros_clock_rate rate_table_siclk[] = {
    {
        .rate = CLOCK_RATE_SIROSCLK,
        .source = MOD_ROS_CLOCK_SI_CLK_SOURCE_SIPLLCLK,
        .divider = CLOCK_RATE_SIPLLCLK / CLOCK_RATE_SIROSCLK,
    },
};

/* SMD clock rate table */
static const struct mod_ros_clock_rate rate_table_smdclk[] = {
    {
        .rate = CLOCK_RATE_SMDROSCLK,
        .source = MOD_ROS_CLOCK_SMD_CLK_SOURCE_SMDPLLCLK,
        .divider = CLOCK_RATE_SMDPLLCLK / CLOCK_RATE_SMDROSCLK,
    },
};

/* DBG clock rate table */
static const struct mod_ros_clock_rate rate_table_dbgclk[] = {
    {
        .rate = CLOCK_RATE_DBGROSCLK,
        .source = MOD_ROS_CLOCK_DBG_CLK_SOURCE_DBGPLLCLK,
        .divider = CLOCK_RATE_DBGPLLCLK / CLOCK_RATE_DBGROSCLK,
    },
};

/* Trace clock rate table */
static const struct mod_ros_clock_rate rate_table_swclktck[] = {
    {
        .rate = CLOCK_RATE_TRACEROSCLK,
        .source = MOD_ROS_CLOCK_SW_CLK_TCK_SOURCE_TRACEPLLCLK,
        .divider = CLOCK_RATE_TRACEPLLCLK / CLOCK_RATE_TRACEROSCLK,
    },
};

static const struct fwk_element ros_clock_table[] = {
    [CFGD_MOD_ROS_CLOCK_EIDX_CLUSTER] = {
        .name = "ROS CLK CLUSTER",
        .data = &((struct mod_ros_clock_dev_config) {
            .control_reg = &ROS_CLOCK_PTR->CLUSTERCLK,
            .rate_table = rate_table_clusterclk,
            .rate_count = FWK_ARRAY_SIZE(rate_table_clusterclk),
            .initial_rate = CLOCK_RATE_CLUSTERROSCLK,
        }),
    },
    [CFGD_MOD_ROS_CLOCK_EIDX_CORE] = {
        .name = "ROS CLK CORE",
        .data = &((struct mod_ros_clock_dev_config) {
            .control_reg = &ROS_CLOCK_PTR->CORECLK,
            .rate_table = rate_table_coreclk,
            .rate_count = FWK_ARRAY_SIZE(rate_table_coreclk),
            .initial_rate = CLOCK_RATE_COREROSCLK,
        }),
    },
    [CFGD_MOD_ROS_CLOCK_EIDX_SYS] = {
        .name = "ROS CLK SYS",
        .data = &((struct mod_ros_clock_dev_config) {
            .control_reg = &ROS_CLOCK_PTR->SYSCLK,
            .rate_table = rate_table_sysclk,
            .rate_count = FWK_ARRAY_SIZE(rate_table_sysclk),
            .initial_rate = CLOCK_RATE_SYSROSCLK,
        }),
    },
    [CFGD_MOD_ROS_CLOCK_EIDX_GIC] = {
        .name = "ROS CLK GIC",
        .data = &((struct mod_ros_clock_dev_config) {
            .control_reg = &ROS_CLOCK_PTR->GICCLK,
            .rate_table = rate_table_gicclk,
            .rate_count = FWK_ARRAY_SIZE(rate_table_gicclk),
            .initial_rate = CLOCK_RATE_GICROSCLK,
        }),
    },
    [CFGD_MOD_ROS_CLOCK_EIDX_IO] = {
        .name = "ROS CLK IO",
        .data = &((struct mod_ros_clock_dev_config) {
            .control_reg = &ROS_CLOCK_PTR->IOCLK,
            .rate_table = rate_table_ioclk,
            .rate_count = FWK_ARRAY_SIZE(rate_table_ioclk),
            .initial_rate = CLOCK_RATE_IOROSCLK,
        }),
    },
    [CFGD_MOD_ROS_CLOCK_EIDX_PERIPH] = {
        .name = "ROS CLK PERIPH",
        .data = &((struct mod_ros_clock_dev_config) {
            .control_reg = &ROS_CLOCK_PTR->PERIPHCLK,
            .rate_table = rate_table_periphclk,
            .rate_count = FWK_ARRAY_SIZE(rate_table_periphclk),
            .initial_rate = CLOCK_RATE_PERIPHROSCLK,
        }),
    },
    [CFGD_MOD_ROS_CLOCK_EIDX_RSE] = {
        .name = "ROS CLK RSE",
        .data = &((struct mod_ros_clock_dev_config) {
            .control_reg = &ROS_CLOCK_PTR->RSECLK,
            .rate_table = rate_table_rseclk,
            .rate_count = FWK_ARRAY_SIZE(rate_table_rseclk),
            .initial_rate = CLOCK_RATE_RSEROSCLK,
        }),
    },
    [CFGD_MOD_ROS_CLOCK_EIDX_SI] = {
        .name = "ROS CLK SI",
        .data = &((struct mod_ros_clock_dev_config) {
            .control_reg = &ROS_CLOCK_PTR->SICLK,
            .rate_table = rate_table_siclk,
            .rate_count = FWK_ARRAY_SIZE(rate_table_siclk),
            .initial_rate = CLOCK_RATE_SIROSCLK,
        }),
    },
    [CFGD_MOD_ROS_CLOCK_EIDX_SMD] = {
        .name = "ROS CLK SMD",
        .data = &((struct mod_ros_clock_dev_config) {
            .control_reg = &ROS_CLOCK_PTR->SMDCLK,
            .rate_table = rate_table_smdclk,
            .rate_count = FWK_ARRAY_SIZE(rate_table_smdclk),
            .initial_rate = CLOCK_RATE_SMDROSCLK,
        }),
    },
    [CFGD_MOD_ROS_CLOCK_EIDX_DBG] = {
        .name = "ROS CLK DBG",
        .data = &((struct mod_ros_clock_dev_config) {
            .control_reg = &ROS_CLOCK_PTR->DBGCLK,
            .rate_table = rate_table_dbgclk,
            .rate_count = FWK_ARRAY_SIZE(rate_table_dbgclk),
            .initial_rate = CLOCK_RATE_DBGROSCLK,
        }),
    },
    [CFGD_MOD_ROS_CLOCK_EIDX_TRACE] = {
        .name = "ROS CLK TRACE",
        .data = &((struct mod_ros_clock_dev_config) {
            .control_reg = &ROS_CLOCK_PTR->SWCLKTCK,
            .rate_table = rate_table_swclktck,
            .rate_count = FWK_ARRAY_SIZE(rate_table_swclktck),
            .initial_rate = CLOCK_RATE_TRACEROSCLK,
        }),
    },
    [CFGD_MOD_ROS_CLOCK_EIDX_COUNT] = { 0 },
};

static const struct fwk_element *ros_clock_get_element_table(fwk_id_t module_id)
{
    return ros_clock_table;
}

const struct fwk_module_config config_ros_clock = {
    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(ros_clock_get_element_table),
};
