/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Configuration data for module 'clock'.
 */

#include "platform_core.h"
#include "si0_cfgd_power_domain.h"
#include "si0_clock.h"

#include <mod_clock.h>
#include <mod_power_domain.h>
#include <mod_ros_clock.h>

#include <fwk_element.h>
#include <fwk_id.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>

/* Module 'clock' element count */
#define MOD_CLOCK_ELEMENT_COUNT (CFGD_MOD_CLOCK_EIDX_COUNT + 1)

/*
 * Module 'clock' element configuration data.
 */
static const struct fwk_element clock_dev_table[MOD_CLOCK_ELEMENT_COUNT] = {
    [CFGD_MOD_CLOCK_EIDX_CLUSTER] = {
        .name = "Cluster",
        .data = &((struct mod_clock_dev_config) {
            .driver_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_ROS_CLOCK,
                CFGD_MOD_ROS_CLOCK_EIDX_CLUSTER),
            .api_id = FWK_ID_API_INIT(FWK_MODULE_IDX_ROS_CLOCK,
                MOD_ROS_CLOCK_API_TYPE_CLOCK),
        }),
    },
    [CFGD_MOD_CLOCK_EIDX_CORE] = {
        .name = "Core",
        .data = &((struct mod_clock_dev_config) {
            .driver_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_ROS_CLOCK,
                CFGD_MOD_ROS_CLOCK_EIDX_CORE),
            .api_id = FWK_ID_API_INIT(FWK_MODULE_IDX_ROS_CLOCK,
                MOD_ROS_CLOCK_API_TYPE_CLOCK),
        }),
    },
    [CFGD_MOD_CLOCK_EIDX_SYS] = {
        .name = "SYS",
        .data = &((struct mod_clock_dev_config) {
            .driver_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_ROS_CLOCK,
                CFGD_MOD_ROS_CLOCK_EIDX_SYS),
            .api_id = FWK_ID_API_INIT(FWK_MODULE_IDX_ROS_CLOCK,
                MOD_ROS_CLOCK_API_TYPE_CLOCK),
        }),
    },
    [CFGD_MOD_CLOCK_EIDX_GIC] = {
        .name = "GIC",
        .data = &((struct mod_clock_dev_config) {
            .driver_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_ROS_CLOCK,
                CFGD_MOD_ROS_CLOCK_EIDX_GIC),
            .api_id = FWK_ID_API_INIT(FWK_MODULE_IDX_ROS_CLOCK,
                MOD_ROS_CLOCK_API_TYPE_CLOCK),
        }),
    },
    [CFGD_MOD_CLOCK_EIDX_IO] = {
        .name = "IO",
        .data = &((struct mod_clock_dev_config) {
            .driver_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_ROS_CLOCK,
                CFGD_MOD_ROS_CLOCK_EIDX_IO),
            .api_id = FWK_ID_API_INIT(FWK_MODULE_IDX_ROS_CLOCK,
                MOD_ROS_CLOCK_API_TYPE_CLOCK),
        }),
    },
    [CFGD_MOD_CLOCK_EIDX_PERIPH] = {
        .name = "Periph",
        .data = &((struct mod_clock_dev_config) {
            .driver_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_ROS_CLOCK,
                CFGD_MOD_ROS_CLOCK_EIDX_PERIPH),
            .api_id = FWK_ID_API_INIT(FWK_MODULE_IDX_ROS_CLOCK,
                MOD_ROS_CLOCK_API_TYPE_CLOCK),
        }),
    },
    [CFGD_MOD_CLOCK_EIDX_RSE] = {
        .name = "RSE",
        .data = &((struct mod_clock_dev_config) {
            .driver_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_ROS_CLOCK,
                CFGD_MOD_ROS_CLOCK_EIDX_RSE),
            .api_id = FWK_ID_API_INIT(FWK_MODULE_IDX_ROS_CLOCK,
                MOD_ROS_CLOCK_API_TYPE_CLOCK),
        }),
    },
    [CFGD_MOD_CLOCK_EIDX_SI] = {
        .name = "SI",
        .data = &((struct mod_clock_dev_config) {
            .driver_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_ROS_CLOCK,
                CFGD_MOD_ROS_CLOCK_EIDX_SI),
            .api_id = FWK_ID_API_INIT(FWK_MODULE_IDX_ROS_CLOCK,
                MOD_ROS_CLOCK_API_TYPE_CLOCK),
        }),
    },
    [CFGD_MOD_CLOCK_EIDX_SMD] = {
        .name = "SMD",
        .data = &((struct mod_clock_dev_config) {
            .driver_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_ROS_CLOCK,
                CFGD_MOD_ROS_CLOCK_EIDX_SMD),
            .api_id = FWK_ID_API_INIT(FWK_MODULE_IDX_ROS_CLOCK,
                MOD_ROS_CLOCK_API_TYPE_CLOCK),
        }),
    },
    [CFGD_MOD_CLOCK_EIDX_DBG] = {
        .name = "DBG",
        .data = &((struct mod_clock_dev_config) {
            .driver_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_ROS_CLOCK,
                CFGD_MOD_ROS_CLOCK_EIDX_DBG),
            .api_id = FWK_ID_API_INIT(FWK_MODULE_IDX_ROS_CLOCK,
                MOD_ROS_CLOCK_API_TYPE_CLOCK),
        }),
    },
    [CFGD_MOD_CLOCK_EIDX_TRACE] = {
        .name = "SWCLKTCK",
        .data = &((struct mod_clock_dev_config) {
            .driver_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_ROS_CLOCK,
                CFGD_MOD_ROS_CLOCK_EIDX_TRACE),
            .api_id = FWK_ID_API_INIT(FWK_MODULE_IDX_ROS_CLOCK,
                MOD_ROS_CLOCK_API_TYPE_CLOCK),
        }),
    },
    [CFGD_MOD_CLOCK_EIDX_COUNT] = { 0 },
};

static const struct fwk_element *clock_get_dev_desc_table(fwk_id_t module_id)
{
    unsigned int i;

    for (i = 0; i < CFGD_MOD_CLOCK_EIDX_COUNT; i++) {
        struct mod_clock_dev_config *dev_config =
            (struct mod_clock_dev_config *)clock_dev_table[i].data;
        dev_config->pd_source_id = fwk_id_build_element_id(
            fwk_module_id_power_domain,
            platform_get_core_count() + platform_get_cluster_count() +
                PD_STATIC_DEV_IDX_SYSTOP);
    }

    return clock_dev_table;
}

const struct fwk_module_config config_clock = {
    .data =
        &(struct mod_clock_config){
            .pd_transition_notification_id = FWK_ID_NOTIFICATION_INIT(
                FWK_MODULE_IDX_POWER_DOMAIN,
                MOD_PD_NOTIFICATION_IDX_POWER_STATE_TRANSITION),
            .pd_pre_transition_notification_id = FWK_ID_NOTIFICATION_INIT(
                FWK_MODULE_IDX_POWER_DOMAIN,
                MOD_PD_NOTIFICATION_IDX_POWER_STATE_PRE_TRANSITION),
        },

    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(clock_get_dev_desc_table),
};
