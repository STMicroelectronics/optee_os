/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Power capping module.
 */
#include <mod_pid_controller.h>
#include <mod_power_capping.h>

#include <interface_power_management.h>

#include <fwk_core.h>
#include <fwk_event.h>
#include <fwk_mm.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>
#include <fwk_notification.h>

#include <stdbool.h>
#include <stdint.h>

struct pcapping_domain_ctx {
    struct mod_power_capping_domain_config *config;
    uint32_t applied_cap;
    uint32_t requested_cap;
    uint32_t threshold_low;
    uint32_t threshold_high;
    uint32_t cookie;
    unsigned int notifications_sent_count;
    struct mod_power_measurement_driver_api *power_measurement_driver_api;
    struct mod_pid_controller_api *pid_ctrl_api;
};

static struct {
    unsigned int domain_count;
} pcapping_ctx;

static struct pcapping_domain_ctx *pcapping_domain_ctx_table;

static const fwk_id_t mod_pcapping_notification_id_cap_change =
    FWK_ID_NOTIFICATION_INIT(
        FWK_MODULE_IDX_POWER_CAPPING,
        MOD_POWER_CAPPING_NOTIFICATION_IDX_CAP_CHANGE);

#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
static const fwk_id_t mod_pcapping_notification_id_pai_changed =
    FWK_ID_NOTIFICATION_INIT(
        FWK_MODULE_IDX_POWER_CAPPING,
        MOD_POWER_CAPPING_NOTIFICATION_IDX_PAI_CHANGED);

static const fwk_id_t mod_pcapping_notification_id_meas_changed =
    FWK_ID_NOTIFICATION_INIT(
        FWK_MODULE_IDX_POWER_CAPPING,
        MOD_POWER_CAPPING_NOTIFICATION_IDX_MEASUREMENTS_CHANGED);

#endif

static int pcapping_get_ctx(
    fwk_id_t domain_id,
    struct pcapping_domain_ctx **domain_ctx)
{
    unsigned int domain_index = fwk_id_get_element_idx(domain_id);

    if (domain_index >= pcapping_ctx.domain_count) {
        return FWK_E_PARAM;
    }

    *domain_ctx = &(pcapping_domain_ctx_table[domain_index]);
    return FWK_SUCCESS;
}

static int mod_pcapping_request_cap(fwk_id_t domain_id, uint32_t requested_cap)
{
    int status;
    struct pcapping_domain_ctx *domain_ctx;

    status = pcapping_get_ctx(domain_id, &domain_ctx);

    if (status != FWK_SUCCESS) {
        return status;
    }

    domain_ctx->requested_cap = requested_cap;
    domain_ctx->pid_ctrl_api->set_point(
        domain_ctx->config->pid_controller_id, requested_cap);

    return (requested_cap >= domain_ctx->applied_cap) ? FWK_SUCCESS :
                                                        FWK_PENDING;
}

static int mod_pcapping_get_applied_cap(fwk_id_t domain_id, uint32_t *cap)
{
    int status;
    struct pcapping_domain_ctx *domain_ctx;

    status = pcapping_get_ctx(domain_id, &domain_ctx);

    if (status != FWK_SUCCESS) {
        return status;
    }

    *cap = domain_ctx->applied_cap;
    return FWK_SUCCESS;
}

static int mod_pcapping_get_average_power(fwk_id_t domain_id, uint32_t *power)
{
    int status;
    struct pcapping_domain_ctx *domain_ctx;

    status = pcapping_get_ctx(domain_id, &domain_ctx);

    if (status != FWK_SUCCESS) {
        return status;
    }

    status = domain_ctx->power_measurement_driver_api->get_average_power(
        domain_id, power);

    if (status != FWK_SUCCESS) {
        return status;
    }
    return FWK_SUCCESS;
}

static int mod_pcapping_set_averaging_interval(fwk_id_t domain_id, uint32_t pai)
{
    int status;
    struct pcapping_domain_ctx *domain_ctx;

    status = pcapping_get_ctx(domain_id, &domain_ctx);
    if (status != FWK_SUCCESS) {
        return status;
    }

    status = domain_ctx->power_measurement_driver_api->set_averaging_interval(
        domain_id, pai);

    if (status != FWK_SUCCESS) {
        return status;
    }

#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
    struct fwk_event outbound_event = {
        .id = mod_pcapping_notification_id_pai_changed,
        .source_id = domain_id,
    };

    status = fwk_notification_notify(
        &outbound_event, &(domain_ctx->notifications_sent_count));

#endif

    return status;
}

static int mod_pcapping_get_averaging_interval(
    fwk_id_t domain_id,
    uint32_t *pai)
{
    int status;
    struct pcapping_domain_ctx *domain_ctx;

    status = pcapping_get_ctx(domain_id, &domain_ctx);

    if (status != FWK_SUCCESS) {
        return status;
    }

    status = domain_ctx->power_measurement_driver_api->get_averaging_interval(
        domain_id, pai);

    if (status != FWK_SUCCESS) {
        return status;
    }

    return FWK_SUCCESS;
}

static int mod_pcapping_get_averaging_interval_step(
    fwk_id_t domain_id,
    uint32_t *pai_step)
{
    int status;
    struct pcapping_domain_ctx *domain_ctx;

    status = pcapping_get_ctx(domain_id, &domain_ctx);

    if (status != FWK_SUCCESS) {
        return status;
    }

    status =
        domain_ctx->power_measurement_driver_api->get_averaging_interval_step(
            domain_id, pai_step);

    if (status != FWK_SUCCESS) {
        return status;
    }

    return FWK_SUCCESS;
}

static int mod_pcapping_get_averaging_interval_range(
    fwk_id_t domain_id,
    uint32_t *min_pai,
    uint32_t *max_pai)
{
    int status;
    struct pcapping_domain_ctx *domain_ctx;

    status = pcapping_get_ctx(domain_id, &domain_ctx);

    if (status != FWK_SUCCESS) {
        return status;
    }

    status =
        domain_ctx->power_measurement_driver_api->get_averaging_interval_range(
            domain_id, min_pai, max_pai);

    if (status != FWK_SUCCESS) {
        return status;
    }
    return FWK_SUCCESS;
}

static int mod_pcapping_set_power_thresholds(
    fwk_id_t domain_id,
    uint32_t threshold_low,
    uint32_t threshold_high)
{
    int status;
    struct pcapping_domain_ctx *domain_ctx;

    status = pcapping_get_ctx(domain_id, &domain_ctx);

    if (status != FWK_SUCCESS) {
        return status;
    }

    if (threshold_low > threshold_high) {
        return FWK_E_PARAM;
    }

    domain_ctx->threshold_low = threshold_low;
    domain_ctx->threshold_high = threshold_high;

    return FWK_SUCCESS;
}

static int mod_pcapping_get_power_limit(
    fwk_id_t domain_id,
    uint32_t *power_limit)
{
    int status;
    struct pcapping_domain_ctx *domain_ctx;
    uint32_t power;
    int64_t pid_output;

    status = pcapping_get_ctx(domain_id, &domain_ctx);

    if (status != FWK_SUCCESS) {
        return status;
    }

    status = domain_ctx->power_measurement_driver_api->get_average_power(
        domain_id, &power);

    if (status != FWK_SUCCESS) {
        return status;
    }

#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
    if (power < domain_ctx->threshold_low ||
        power > domain_ctx->threshold_high) {
        struct fwk_event outbound_event = {
            .id = mod_pcapping_notification_id_meas_changed,
            .source_id = domain_id,
        };

        status = fwk_notification_notify(
            &outbound_event, &(domain_ctx->notifications_sent_count));

        if (status != FWK_SUCCESS) {
            return status;
        }
    }
#endif

    status = domain_ctx->pid_ctrl_api->update(
        domain_ctx->config->pid_controller_id, power, &pid_output);

    if (status != FWK_SUCCESS) {
        return status;
    }

    if (pid_output < 0) {
        *power_limit = 0;
    } else if (pid_output > UINT32_MAX) {
        *power_limit = UINT32_MAX;
    } else {
        *power_limit = pid_output;
    }

    return FWK_SUCCESS;
}

static struct mod_power_capping_api pcapping_api = {
    .request_cap = mod_pcapping_request_cap,
    .get_applied_cap = mod_pcapping_get_applied_cap,
    .get_average_power = mod_pcapping_get_average_power,
    .get_averaging_interval = mod_pcapping_get_averaging_interval,
    .get_averaging_interval_range = mod_pcapping_get_averaging_interval_range,
    .get_averaging_interval_step = mod_pcapping_get_averaging_interval_step,
    .set_averaging_interval = mod_pcapping_set_averaging_interval,
    .set_power_thresholds = mod_pcapping_set_power_thresholds,
};

static struct interface_power_management_api power_management_api = {
    .get_power_limit = mod_pcapping_get_power_limit,
};

static int pcapping_init(
    fwk_id_t module_id,
    unsigned int element_count,
    const void *data)
{
    pcapping_domain_ctx_table =
        fwk_mm_calloc(element_count, sizeof(struct pcapping_domain_ctx));

    pcapping_ctx.domain_count = element_count;

    return FWK_SUCCESS;
}

static int pcapping_domain_init(
    fwk_id_t element_id,
    unsigned int unused,
    const void *data)
{
    unsigned int domain_idx = fwk_id_get_element_idx(element_id);

    pcapping_domain_ctx_table[domain_idx].config =
        (struct mod_power_capping_domain_config *)data;

    return FWK_SUCCESS;
}

static int mod_pcapping_process_notification(
    const struct fwk_event *event,
    struct fwk_event *resp_event)
{
    int status;
    struct pcapping_domain_ctx *domain_ctx;

    status = pcapping_get_ctx(event->target_id, &domain_ctx);

    if (status != FWK_SUCCESS) {
        return status;
    }

    if (fwk_id_is_equal(
            event->id, domain_ctx->config->power_limit_set_notification_id)) {
        domain_ctx->applied_cap = domain_ctx->requested_cap;

        struct fwk_event outbound_event = {
            .id = mod_pcapping_notification_id_cap_change,
            .source_id = event->target_id,
        };

        return fwk_notification_notify(
            &outbound_event, &(domain_ctx->notifications_sent_count));
    }

    return FWK_E_PARAM;
}

static int mod_pcapping_bind(fwk_id_t id, unsigned int round)
{
    int status;
    struct pcapping_domain_ctx *domain_ctx;

    if ((round > 0) || fwk_id_is_type(id, FWK_ID_TYPE_MODULE)) {
        return FWK_SUCCESS;
    }

    domain_ctx = &pcapping_domain_ctx_table[fwk_id_get_element_idx(id)];

    status = fwk_module_bind(
        domain_ctx->config->power_measurement_id,
        domain_ctx->config->power_measurement_api_id,
        &domain_ctx->power_measurement_driver_api);

    if (status != FWK_SUCCESS) {
        return status;
    }

    status = fwk_module_bind(
        domain_ctx->config->pid_controller_id,
        domain_ctx->config->pid_controller_api_id,
        &domain_ctx->pid_ctrl_api);

    return status;
}

static int mod_pcapping_start(fwk_id_t id)
{
    int status;
    struct pcapping_domain_ctx *domain_ctx;

    if (!fwk_id_is_type(id, FWK_ID_TYPE_ELEMENT)) {
        return FWK_SUCCESS;
    }

    domain_ctx = &pcapping_domain_ctx_table[fwk_id_get_element_idx(id)];

    domain_ctx->requested_cap = UINT32_MAX;
    domain_ctx->applied_cap = UINT32_MAX;

    status = fwk_notification_subscribe(
        domain_ctx->config->power_limit_set_notification_id,
        domain_ctx->config->power_limit_set_notifier_id,
        id);

    return status;
}

static int mod_pcapping_process_bind_request(
    fwk_id_t source_id,
    fwk_id_t target_id,
    fwk_id_t api_id,
    const void **api)
{
    int status;
    enum mod_power_capping_api_idx api_idx;

    if (fwk_id_get_module_idx(api_id) != FWK_MODULE_IDX_POWER_CAPPING) {
        return FWK_E_PARAM;
    }

    api_idx = (enum mod_power_capping_api_idx)fwk_id_get_api_idx(api_id);

    switch (api_idx) {
    case MOD_POWER_CAPPING_API_IDX_CAP:
        *api = &pcapping_api;
        status = FWK_SUCCESS;
        break;
    case MOD_POWER_CAPPING_API_IDX_POWER_MANAGEMENT:
        *api = &power_management_api;
        status = FWK_SUCCESS;
        break;
    default:
        status = FWK_E_PARAM;
        break;
    }

    return status;
}

struct fwk_module module_power_capping = {
    .api_count = (unsigned int)MOD_POWER_CAPPING_API_IDX_COUNT,
#ifdef BUILD_HAS_NOTIFICATION
    .notification_count =
        (unsigned int)MOD_POWER_CAPPING_NOTIFICATION_IDX_COUNT,
#endif
    .type = FWK_MODULE_TYPE_HAL,
    .init = pcapping_init,
    .element_init = pcapping_domain_init,
    .bind = mod_pcapping_bind,
    .start = mod_pcapping_start,
    .process_bind_request = mod_pcapping_process_bind_request,
    .process_notification = mod_pcapping_process_notification,
};
