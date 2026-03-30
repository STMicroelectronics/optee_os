/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *      SCMI power capping and monitoring protocol completer core.
 */
#include <internal/scmi_power_capping.h>
#include <internal/scmi_power_capping_core.h>

#include <fwk_assert.h>
#include <fwk_core.h>
#include <fwk_event.h>
#include <fwk_id.h>
#include <fwk_mm.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>
#include <fwk_notification.h>

#include <stdint.h>
#ifdef BUILD_HAS_MOD_RESOURCE_PERMS
#    include <mod_resource_perms.h>
#endif

static struct {
    /* Number of power capping domains */
    unsigned int power_capping_domain_count;

    /* Table of power capping domain ctxs */
    struct mod_scmi_power_capping_domain_context
        *power_capping_domain_ctx_table;

    /* Power capping API */
    const struct mod_power_capping_api *power_capping_api;
} pcapping_core_ctx;

static const fwk_id_t pcapping_core_cap_notification = FWK_ID_NOTIFICATION_INIT(
    FWK_MODULE_IDX_POWER_CAPPING,
    MOD_POWER_CAPPING_NOTIFICATION_IDX_CAP_CHANGE);

#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
static const fwk_id_t pcapping_core_pai_notification = FWK_ID_NOTIFICATION_INIT(
    FWK_MODULE_IDX_POWER_CAPPING,
    MOD_POWER_CAPPING_NOTIFICATION_IDX_PAI_CHANGED);
static const fwk_id_t pcapping_core_power_measurements_notification =
    FWK_ID_NOTIFICATION_INIT(
        FWK_MODULE_IDX_POWER_CAPPING,
        MOD_POWER_CAPPING_NOTIFICATION_IDX_MEASUREMENTS_CHANGED);
#endif

int pcapping_core_get_domain_ctx(
    unsigned int domain_idx,
    struct mod_scmi_power_capping_domain_context **ctx)
{
    if (domain_idx >= pcapping_core_ctx.power_capping_domain_count) {
        return FWK_E_RANGE;
    }

    if (!fwk_expect(ctx != NULL)) {
        return FWK_E_PARAM;
    }

    *ctx = &(pcapping_core_ctx.power_capping_domain_ctx_table[domain_idx]);

    return FWK_SUCCESS;
}

static int pcapping_core_check_domain_configuration(
    const struct mod_scmi_power_capping_domain_config *config)
{
    if ((config->min_power_cap == (uint32_t)0) ||
        (config->max_power_cap == (uint32_t)0)) {
        return FWK_E_DATA;
    }

    if (config->min_power_cap == config->max_power_cap) {
        return FWK_SUCCESS;
    }

    if (config->power_cap_step == (uint32_t)0) {
        return FWK_E_DATA;
    }

    if ((config->max_power_cap - config->min_power_cap) %
            config->power_cap_step !=
        (uint32_t)0) {
        return FWK_E_DATA;
    }

    return FWK_SUCCESS;
}

static int pcapping_core_check_set_cap_params(
    struct mod_scmi_power_capping_domain_context *ctx,
    uint32_t cap)
{
    if (!ctx->cap_config_support) {
        return FWK_E_SUPPORT;
    }

    if (((cap < ctx->config->min_power_cap) ||
         (cap > ctx->config->max_power_cap)) &&
        (cap != SCMI_POWER_CAPPING_DISABLE_CAP_VALUE)) {
        return FWK_E_RANGE;
    }

    if ((cap - ctx->config->min_power_cap) % ctx->config->power_cap_step !=
        (uint32_t)0) {
        return FWK_E_RANGE;
    }

    return FWK_SUCCESS;
}

static bool pcapping_core_is_set_cap_busy(
    struct mod_scmi_power_capping_domain_context *ctx)
{
    if (!fwk_id_is_equal(ctx->cap_pending_service_id, FWK_ID_NONE)) {
        return true;
    }

#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
    if (ctx->config->cap_pai_change_notification_support) {
        if (!fwk_id_is_equal(ctx->cap_notification_service_id, FWK_ID_NONE)) {
            return true;
        }
    }
#endif

    return false;
}

static int pcapping_core_process_cap_fwk_notification(
    unsigned int domain_idx,
    struct mod_scmi_power_capping_domain_context *domain_ctx)
{
    struct fwk_event scmi_notification_event = {
        .target_id = FWK_ID_MODULE_INIT(FWK_MODULE_IDX_SCMI_POWER_CAPPING),
    };

    scmi_notification_event.id = FWK_ID_EVENT(
        FWK_MODULE_IDX_SCMI_POWER_CAPPING,
        SCMI_POWER_CAPPING_EVENT_IDX_PROCESS_HAL_CAP_PAI_NOTIFICATION);

    struct pcapping_core_cap_pai_event_parameters *event_params =
        (struct pcapping_core_cap_pai_event_parameters *)
            scmi_notification_event.params;

    event_params->domain_idx = domain_idx;

    event_params->service_id = domain_ctx->cap_pending_service_id;

#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
    if (domain_ctx->config->cap_pai_change_notification_support) {
        if (fwk_id_is_equal(event_params->service_id, FWK_ID_NONE)) {
            event_params->service_id = domain_ctx->cap_notification_service_id;
        }

        int status =
            pcapping_core_ctx.power_capping_api->get_averaging_interval(
                domain_ctx->config->power_capping_domain_id,
                &event_params->pai);

        if (status != FWK_SUCCESS) {
            return status;
        }

        status = pcapping_core_ctx.power_capping_api->get_applied_cap(
            domain_ctx->config->power_capping_domain_id, &event_params->cap);

        if (status != FWK_SUCCESS) {
            return status;
        }

        domain_ctx->cap_notification_service_id = FWK_ID_NONE;
    }
#endif
    domain_ctx->cap_pending_service_id = FWK_ID_NONE;

    return fwk_put_event(&scmi_notification_event);
}

#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
static int pcapping_core_process_pai_fwk_notification(
    unsigned int domain_idx,
    struct mod_scmi_power_capping_domain_context *domain_ctx)
{
    if (!domain_ctx->config->cap_pai_change_notification_support) {
        return FWK_SUCCESS;
    }

    int status;

    struct fwk_event scmi_notification_event = {
        .target_id = FWK_ID_MODULE_INIT(FWK_MODULE_IDX_SCMI_POWER_CAPPING),
    };

    scmi_notification_event.id = FWK_ID_EVENT(
        FWK_MODULE_IDX_SCMI_POWER_CAPPING,
        SCMI_POWER_CAPPING_EVENT_IDX_PROCESS_HAL_CAP_PAI_NOTIFICATION);

    struct pcapping_core_cap_pai_event_parameters *event_params =
        (struct pcapping_core_cap_pai_event_parameters *)
            scmi_notification_event.params;

    event_params->domain_idx = domain_idx;
    event_params->service_id = domain_ctx->pai_notification_service_id;

    status = pcapping_core_ctx.power_capping_api->get_applied_cap(
        domain_ctx->config->power_capping_domain_id, &event_params->cap);

    if (status != FWK_SUCCESS) {
        return status;
    }

    status = pcapping_core_ctx.power_capping_api->get_averaging_interval(
        domain_ctx->config->power_capping_domain_id, &event_params->pai);

    if (status != FWK_SUCCESS) {
        return status;
    }

    domain_ctx->pai_notification_service_id = FWK_ID_NONE;

    return fwk_put_event(&scmi_notification_event);
}

static int pcapping_core_process_power_measurements_fwk_notification(
    unsigned int domain_idx,
    struct mod_scmi_power_capping_domain_context *domain_ctx)
{
    struct fwk_event scmi_notification_event = {
        .target_id = FWK_ID_MODULE_INIT(FWK_MODULE_IDX_SCMI_POWER_CAPPING),
    };

    struct pcapping_core_pwr_meas_event_parameters *scmi_notif_event_params =
        (struct pcapping_core_pwr_meas_event_parameters *)
            scmi_notification_event.params;

    if (!domain_ctx->config->power_measurements_change_notification_support) {
        return FWK_SUCCESS;
    }

    scmi_notification_event.id = FWK_ID_EVENT(
        FWK_MODULE_IDX_SCMI_POWER_CAPPING,
        SCMI_POWER_CAPPING_EVENT_IDX_PROCESS_HAL_MEASUREMENT_NOTIF);
    scmi_notif_event_params->service_id = FWK_ID_NONE;
    scmi_notif_event_params->domain_idx = domain_idx;

    int status = pcapping_core_ctx.power_capping_api->get_average_power(
        domain_ctx->config->power_capping_domain_id,
        &(scmi_notif_event_params->power));

    if (status != FWK_SUCCESS) {
        return status;
    }

    return fwk_put_event(&scmi_notification_event);
}
#endif

int pcapping_core_get_config(
    unsigned int domain_idx,
    const struct mod_scmi_power_capping_domain_config **config)
{
    struct mod_scmi_power_capping_domain_context *ctx;

    int status = pcapping_core_get_domain_ctx(domain_idx, &ctx);

    if (status == FWK_SUCCESS) {
        *config = ctx->config;
    }

    return status;
}

int pcapping_core_get_cap_support(uint32_t domain_idx, bool *support)
{
    struct mod_scmi_power_capping_domain_context *domain_ctx;
    int status = pcapping_core_get_domain_ctx(domain_idx, &domain_ctx);

    if (status == FWK_SUCCESS) {
        *support = domain_ctx->cap_config_support;
    }

    return status;
}

int pcapping_core_bind(void)
{
    return fwk_module_bind(
        FWK_ID_MODULE(FWK_MODULE_IDX_POWER_CAPPING),
        FWK_ID_API(FWK_MODULE_IDX_POWER_CAPPING, MOD_POWER_CAPPING_API_IDX_CAP),
        &(pcapping_core_ctx.power_capping_api));
}

void pcapping_core_init(unsigned int element_count)
{
    pcapping_core_ctx.power_capping_domain_ctx_table = fwk_mm_calloc(
        element_count, sizeof(struct mod_scmi_power_capping_domain_context));
    pcapping_core_ctx.power_capping_domain_count = element_count;
}

int pcapping_core_domain_init(
    uint32_t domain_idx,
    const struct mod_scmi_power_capping_domain_config *config)
{
    struct mod_scmi_power_capping_domain_context *domain_ctx;

    int status = pcapping_core_get_domain_ctx(domain_idx, &domain_ctx);

    if (!fwk_expect(status == FWK_SUCCESS)) {
        return status;
    }

    status = pcapping_core_check_domain_configuration(config);

    if (!fwk_expect(status == FWK_SUCCESS)) {
        return status;
    }

    domain_ctx->config = config;
    domain_ctx->cap_pending_service_id = FWK_ID_NONE;
    domain_ctx->cap_notification_service_id = FWK_ID_NONE;
    domain_ctx->pai_notification_service_id = FWK_ID_NONE;

    return FWK_SUCCESS;
}

int pcapping_core_start(unsigned int domain_idx)
{
    int status;

    struct mod_scmi_power_capping_domain_context *domain_ctx;

    status = pcapping_core_get_domain_ctx(domain_idx, &domain_ctx);

    if (!fwk_expect(status == FWK_SUCCESS)) {
        return status;
    }

    domain_ctx->cap_config_support =
        (domain_ctx->config->min_power_cap !=
         domain_ctx->config->max_power_cap);

    fwk_id_t scmi_pcapping_domain_id =
        FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_SCMI_POWER_CAPPING, domain_idx);

    status = fwk_notification_subscribe(
        pcapping_core_cap_notification,
        domain_ctx->config->power_capping_domain_id,
        scmi_pcapping_domain_id);

#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
    if (!fwk_expect(status == FWK_SUCCESS)) {
        return status;
    }

    status = fwk_notification_subscribe(
        pcapping_core_pai_notification,
        FWK_ID_MODULE(FWK_MODULE_IDX_POWER_CAPPING),
        scmi_pcapping_domain_id);

    if (!fwk_expect(status == FWK_SUCCESS)) {
        return status;
    }

    status = fwk_notification_subscribe(
        pcapping_core_power_measurements_notification,
        FWK_ID_MODULE(FWK_MODULE_IDX_POWER_CAPPING),
        scmi_pcapping_domain_id);
#endif

    return status;
}

unsigned int pcapping_core_get_domain_count(void)
{
    return pcapping_core_ctx.power_capping_domain_count;
}

int pcapping_core_set_cap(
    fwk_id_t service_id,
    unsigned int domain_idx,
    bool async_flag,
    uint32_t cap)
{
    int status;
    struct mod_scmi_power_capping_domain_context *ctx;

    status = pcapping_core_get_domain_ctx(domain_idx, &ctx);

    if (status != FWK_SUCCESS) {
        return status;
    }

    status = pcapping_core_check_set_cap_params(ctx, cap);

    if (status != FWK_SUCCESS) {
        return status;
    }

    if (pcapping_core_is_set_cap_busy(ctx)) {
        return FWK_E_BUSY;
    }

    fwk_id_t domain_id = ctx->config->power_capping_domain_id;

    status = pcapping_core_ctx.power_capping_api->request_cap(domain_id, cap);

    if (status == FWK_PENDING) {
        ctx->cap_pending_service_id = service_id;
        ctx->is_cap_request_async = async_flag;
    }

#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
    if (ctx->config->cap_pai_change_notification_support) {
        ctx->cap_notification_service_id = service_id;
    }
#endif

    return status;
}

int pcapping_core_get_cap(unsigned int domain_idx, uint32_t *cap)
{
    int status;
    struct mod_scmi_power_capping_domain_context *ctx;

    status = pcapping_core_get_domain_ctx(domain_idx, &ctx);

    if (status != FWK_SUCCESS) {
        return status;
    }

    return pcapping_core_ctx.power_capping_api->get_applied_cap(
        ctx->config->power_capping_domain_id, cap);
}

int pcapping_core_get_pai_info(
    unsigned int domain_idx,
    uint32_t *min_pai,
    uint32_t *max_pai,
    uint32_t *pai_step)
{
    int status;
    struct mod_scmi_power_capping_domain_context *ctx;

    status = pcapping_core_get_domain_ctx(domain_idx, &ctx);

    if (status != FWK_SUCCESS) {
        return status;
    }

    status = pcapping_core_ctx.power_capping_api->get_averaging_interval_step(
        ctx->config->power_capping_domain_id, pai_step);

    if (status != FWK_SUCCESS) {
        return status;
    }

    status = pcapping_core_ctx.power_capping_api->get_averaging_interval_range(
        ctx->config->power_capping_domain_id, min_pai, max_pai);

    return status;
}

int pcapping_core_set_pai(
    fwk_id_t service_id,
    unsigned int domain_idx,
    uint32_t pai)
{
    int status;
    struct mod_scmi_power_capping_domain_context *ctx;

    status = pcapping_core_get_domain_ctx(domain_idx, &ctx);

    if (status != FWK_SUCCESS) {
        return status;
    }

    if (!ctx->config->pai_config_support) {
        return FWK_E_SUPPORT;
    }

#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
    if (ctx->config->cap_pai_change_notification_support) {
        if (!fwk_id_is_equal(ctx->pai_notification_service_id, FWK_ID_NONE)) {
            return FWK_E_BUSY;
        }
    }
#endif

    status = pcapping_core_ctx.power_capping_api->set_averaging_interval(
        ctx->config->power_capping_domain_id, pai);

    if (status != FWK_SUCCESS) {
        return status;
    }

#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
    if (ctx->config->cap_pai_change_notification_support) {
        ctx->pai_notification_service_id = service_id;
    }
#endif

    return FWK_SUCCESS;
}

int pcapping_core_get_pai(unsigned int domain_idx, uint32_t *pai)
{
    int status;
    struct mod_scmi_power_capping_domain_context *ctx;

    status = pcapping_core_get_domain_ctx(domain_idx, &ctx);

    if (status != FWK_SUCCESS) {
        return status;
    }

    status = pcapping_core_ctx.power_capping_api->get_averaging_interval(
        ctx->config->power_capping_domain_id, pai);

    return status;
}

int pcapping_core_get_power(unsigned int domain_idx, uint32_t *power)
{
    int status;

    struct mod_scmi_power_capping_domain_context *ctx;
    status = pcapping_core_get_domain_ctx(domain_idx, &ctx);

    if (status != FWK_SUCCESS) {
        return status;
    }

    status = pcapping_core_ctx.power_capping_api->get_average_power(
        ctx->config->power_capping_domain_id, power);

    return status;
}

int pcapping_core_set_power_thresholds(
    unsigned int domain_idx,
    uint32_t threshold_low,
    uint32_t threshold_high)
{
    int status;
    struct mod_scmi_power_capping_domain_context *ctx;

    status = pcapping_core_get_domain_ctx(domain_idx, &ctx);

    if (status != FWK_SUCCESS) {
        return status;
    }

    return pcapping_core_ctx.power_capping_api->set_power_thresholds(
        ctx->config->power_capping_domain_id, threshold_low, threshold_high);
}

bool pcapping_core_is_cap_request_async(uint32_t domain_idx)
{
    if (domain_idx < pcapping_core_ctx.power_capping_domain_count) {
        return pcapping_core_ctx.power_capping_domain_ctx_table[domain_idx]
            .is_cap_request_async;
    }

    return true;
}

int pcapping_core_process_fwk_notification(
    const struct fwk_event *fwk_notification_event)
{
    int status;
    struct mod_scmi_power_capping_domain_context *domain_ctx;

    unsigned int domain_idx =
        fwk_id_get_element_idx(fwk_notification_event->target_id);

    status = pcapping_core_get_domain_ctx(domain_idx, &domain_ctx);

    if (status != FWK_SUCCESS) {
        return status;
    }

    if (fwk_id_is_equal(
            fwk_notification_event->id, pcapping_core_cap_notification)) {
        return pcapping_core_process_cap_fwk_notification(
            domain_idx, domain_ctx);
    }
#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
    if (fwk_id_is_equal(
            fwk_notification_event->id, pcapping_core_pai_notification)) {
        return pcapping_core_process_pai_fwk_notification(
            domain_idx, domain_ctx);
    }
    if (fwk_id_is_equal(
            fwk_notification_event->id,
            pcapping_core_power_measurements_notification)) {
        return pcapping_core_process_power_measurements_fwk_notification(
            domain_idx, domain_ctx);
    }
#endif

    return FWK_E_PARAM;
}
