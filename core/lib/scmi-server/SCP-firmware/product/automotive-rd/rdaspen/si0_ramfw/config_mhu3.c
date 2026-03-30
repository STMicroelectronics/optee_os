/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Configuration data for module 'mhu3'.
 */

#include "si0_cfgd_mhu3.h"
#include "si0_irq.h"
#include "si0_mmap.h"

#include <mod_mhu3.h>

#include <fwk_element.h>
#include <fwk_id.h>
#include <fwk_module.h>

/*
 * Timeout to wait for the receiver to clear the MHUv3 channel status
 * register so the channel can become available again.
 */
#define RESP_WAIT_TIMEOUT_US (30 * 1000)

/* SI0<-->RSE Secure MHUv3 Doorbell channel configuration */
struct mod_mhu3_channel_config si02rse_s_dbch_config[] = {
    /* PBX CH 0, FLAG 0, MBX CH 0, FLAG 0 not used */
    /* PBX CH 0, FLAG 1, MBX CH 0, FLAG 1 for RSE A2P SCP */
    [0] = MOD_MHU3_INIT_DBCH(0, 1, 0, 1),
#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
    /* PBX CH 0, FLAG 2, MBX CH 0, FLAG 2 for SCP P2A RSE */
    [1] = MOD_MHU3_INIT_DBCH(0, 2, 0, 2),
#endif
};

/* SI0<->AP Secure MHUv3 doorbell channel configuration */
struct mod_mhu3_channel_config
    si02ap_s_dbch_config[SI0_CFGD_MOD_MHU3_SI0_AP_S_EIDX_COUNT] = {
        /* PBX CH 0, FLAG 0, MBX CH 0, FLAG 0 */
        [SI0_CFGD_MOD_MHU3_SI0_AP_S_EIDX_PSCI] = MOD_MHU3_INIT_DBCH(0, 0, 0, 0),
        /* PBX CH 1, FLAG 0, MBX CH 1, FLAG 0 */
        [SI0_CFGD_MOD_MHU3_SI0_AP_S_EIDX_PFDI_MONITOR_AP_CLUSTER_0_CORE_0] =
            MOD_MHU3_INIT_DBCH(1, 0, 1, 0),
        /* PBX CH 2, FLAG 0, MBX CH 2, FLAG 0 */
        [SI0_CFGD_MOD_MHU3_SI0_AP_S_EIDX_PFDI_MONITOR_AP_CLUSTER_0_CORE_1] =
            MOD_MHU3_INIT_DBCH(2, 0, 2, 0),
        /* PBX CH 3, FLAG 0, MBX CH 3, FLAG 0 */
        [SI0_CFGD_MOD_MHU3_SI0_AP_S_EIDX_PFDI_MONITOR_AP_CLUSTER_0_CORE_2] =
            MOD_MHU3_INIT_DBCH(3, 0, 3, 0),
        /* PBX CH 4, FLAG 0, MBX CH 4, FLAG 0 */
        [SI0_CFGD_MOD_MHU3_SI0_AP_S_EIDX_PFDI_MONITOR_AP_CLUSTER_0_CORE_3] =
            MOD_MHU3_INIT_DBCH(4, 0, 4, 0),
        /* PBX CH 5, FLAG 0, MBX CH 5, FLAG 0 */
        [SI0_CFGD_MOD_MHU3_SI0_AP_S_EIDX_PFDI_MONITOR_AP_CLUSTER_1_CORE_0] =
            MOD_MHU3_INIT_DBCH(5, 0, 5, 0),
        /* PBX CH 6, FLAG 0, MBX CH 6, FLAG 0 */
        [SI0_CFGD_MOD_MHU3_SI0_AP_S_EIDX_PFDI_MONITOR_AP_CLUSTER_1_CORE_1] =
            MOD_MHU3_INIT_DBCH(6, 0, 6, 0),
        /* PBX CH 7, FLAG 0, MBX CH 7, FLAG 0 */
        [SI0_CFGD_MOD_MHU3_SI0_AP_S_EIDX_PFDI_MONITOR_AP_CLUSTER_1_CORE_2] =
            MOD_MHU3_INIT_DBCH(7, 0, 7, 0),
        /* PBX CH 8, FLAG 0, MBX CH 8, FLAG 0 */
        [SI0_CFGD_MOD_MHU3_SI0_AP_S_EIDX_PFDI_MONITOR_AP_CLUSTER_1_CORE_3] =
            MOD_MHU3_INIT_DBCH(8, 0, 8, 0),
        /* PBX CH 9, FLAG 0, MBX CH 9, FLAG 0 */
        [SI0_CFGD_MOD_MHU3_SI0_AP_S_EIDX_PFDI_MONITOR_AP_CLUSTER_2_CORE_0] =
            MOD_MHU3_INIT_DBCH(9, 0, 9, 0),
        /* PBX CH 10, FLAG 0, MBX CH 10, FLAG 0 */
        [SI0_CFGD_MOD_MHU3_SI0_AP_S_EIDX_PFDI_MONITOR_AP_CLUSTER_2_CORE_1] =
            MOD_MHU3_INIT_DBCH(10, 0, 10, 0),
        /* PBX CH 11, FLAG 0, MBX CH 11, FLAG 0 */
        [SI0_CFGD_MOD_MHU3_SI0_AP_S_EIDX_PFDI_MONITOR_AP_CLUSTER_2_CORE_2] =
            MOD_MHU3_INIT_DBCH(11, 0, 11, 0),
        /* PBX CH 12, FLAG 0, MBX CH 12, FLAG 0 */
        [SI0_CFGD_MOD_MHU3_SI0_AP_S_EIDX_PFDI_MONITOR_AP_CLUSTER_2_CORE_3] =
            MOD_MHU3_INIT_DBCH(12, 0, 12, 0),
        /* PBX CH 13, FLAG 0, MBX CH 13, FLAG 0 */
        [SI0_CFGD_MOD_MHU3_SI0_AP_S_EIDX_PFDI_MONITOR_AP_CLUSTER_3_CORE_0] =
            MOD_MHU3_INIT_DBCH(13, 0, 13, 0),
        /* PBX CH 14, FLAG 0, MBX CH 14, FLAG 0 */
        [SI0_CFGD_MOD_MHU3_SI0_AP_S_EIDX_PFDI_MONITOR_AP_CLUSTER_3_CORE_1] =
            MOD_MHU3_INIT_DBCH(14, 0, 14, 0),
        /* PBX CH 15, FLAG 0, MBX CH 15, FLAG 0 */
        [SI0_CFGD_MOD_MHU3_SI0_AP_S_EIDX_PFDI_MONITOR_AP_CLUSTER_3_CORE_2] =
            MOD_MHU3_INIT_DBCH(15, 0, 15, 0),
        /* PBX CH 16, FLAG 0, MBX CH 16, FLAG 0 */
        [SI0_CFGD_MOD_MHU3_SI0_AP_S_EIDX_PFDI_MONITOR_AP_CLUSTER_3_CORE_3] =
            MOD_MHU3_INIT_DBCH(16, 0, 16, 0),
    };

/* Module element table */
static const struct fwk_element mhu_element_table[]  = {
    [SI0_CFGD_MOD_MHU3_EIDX_SI0_RSE] = {
        .name = "SI02RSE_S_MHU_DBCH",
        .sub_element_count = FWK_ARRAY_SIZE(si02rse_s_dbch_config),
        .data = &(struct mod_mhu3_device_config) {
            .irq = (unsigned int) CL0_MHU3_RSE2SI0_IRQ,
            .in = SI0_RSE2SI0_MHUV3_RCV_BASE,
            .out = SI0_SI02RSE_MHUV3_SEND_BASE,
            .channels = si02rse_s_dbch_config,
            .timer_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_TIMER, 0),
            .resp_wait_timeout_us = RESP_WAIT_TIMEOUT_US,
        },
    },
    [SI0_CFGD_MOD_MHU3_EIDX_SI0_AP_S] = {
        .name = "SI02AP_S_MHU_DBCH",
        .sub_element_count = FWK_ARRAY_SIZE(si02ap_s_dbch_config),
        .data = &(struct mod_mhu3_device_config) {
            .irq = (unsigned int) CL0_MHU3_AP2SI0_S_IRQ,
            .in = SI0_AP2SI0_S_MHUV3_RCV_BASE,
            .out = SI0_SI02AP_S_MHUV3_SEND_BASE,
            .channels = si02ap_s_dbch_config,
            .timer_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_TIMER, 0),
            .resp_wait_timeout_us = RESP_WAIT_TIMEOUT_US,
        },
    },
    [SI0_CFGD_MOD_MHU3_EIDX_COUNT] = { 0 },
};

struct fwk_module_config config_mhu3 = {
    .elements = FWK_MODULE_STATIC_ELEMENTS_PTR(mhu_element_table),
};
