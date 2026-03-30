/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "internal/cluster_control_reg.h"

#include <mod_cluster_control.h>

#include <fwk_assert.h>
#include <fwk_id.h>
#include <fwk_log.h>
#include <fwk_macros.h>
#include <fwk_module.h>
#include <fwk_notification.h>

#include <stdint.h>

#define MOD_NAME "[CLUSTER_CONTROL] "

static int cluster_control_configure(
    const struct mod_cluster_control_config *config)
{
    unsigned int cluster_idx;
    struct cluster_control_reg *reg;
    uint32_t rvbar_lw, rvbar_up;

    /* Compute the lower and upper field values from the 64-bit configuration
     * value */
    rvbar_lw = (uint32_t)(config->rvbar & UINT32_MAX);
    rvbar_up = (uint32_t)(config->rvbar >> 32);

    for (cluster_idx = 0; cluster_idx < config->region_count; cluster_idx++) {
        reg = (struct cluster_control_reg *)config->regions[cluster_idx];

        reg->PE0_RVBARADDR_LW = rvbar_lw;
        reg->PE0_RVBARADDR_UP = rvbar_up;
        reg->PE1_RVBARADDR_LW = rvbar_lw;
        reg->PE1_RVBARADDR_UP = rvbar_up;
        reg->PE2_RVBARADDR_LW = rvbar_lw;
        reg->PE2_RVBARADDR_UP = rvbar_up;
        reg->PE3_RVBARADDR_LW = rvbar_lw;
        reg->PE3_RVBARADDR_UP = rvbar_up;
    }

    FWK_LOG_INFO(MOD_NAME "Cluster control registers initialized");

    return FWK_SUCCESS;
}

static int cluster_control_init(
    fwk_id_t module_id,
    unsigned int element_count,
    const void *data)
{
    fwk_assert(element_count == 0);
    fwk_assert(data != NULL);

    return FWK_SUCCESS;
}

static int cluster_control_start(fwk_id_t id)
{
    int status;

    const struct mod_cluster_control_config *config = fwk_module_get_data(id);

    if ((fwk_id_type_is_valid(config->platform_notification.source_id)) &&
        (!fwk_id_is_equal(
            config->platform_notification.source_id, FWK_ID_NONE))) {
        status = fwk_notification_subscribe(
            config->platform_notification.notification_id,
            config->platform_notification.source_id,
            id);
        if (status != FWK_SUCCESS) {
            FWK_LOG_CRIT(MOD_NAME
                         "Failed to subscribe platform "
                         "notification");
        }

        return status;
    }

    return cluster_control_configure(config);
}

static int cluster_control_process_notification(
    const struct fwk_event *event,
    struct fwk_event *resp_event)
{
    const struct mod_cluster_control_config *config =
        fwk_module_get_data(fwk_module_id_cluster_control);
    int status = FWK_SUCCESS;

    fwk_assert(fwk_id_is_type(event->target_id, FWK_ID_TYPE_MODULE));

    if (fwk_id_is_equal(
            event->id, config->platform_notification.notification_id)) {
        status = fwk_notification_unsubscribe(
            event->id, event->source_id, event->target_id);
        if (status != FWK_SUCCESS) {
            FWK_LOG_CRIT(MOD_NAME
                         "Failed to unsubscribe platform "
                         "notification");
            return status;
        }

        status = cluster_control_configure(config);
    }

    return status;
}

const struct fwk_module module_cluster_control = {
    .type = FWK_MODULE_TYPE_DRIVER,
    .init = cluster_control_init,
    .start = cluster_control_start,
    .process_notification = cluster_control_process_notification,
};
