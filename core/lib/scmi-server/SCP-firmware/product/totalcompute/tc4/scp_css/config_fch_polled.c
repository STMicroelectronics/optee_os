/*
 * Arm SCP/MCP Software
 * Copyright (c) 2023-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "tc4_dvfs.h"
#include "tc4_scmi.h"
#include "tc4_timer.h"
#include "tc_core.h"
#include "tc_scmi_perf.h"

#include <mod_fch_polled.h>

#include <fwk_element.h>
#include <fwk_id.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>

/* get the channel offset by itself and add the AP location */
#define FC_LEVEL_SET_AP_ADDR(PERF_IDX) \
    (FC_LEVEL_SET_ADDR(PERF_IDX) - SCP_SCMI_PAYLOAD_NS_A2P_BASE + \
     SCP_ATU_PHY_ADDR_SHARED_NSRAM)

#define FC_LIMIT_SET_AP_ADDR(PERF_IDX) \
    (FC_LIMIT_SET_ADDR(PERF_IDX) - SCP_SCMI_PAYLOAD_NS_A2P_BASE + \
     SCP_ATU_PHY_ADDR_SHARED_NSRAM)

#define FC_LEVEL_GET_AP_ADDR(PERF_IDX) \
    (FC_LEVEL_GET_ADDR(PERF_IDX) - SCP_SCMI_PAYLOAD_NS_A2P_BASE + \
     SCP_ATU_PHY_ADDR_SHARED_NSRAM)

#define FC_LIMIT_GET_AP_ADDR(PERF_IDX) \
    (FC_LIMIT_GET_ADDR(PERF_IDX) - SCP_SCMI_PAYLOAD_NS_A2P_BASE + \
     SCP_ATU_PHY_ADDR_SHARED_NSRAM)

static struct mod_fch_polled_config module_config = {
    .fch_alarm_id = FWK_ID_SUB_ELEMENT_INIT(
        FWK_MODULE_IDX_TIMER,
        0,
        TC4_CONFIG_TIMER_FAST_CHANNEL_TIMER_IDX),
    .fch_poll_rate = FCH_MIN_POLL_RATE_US,
    .rate_limit = (4 * 1000),
    .attributes = 0,
};

static const struct fwk_element fch_polled_element_table[] = {
    [TC4_PLAT_FCH_GROUP_LITTLE_LEVEL_SET] = {
        .name = "FCH_" TC4_GROUP_LITTLE_NAME "_LEVEL_SET",
        .data = FCH_ADDR_INIT(
            FC_LEVEL_SET_ADDR(DVFS_ELEMENT_IDX_GROUP_LITTLE),
            FC_LEVEL_SET_AP_ADDR(DVFS_ELEMENT_IDX_GROUP_LITTLE),
            FCH_POLLED_LEVEL_SET_LENGTH
            )
    },
    [TC4_PLAT_FCH_GROUP_LITTLE_LIMIT_SET] = {
        .name = "FCH_" TC4_GROUP_LITTLE_NAME "_LIMIT_SET",
        .data =FCH_ADDR_INIT(
            FC_LIMIT_SET_ADDR(DVFS_ELEMENT_IDX_GROUP_LITTLE),
            FC_LIMIT_SET_AP_ADDR(DVFS_ELEMENT_IDX_GROUP_LITTLE),
            FCH_POLLED_LIMIT_SET_LENGTH
            ),
    },
    [TC4_PLAT_FCH_GROUP_LITTLE_LEVEL_GET] = {
        .name = "FCH_" TC4_GROUP_LITTLE_NAME "_LEVEL_GET",
        .data = FCH_ADDR_INIT(
            FC_LEVEL_GET_ADDR(DVFS_ELEMENT_IDX_GROUP_LITTLE),
            FC_LEVEL_GET_AP_ADDR(DVFS_ELEMENT_IDX_GROUP_LITTLE),
            FCH_POLLED_LEVEL_GET_LENGTH
            )
    },
    [TC4_PLAT_FCH_GROUP_LITTLE_LIMIT_GET] = {
        .name = "FCH_" TC4_GROUP_LITTLE_NAME "_LIMIT_GET",
        .data = FCH_ADDR_INIT(
            FC_LIMIT_GET_ADDR(DVFS_ELEMENT_IDX_GROUP_LITTLE),
            FC_LIMIT_GET_AP_ADDR(DVFS_ELEMENT_IDX_GROUP_LITTLE),
            FCH_POLLED_LIMIT_GET_LENGTH
            )
    },
    [TC4_PLAT_FCH_GROUP_MID_LEVEL_SET] = {
        .name = "FCH_" TC4_GROUP_MID_NAME "_LEVEL_SET",
        .data = FCH_ADDR_INIT(
            FC_LEVEL_SET_ADDR(DVFS_ELEMENT_IDX_GROUP_MID),
            FC_LEVEL_SET_AP_ADDR(DVFS_ELEMENT_IDX_GROUP_MID),
            FCH_POLLED_LEVEL_SET_LENGTH
            )
    },
    [TC4_PLAT_FCH_GROUP_MID_LIMIT_SET] = {
        .name = "FCH_" TC4_GROUP_MID_NAME "_LIMIT_SET",
        .data =FCH_ADDR_INIT(
            FC_LIMIT_SET_ADDR(DVFS_ELEMENT_IDX_GROUP_MID),
            FC_LIMIT_SET_AP_ADDR(DVFS_ELEMENT_IDX_GROUP_MID),
            FCH_POLLED_LIMIT_SET_LENGTH
            ),
    },
    [TC4_PLAT_FCH_GROUP_MID_LEVEL_GET] = {
        .name = "FCH_" TC4_GROUP_MID_NAME "_LEVEL_GET",
        .data = FCH_ADDR_INIT(
            FC_LEVEL_GET_ADDR(DVFS_ELEMENT_IDX_GROUP_MID),
            FC_LEVEL_GET_AP_ADDR(DVFS_ELEMENT_IDX_GROUP_MID),
            FCH_POLLED_LEVEL_GET_LENGTH
            )
    },
    [TC4_PLAT_FCH_GROUP_MID_LIMIT_GET] = {
        .name = "FCH_" TC4_GROUP_MID_NAME "_LIMIT_GET",
        .data = FCH_ADDR_INIT(
            FC_LIMIT_GET_ADDR(DVFS_ELEMENT_IDX_GROUP_MID),
            FC_LIMIT_GET_AP_ADDR(DVFS_ELEMENT_IDX_GROUP_MID),
            FCH_POLLED_LIMIT_GET_LENGTH
            )
    },
    [TC4_PLAT_FCH_GROUP_BIG_LEVEL_SET] = {
        .name = "FCH_" TC4_GROUP_BIG_NAME "_LEVEL_SET",
        .data = FCH_ADDR_INIT(
            FC_LEVEL_SET_ADDR(DVFS_ELEMENT_IDX_GROUP_BIG),
            FC_LEVEL_SET_AP_ADDR(DVFS_ELEMENT_IDX_GROUP_BIG),
            FCH_POLLED_LEVEL_SET_LENGTH
            )
    },
    [TC4_PLAT_FCH_GROUP_BIG_LIMIT_SET] = {
        .name = "FCH_" TC4_GROUP_BIG_NAME "_LIMIT_SET",
        .data =FCH_ADDR_INIT(
            FC_LIMIT_SET_ADDR(DVFS_ELEMENT_IDX_GROUP_BIG),
            FC_LIMIT_SET_AP_ADDR(DVFS_ELEMENT_IDX_GROUP_BIG),
            FCH_POLLED_LIMIT_SET_LENGTH
            ),
    },
    [TC4_PLAT_FCH_GROUP_BIG_LEVEL_GET] = {
        .name = "FCH_" TC4_GROUP_BIG_NAME "_LEVEL_GET",
        .data = FCH_ADDR_INIT(
            FC_LEVEL_GET_ADDR(DVFS_ELEMENT_IDX_GROUP_BIG),
            FC_LEVEL_GET_AP_ADDR(DVFS_ELEMENT_IDX_GROUP_BIG),
            FCH_POLLED_LEVEL_GET_LENGTH
            )
    },
    [TC4_PLAT_FCH_GROUP_BIG_LIMIT_GET] = {
        .name = "FCH_" TC4_GROUP_BIG_NAME "_LIMIT_GET",
        .data = FCH_ADDR_INIT(
            FC_LIMIT_GET_ADDR(DVFS_ELEMENT_IDX_GROUP_BIG),
            FC_LIMIT_GET_AP_ADDR(DVFS_ELEMENT_IDX_GROUP_BIG),
            FCH_POLLED_LIMIT_GET_LENGTH
            )
    },
    [TC4_PLAT_FCH_GPU_LEVEL_SET] = {
        .name = "FCH_GPU_LEVEL_SET",
        .data = FCH_ADDR_INIT(
            FC_LEVEL_SET_ADDR(DVFS_ELEMENT_IDX_GPU),
            FC_LEVEL_SET_AP_ADDR(DVFS_ELEMENT_IDX_GPU),
            FCH_POLLED_LEVEL_SET_LENGTH
            )
    },
    [TC4_PLAT_FCH_GPU_LIMIT_SET] = {
        .name = "FCH_GPU_LIMIT_SET",
        .data =FCH_ADDR_INIT(
            FC_LIMIT_SET_ADDR(DVFS_ELEMENT_IDX_GPU),
            FC_LIMIT_SET_AP_ADDR(DVFS_ELEMENT_IDX_GPU),
            FCH_POLLED_LIMIT_SET_LENGTH
            ),
    },
    [TC4_PLAT_FCH_GPU_LEVEL_GET] = {
        .name = "FCH_GPU_LEVEL_GET",
        .data = FCH_ADDR_INIT(
            FC_LEVEL_GET_ADDR(DVFS_ELEMENT_IDX_GPU),
            FC_LEVEL_GET_AP_ADDR(DVFS_ELEMENT_IDX_GPU),
            FCH_POLLED_LEVEL_GET_LENGTH
            )
    },
    [TC4_PLAT_FCH_GPU_LIMIT_GET] = {
        .name = "FCH_GPU_LIMIT_GET",
        .data = FCH_ADDR_INIT(
            FC_LIMIT_GET_ADDR(DVFS_ELEMENT_IDX_GPU),
            FC_LIMIT_GET_AP_ADDR(DVFS_ELEMENT_IDX_GPU),
            FCH_POLLED_LIMIT_GET_LENGTH
            )
    },

    [TC4_PLAT_FCH_COUNT] = {0},
};

static const struct fwk_element *fch_polled_get_element_table(
    fwk_id_t module_id)
{
    return fch_polled_element_table;
}

const struct fwk_module_config config_fch_polled = {
    .data = &module_config,
    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(fch_polled_get_element_table),
};
