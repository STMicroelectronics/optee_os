/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <internal/perf_controller.h>

#include <interface_power_management.h>

#include <fwk_assert.h>
#include <fwk_mm.h>
#include <fwk_module.h>
#include <fwk_notification.h>

static struct mod_perf_controller_ctx perf_controller_ctx;

static struct mod_perf_controller_internal_api internal_api;

static uint32_t get_limiters_min_power_limit(
    struct mod_perf_controller_domain_ctx *domain_ctx)
{
    unsigned int limiter_idx;
    struct mod_perf_controller_limiter_ctx *limiter_ctx;
    uint32_t min_power_limit = UINT32_MAX;

    for (limiter_idx = 0U; limiter_idx < domain_ctx->limiter_count;
         limiter_idx++) {
        limiter_ctx = &(domain_ctx->limiter_ctx_table[limiter_idx]);
        min_power_limit = FWK_MIN(min_power_limit, limiter_ctx->power_limit);
    }

    return min_power_limit;
}

static int domain_apply_performance_granted(
    struct mod_perf_controller_domain_ctx *domain_ctx)
{
    uint32_t min_power_limit;
    uint32_t performance_limit;
    uint32_t requested_performance;
    uintptr_t cookie;
    int status;

    min_power_limit = internal_api.get_limiters_min_power_limit(domain_ctx);
    status = domain_ctx->power_model_api->power_to_performance(
        domain_ctx->config->power_model_id,
        min_power_limit,
        &performance_limit);

    if (status != FWK_SUCCESS) {
        return status;
    }

    domain_ctx->performance_limit = performance_limit;

    if (domain_ctx->performance_request_details.level <= performance_limit) {
        cookie = domain_ctx->performance_request_details.cookie;
        requested_performance = domain_ctx->performance_request_details.level;
    } else {
        cookie = 0U;
        requested_performance = performance_limit;
    }

    return domain_ctx->perf_driver_api->set_performance_level(
        domain_ctx->config->performance_driver_id,
        cookie,
        requested_performance);
}

static struct mod_perf_controller_internal_api internal_api = {
    .get_limiters_min_power_limit = get_limiters_min_power_limit,
    .domain_apply_performance_granted = domain_apply_performance_granted,
};

static int mod_perf_controller_set_performance_level(
    fwk_id_t domain_id,
    uintptr_t cookie,
    uint32_t performance_level)
{
    int status;
    struct mod_perf_controller_domain_ctx *domain_ctx;

    domain_ctx = &perf_controller_ctx
                      .domain_ctx_table[fwk_id_get_element_idx(domain_id)];

    domain_ctx->performance_request_details.level = performance_level;

    if (performance_level <= domain_ctx->performance_limit) {
        status = domain_ctx->perf_driver_api->set_performance_level(
            domain_ctx->config->performance_driver_id,
            cookie,
            performance_level);
    } else {
        status = FWK_PENDING;
        domain_ctx->performance_request_details.cookie = cookie;
    }

    return status;
}

static struct mod_perf_controller_perf_api perf_api = {
    .set_performance_level = mod_perf_controller_set_performance_level,
};

static int mod_perf_controller_set_power_limit(
    fwk_id_t limiter_id,
    uint32_t power_limit)
{
    unsigned int limiter_idx;
    unsigned int domain_idx;
    struct mod_perf_controller_limiter_ctx *limiter_ctx;

    if (!fwk_module_is_valid_sub_element_id(limiter_id)) {
        return FWK_E_PARAM;
    }

    domain_idx = fwk_id_get_element_idx(limiter_id);
    limiter_idx = fwk_id_get_sub_element_idx(limiter_id);

    limiter_ctx = &perf_controller_ctx.domain_ctx_table[domain_idx]
                       .limiter_ctx_table[limiter_idx];

    limiter_ctx->power_limit = power_limit;

    return FWK_SUCCESS;
}

static struct interface_power_management_api power_api = {
    .set_power_limit = mod_perf_controller_set_power_limit,
};

static int mod_perf_controller_apply_performance_granted(void)
{
    uint32_t domain_idx;
    struct mod_perf_controller_domain_ctx *domain_ctx;
    int status = FWK_SUCCESS;

    for (domain_idx = 0U; (domain_idx < perf_controller_ctx.domain_count) &&
         (status == FWK_SUCCESS);
         domain_idx++) {
        domain_ctx = &perf_controller_ctx.domain_ctx_table[domain_idx];
        status = internal_api.domain_apply_performance_granted(domain_ctx);
    }
    return status;
}

static struct mod_perf_controller_apply_performance_granted_api
    apply_performance_granted_api = {
        .apply_performance_granted =
            mod_perf_controller_apply_performance_granted,
    };

/*
 * Framework handlers
 */
static int mod_perf_controller_init(
    fwk_id_t module_id,
    unsigned int element_count,
    const void *data)
{
    if (element_count == 0U) {
        return FWK_E_PARAM;
    }

    perf_controller_ctx.domain_count = element_count;

    perf_controller_ctx.domain_ctx_table = fwk_mm_calloc(
        element_count, sizeof(struct mod_perf_controller_domain_ctx));

    return FWK_SUCCESS;
}

static int mod_perf_controller_element_init(
    fwk_id_t element_id,
    unsigned int sub_element_count,
    const void *data)
{
    struct mod_perf_controller_domain_ctx *domain_ctx;
    const struct mod_perf_controller_domain_config *domain_config;

    if ((sub_element_count == 0U) || (data == NULL)) {
        return FWK_E_PARAM;
    }

    domain_ctx = &perf_controller_ctx
                      .domain_ctx_table[fwk_id_get_element_idx(element_id)];

    domain_ctx->limiter_ctx_table = fwk_mm_calloc(
        sub_element_count, sizeof(struct mod_perf_controller_limiter_ctx));

    domain_config = (const struct mod_perf_controller_domain_config *)data;

    domain_ctx->config = domain_config;

    domain_ctx->limiter_count = sub_element_count;

    domain_ctx->performance_limit = domain_config->initial_performance_limit;

    return FWK_SUCCESS;
}

#ifdef BUILD_HAS_NOTIFICATION
static int mod_perf_controller_process_event(
    const struct fwk_event *event,
    struct fwk_event *resp_event)
{
    int status;
    fwk_id_t domain_id;
    unsigned int event_idx;
    struct mod_perf_controller_domain_ctx *domain_ctx;
    struct mod_perf_controller_event_drv_resp_params *drv_response_params;
    struct mod_perf_controller_notification_params *notification_params;
    uint32_t power_limit;

    event_idx = fwk_id_get_event_idx(event->id);

    if (event_idx == MOD_PERF_CONTROLLER_EVENT_IDX_DRIVER_RESPONSE) {
        domain_id = event->target_id;
        domain_ctx = &perf_controller_ctx
                          .domain_ctx_table[fwk_id_get_element_idx(domain_id)];

        drv_response_params =
            (struct mod_perf_controller_event_drv_resp_params *)event->params;

        struct fwk_event outbound_notification = {
            .id = FWK_ID_NOTIFICATION_INIT(
                FWK_MODULE_IDX_PERF_CONTROLLER,
                MOD_PERF_CONTROLLER_NOTIFICATION_IDX_PERF_SET),
            .source_id = domain_id,
        };

        notification_params = (struct mod_perf_controller_notification_params *)
                                  outbound_notification.params;

        status = domain_ctx->power_model_api->performance_to_power(
            domain_ctx->config->power_model_id,
            drv_response_params->performance_level,
            &power_limit);
        if (status != FWK_SUCCESS) {
            return status;
        }

        notification_params->performance_level =
            drv_response_params->performance_level;
        notification_params->power_limit = power_limit;

        return fwk_notification_notify(
            &outbound_notification, &domain_ctx->notification_count);
    }

    return FWK_SUCCESS;
}
#endif

static int mod_perf_controller_bind(fwk_id_t id, unsigned int round)
{
    int status;
    struct mod_perf_controller_domain_ctx *domain_ctx;

    if ((round != 0U) || (fwk_id_is_type(id, FWK_ID_TYPE_MODULE))) {
        return FWK_SUCCESS;
    }

    domain_ctx =
        &perf_controller_ctx.domain_ctx_table[fwk_id_get_element_idx(id)];

    status = fwk_module_bind(
        domain_ctx->config->performance_driver_id,
        domain_ctx->config->performance_driver_api_id,
        &domain_ctx->perf_driver_api);

    if (status != FWK_SUCCESS) {
        return status;
    }

    return fwk_module_bind(
        domain_ctx->config->power_model_id,
        domain_ctx->config->power_model_api_id,
        &domain_ctx->power_model_api);
}

static int mod_perf_controller_process_bind_request(
    fwk_id_t source_id,
    fwk_id_t target_id,
    fwk_id_t api_id,
    const void **api)
{
    int status;
    enum mod_perf_controller_api_idx api_idx;

    if (fwk_id_get_module_idx(api_id) != FWK_MODULE_IDX_PERF_CONTROLLER) {
        return FWK_E_PARAM;
    }

    api_idx = (enum mod_perf_controller_api_idx)fwk_id_get_api_idx(api_id);

    switch (api_idx) {
    case MOD_PERF_CONTROLLER_DOMAIN_PERF_API:
        *api = &perf_api;
        status = FWK_SUCCESS;
        break;
    case MOD_PERF_CONTROLLER_LIMITER_POWER_API:
        *api = &power_api;
        status = FWK_SUCCESS;
        break;
    case MOD_PERF_CONTROLLER_APPLY_PERFORMANCE_GRANTED_API:
        *api = &apply_performance_granted_api;
        status = FWK_SUCCESS;
        break;
    default:
        status = FWK_E_PARAM;
        break;
    }

    return status;
}

const struct fwk_module module_perf_controller = {
    .type = FWK_MODULE_TYPE_HAL,
    .api_count = (uint32_t)MOD_PERF_CONTROLLER_API_COUNT,
    .event_count = (uint32_t)MOD_PERF_CONTROLLER_EVENT_IDX_COUNT,
#ifdef BUILD_HAS_NOTIFICATION
    .notification_count = (uint32_t)MOD_PERF_CONTROLLER_NOTIFICATION_IDX_COUNT,
#endif
    .init = mod_perf_controller_init,
    .element_init = mod_perf_controller_element_init,
    .bind = mod_perf_controller_bind,
    .process_bind_request = mod_perf_controller_process_bind_request,
#ifdef BUILD_HAS_NOTIFICATION
    .process_event = mod_perf_controller_process_event,
#endif
};
