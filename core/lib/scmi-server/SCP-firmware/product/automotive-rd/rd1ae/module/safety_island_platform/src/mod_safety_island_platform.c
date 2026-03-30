/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Safety Island Platform Module.
 */

#include "platform_core.h"

#include <mod_power_domain.h>
#include <mod_safety_island_platform.h>
#include <mod_transport.h>

#include <fwk_assert.h>
#include <fwk_id.h>
#include <fwk_log.h>
#include <fwk_mm.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>
#include <fwk_status.h>

#include <stdbool.h>
#include <stdint.h>

#define MOD_NAME "[SAFETY_ISLAND_PLATFORM]"

/* Module context */
struct safety_island_platform_ctx {
    /* Safety Island Cluster config data */
    struct safety_island_cluster_ctx *safety_island_ctx_table;
    /* Number of Safety Island clusters */
    uint32_t safety_island_cluster_count;
};

static struct safety_island_platform_ctx ctx;

static int init_si_cluster_cores(fwk_id_t safety_island_cluster_idx)
{
    uint32_t pd_state;
    uint32_t core;
    struct safety_island_cluster_ctx *safety_island_cluster_ctx =
        &ctx.safety_island_ctx_table[fwk_id_get_element_idx(
            safety_island_cluster_idx)];
    uint32_t cluster_offset =
        safety_island_cluster_ctx->config->cluster_layout.core_offset;
    uint32_t num_cores =
        safety_island_cluster_ctx->config->cluster_layout.num_cores;

    uint32_t start_id = platform_get_core_count() +
        platform_get_cluster_count() + cluster_offset;

    /* Composite Power Domain state to be set for the Safety Island */
    pd_state = MOD_PD_COMPOSITE_STATE(
        MOD_PD_LEVEL_1, 0, 0, MOD_PD_STATE_ON, MOD_PD_STATE_ON);

    for (core = 0; core < num_cores; core++) {
        fwk_id_t pd_id =
            FWK_ID_ELEMENT(FWK_MODULE_IDX_POWER_DOMAIN, start_id + core);
        int status = safety_island_cluster_ctx->pd_restricted_api->set_state(
            pd_id, false, pd_state);
        if (status != FWK_SUCCESS) {
            FWK_LOG_ERR(
                MOD_NAME "Failed to intialize %s core %lu",
                fwk_module_get_element_name(safety_island_cluster_idx),
                (unsigned long)core);
            return status;
        }
    }

    return FWK_SUCCESS;
}

static int signal_error(fwk_id_t safety_island_cluster_idx)
{
    struct safety_island_cluster_ctx *safety_island_cluster_ctx =
        &ctx.safety_island_ctx_table[fwk_id_get_element_idx(
            safety_island_cluster_idx)];
    FWK_LOG_ERR(
        MOD_NAME "Error! Invalid response received to boot %s",
        fwk_module_get_element_name(safety_island_cluster_idx));
    safety_island_cluster_ctx->transport_api->release_transport_channel_lock(
        safety_island_cluster_ctx->config->transport_id);

    return FWK_SUCCESS;
}

static int signal_message(fwk_id_t safety_island_cluster_idx)
{
    int status;
    struct safety_island_cluster_ctx *safety_island_cluster_ctx =
        &ctx.safety_island_ctx_table[fwk_id_get_element_idx(
            safety_island_cluster_idx)];

    FWK_LOG_INFO(
        MOD_NAME "Received doorbell event to boot %s",
        fwk_module_get_element_name(safety_island_cluster_idx));
    FWK_LOG_INFO(
        MOD_NAME "Initializing %s cores...",
        fwk_module_get_element_name(safety_island_cluster_idx));
    status = init_si_cluster_cores(safety_island_cluster_idx);
    if (status != FWK_SUCCESS) {
        FWK_LOG_ERR(
            MOD_NAME "Error! Failed to initialize %s cores",
            fwk_module_get_element_name(safety_island_cluster_idx));
        fwk_trap();
    }
    safety_island_cluster_ctx->transport_api->release_transport_channel_lock(
        safety_island_cluster_ctx->config->transport_id);

    return FWK_SUCCESS;
}

static const struct mod_transport_firmware_signal_api
    platform_transport_signal_api = {
        .signal_error = signal_error,
        .signal_message = signal_message,
    };

/*
 * Framework handlers
 */
static int safety_island_platform_mod_init(
    fwk_id_t module_id,
    unsigned int safety_island_cluster_count,
    const void *unused)
{
    if (safety_island_cluster_count == 0) {
        return FWK_E_PARAM;
    }

    ctx.safety_island_ctx_table = fwk_mm_calloc(
        safety_island_cluster_count, sizeof(ctx.safety_island_ctx_table[0]));

    ctx.safety_island_cluster_count = safety_island_cluster_count;

    return FWK_SUCCESS;
}

static int safety_island_platform_cluster_init(
    fwk_id_t safety_island_cluster_idx,
    unsigned int sub_element_count,
    const void *data)
{
    struct safety_island_cluster_config *config;
    struct safety_island_cluster_ctx *safety_island_cluster_ctx;

    config = (struct safety_island_cluster_config *)data;

    if (config == NULL) {
        return FWK_E_DATA;
    }

    safety_island_cluster_ctx =
        &ctx.safety_island_ctx_table[fwk_id_get_element_idx(
            safety_island_cluster_idx)];
    safety_island_cluster_ctx->config = config;

    return FWK_SUCCESS;
}

static int safety_island_platform_bind(fwk_id_t id, unsigned int round)
{
    struct safety_island_cluster_ctx *safety_island_cluster_ctx;
    fwk_id_t transport_api_id;
    int status;

    if ((round != 0) || (fwk_id_is_type(id, FWK_ID_TYPE_MODULE))) {
        return FWK_SUCCESS;
    }

    safety_island_cluster_ctx =
        &ctx.safety_island_ctx_table[fwk_id_get_element_idx(id)];

    transport_api_id =
        FWK_ID_API(FWK_MODULE_IDX_TRANSPORT, MOD_TRANSPORT_API_IDX_FIRMWARE);

    status = fwk_module_bind(
        safety_island_cluster_ctx->config->transport_id,
        transport_api_id,
        &safety_island_cluster_ctx->transport_api);

    if (status != FWK_SUCCESS) {
        return status;
    }

    return fwk_module_bind(
        fwk_module_id_power_domain,
        mod_pd_api_id_restricted,
        &safety_island_cluster_ctx->pd_restricted_api);

    return FWK_SUCCESS;
}

static int safety_island_platform_process_bind_request(
    fwk_id_t source_id,
    fwk_id_t target_id,
    fwk_id_t api_id,
    const void **api)
{
    int status;
    enum mod_safety_island_platform_api_idx api_id_type;
    struct safety_island_cluster_ctx *safety_island_cluster_ctx;

    if (!fwk_id_is_type(target_id, FWK_ID_TYPE_ELEMENT)) {
        /*  Only binding to an element is allowed */
        fwk_unexpected();
        return FWK_E_ACCESS;
    }

    safety_island_cluster_ctx =
        &ctx.safety_island_ctx_table[fwk_id_get_element_idx(target_id)];

    api_id_type =
        (enum mod_safety_island_platform_api_idx)fwk_id_get_api_idx(api_id);

    switch (api_id_type) {
    case MOD_SAFETY_ISLAND_PLATFORM_API_IDX_BOOT_CLUSTER:
        if (fwk_id_is_equal(
                source_id, safety_island_cluster_ctx->config->transport_id)) {
            *api = &platform_transport_signal_api;
            status = FWK_SUCCESS;
        } else {
            status = FWK_E_PARAM;
        }
        break;
    default:
        status = FWK_E_PARAM;
        break;
    }

    return status;
}

const struct fwk_module module_safety_island_platform = {
    .type = FWK_MODULE_TYPE_HAL,
    .api_count = MOD_SAFETY_ISLAND_PLATFORM_API_COUNT,
    .init = safety_island_platform_mod_init,
    .element_init = safety_island_platform_cluster_init,
    .bind = safety_island_platform_bind,
    .process_bind_request = safety_island_platform_process_bind_request,
};
