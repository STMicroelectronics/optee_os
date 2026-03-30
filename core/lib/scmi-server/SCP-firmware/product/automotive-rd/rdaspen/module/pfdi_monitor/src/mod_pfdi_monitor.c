/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <mod_pfdi_monitor.h>
#include <mod_power_domain.h>
#include <mod_timer.h>

#include <fwk_assert.h>
#include <fwk_core.h>
#include <fwk_id.h>
#include <fwk_log.h>
#include <fwk_mm.h>
#include <fwk_module.h>
#include <fwk_notification.h>

#define MOD_NAME "[PFDI_MONITOR] "

enum pfdi_monitor_event_idx {
    PFDI_MONITOR_EVENT_IDX_OOR_STATUS,
    PFDI_MONITOR_EVENT_IDX_ONL_STATUS,
    PFDI_MONITOR_EVENT_IDX_TIMEOUT,
    PFDI_MONITOR_EVENT_IDX_COUNT
};

enum pfdi_monitor_core_state {
    PFDI_MONITOR_STATE_WAIT_FOR_OOR,
    PFDI_MONITOR_STATE_WAIT_FOR_BOOT,
    PFDI_MONITOR_STATE_WAIT_FOR_ONL,
};

/*!
 * \brief Core context.
 */
struct pfdi_monitor_core_context {
    /*! Timer alarm API */
    const struct mod_timer_alarm_api *alarm_api;
    /*! Wait for boot flag */
    enum pfdi_monitor_core_state core_state;
    /*! PFDI core configuration data */
    const struct mod_pfdi_monitor_core_config *core_cfg;
};

/*!
 * \brief Module context.
 */
struct pfdi_monitor_ctx {
    /*! PFDI core context data */
    struct pfdi_monitor_core_context *core_ctx_table;
    /*! Number of cores running the PFDI */
    uint32_t core_count;
};

static struct pfdi_monitor_ctx ctx;

static fwk_id_t pd_transition_notification_id = FWK_ID_NOTIFICATION_INIT(
    FWK_MODULE_IDX_POWER_DOMAIN,
    MOD_PD_NOTIFICATION_IDX_POWER_STATE_TRANSITION);

static int pfdi_monitor_oor_status(fwk_id_t id, uint32_t status)
{
    int ret;
    unsigned int element_idx = fwk_id_get_element_idx(id);
    struct fwk_event event = {
        .source_id = FWK_ID_MODULE(FWK_MODULE_IDX_PFDI_MONITOR),
        .target_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_PFDI_MONITOR, element_idx),
        .id = FWK_ID_EVENT_INIT(
            FWK_MODULE_IDX_PFDI_MONITOR, PFDI_MONITOR_EVENT_IDX_OOR_STATUS),
        .params = { status },
    };

    if (element_idx >= ctx.core_count) {
        return FWK_E_PARAM;
    }

    ret = fwk_put_event(&event);
    if (ret != FWK_SUCCESS) {
        /**
         * @brief Discards invalid PFDI events instead of returning an error.
         *
         * This prevents a malicious PFDI caller from overwhelming the
         * PFDI monitor and causing a Denial of Service (DoS) attack.
         * The event is discarded with an error message rather than
         * returning an error code.
         */
        FWK_LOG_ERR(
            MOD_NAME "Error! Failed to add OoR event for %s",
            fwk_module_get_element_name(id));
    }

    return FWK_SUCCESS;
}

static int pfdi_monitor_onl_status(fwk_id_t id, uint32_t status)
{
    int ret;
    unsigned int element_idx = fwk_id_get_element_idx(id);
    struct fwk_event event = {
        .source_id = FWK_ID_MODULE(FWK_MODULE_IDX_PFDI_MONITOR),
        .target_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_PFDI_MONITOR, element_idx),
        .id = FWK_ID_EVENT_INIT(
            FWK_MODULE_IDX_PFDI_MONITOR, PFDI_MONITOR_EVENT_IDX_ONL_STATUS),
        .params = { status },
    };

    if (element_idx >= ctx.core_count) {
        return FWK_E_PARAM;
    }

    ret = fwk_put_event(&event);
    if (ret != FWK_SUCCESS) {
        /**
         * @note Do not return an error.
         *
         * Simply discard the event with an error message to prevent a
         * malicious PFDI caller from overwhelming the PFDI monitor and
         * causing a Denial of Service (DoS) attack.
         */
        FWK_LOG_ERR(
            MOD_NAME "Error! Failed to add Onl event for %s",
            fwk_module_get_element_name(id));
    }

    return FWK_SUCCESS;
}

static struct mod_pfdi_monitor_api pfdi_monitor_api = {
    .oor_status = pfdi_monitor_oor_status,
    .onl_status = pfdi_monitor_onl_status,
};

static void pfdi_monitor_timeout(uintptr_t id)
{
    int status;
    uint32_t element_idx = (uint32_t)id;
    struct fwk_event event = {
        .source_id = FWK_ID_MODULE(FWK_MODULE_IDX_PFDI_MONITOR),
        .target_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_PFDI_MONITOR, element_idx),
        .id = FWK_ID_EVENT_INIT(
            FWK_MODULE_IDX_PFDI_MONITOR, PFDI_MONITOR_EVENT_IDX_TIMEOUT),
    };

    if (element_idx >= ctx.core_count) {
        fwk_trap();
    }

    status = fwk_put_event(&event);

    if (status != FWK_SUCCESS) {
        /**
         * @note Do not return an error.
         *
         * Simply discard the event with an error message to prevent a
         * malicious PFDI caller from overwhelming the PFDI monitor and
         * causing a Denial of Service (DoS) attack.
         */
        FWK_LOG_ERR(
            MOD_NAME "Error! Failed to add timeout event for %s",
            fwk_module_get_element_name(fwk_id_build_element_id(
                fwk_module_id_pfdi_monitor, element_idx)));
    }
}

static int pfdi_monitor_init(
    fwk_id_t module_id,
    unsigned int element_count,
    const void *data)
{
    if (element_count == 0) {
        return FWK_E_PARAM;
    }

    ctx.core_ctx_table =
        fwk_mm_calloc(element_count, sizeof(struct pfdi_monitor_core_context));

    ctx.core_count = element_count;

    return FWK_SUCCESS;
}

static int pfdi_monitor_element_init(
    fwk_id_t id,
    unsigned int sub_element_count,
    const void *data)
{
    struct pfdi_monitor_core_context *core_ctx;
    const struct mod_pfdi_monitor_core_config *core_cfg;
    unsigned int element_idx = fwk_id_get_element_idx(id);

    if (element_idx >= ctx.core_count) {
        return FWK_E_PARAM;
    }

    core_cfg = (struct mod_pfdi_monitor_core_config *)data;

    if (core_cfg == NULL) {
        return FWK_E_DATA;
    }

    core_ctx = &ctx.core_ctx_table[element_idx];
    core_ctx->core_cfg = core_cfg;
    core_ctx->core_state = PFDI_MONITOR_STATE_WAIT_FOR_OOR;

    return FWK_SUCCESS;
}

static int pfdi_monitor_start(fwk_id_t id)
{
    int status;
    struct pfdi_monitor_core_context *core_ctx;
    const struct mod_pfdi_monitor_core_config *core_cfg;
    unsigned int element_idx;
    fwk_id_t pd_transition_source_id;

    if (fwk_id_is_type(id, FWK_ID_TYPE_MODULE)) {
        return FWK_SUCCESS;
    }

    element_idx = fwk_id_get_element_idx(id);

    pd_transition_source_id =
        fwk_id_build_element_id(fwk_module_id_power_domain, element_idx);

    status = fwk_notification_subscribe(
        pd_transition_notification_id, pd_transition_source_id, id);
    if (status != FWK_SUCCESS) {
        FWK_LOG_ERR(
            MOD_NAME "Failed to subscribe to Power Domain notification for %s",
            fwk_module_get_element_name(id));
    } else {
        FWK_LOG_DEBUG(
            MOD_NAME "Subscribed to Power Domain notifications for %s",
            fwk_module_get_element_name(id));
    }

    if (element_idx >= ctx.core_count) {
        return FWK_E_PARAM;
    }

    core_ctx = &ctx.core_ctx_table[element_idx];
    core_cfg = core_ctx->core_cfg;
    status = core_ctx->alarm_api->start(
        core_cfg->alarm_id,
        core_cfg->oor_pfdi_period_us,
        MOD_TIMER_ALARM_TYPE_ONCE,
        pfdi_monitor_timeout,
        (uintptr_t)fwk_id_get_element_idx(id));

    if (status != FWK_SUCCESS) {
        FWK_LOG_ERR(
            MOD_NAME "Error! Failed to start PFDI monitoring for %s",
            fwk_module_get_element_name(id));
    } else {
        FWK_LOG_INFO(
            MOD_NAME "Started PFDI monitoring for %s",
            fwk_module_get_element_name(id));
        FWK_LOG_DEBUG(
            MOD_NAME "%s waiting for OoR PFDI status ...",
            fwk_module_get_element_name(id));
    }

    return status;
}

static int pfdi_monitor_bind(fwk_id_t id, unsigned int round)
{
    unsigned int element_idx;
    struct pfdi_monitor_core_context *core_ctx;
    const struct mod_pfdi_monitor_core_config *core_cfg;

    if (round != 0) {
        return FWK_SUCCESS;
    }

    if (fwk_id_is_type(id, FWK_ID_TYPE_MODULE)) {
        return FWK_SUCCESS;
    }

    element_idx = fwk_id_get_element_idx(id);

    if (element_idx >= ctx.core_count) {
        return FWK_E_PARAM;
    }

    core_ctx = &ctx.core_ctx_table[element_idx];
    core_cfg = core_ctx->core_cfg;

    return fwk_module_bind(
        core_cfg->alarm_id, MOD_TIMER_API_ID_ALARM, &core_ctx->alarm_api);
}

static int pfdi_monitor_process_bind_request(
    fwk_id_t source_id,
    fwk_id_t target_id,
    fwk_id_t api_id,
    const void **api)
{
    int status;
    enum mod_pfdi_monitor_api_idx api_idx;

    if (!api) {
        return FWK_E_PARAM;
    }

    api_idx = (enum mod_pfdi_monitor_api_idx)fwk_id_get_api_idx(api_id);

    switch (api_idx) {
    case MOD_PFDI_MONITOR_API_IDX_PFDI_MONITOR:
        *api = &pfdi_monitor_api;
        status = FWK_SUCCESS;
        break;

    default:
        status = FWK_E_PARAM;
        break;
    }

    return status;
}

static int pfdi_monitor_process_event(
    const struct fwk_event *event,
    struct fwk_event *resp_event)
{
    struct pfdi_monitor_core_context *core_ctx;
    const struct mod_pfdi_monitor_core_config *core_cfg;
    unsigned int element_idx;
    int status;

    if (!event) {
        return FWK_E_PARAM;
    }

    element_idx = fwk_id_get_element_idx(event->target_id);
    if (element_idx >= ctx.core_count) {
        return FWK_E_PARAM;
    }

    core_ctx = &ctx.core_ctx_table[element_idx];
    core_cfg = core_ctx->core_cfg;

    switch (fwk_id_get_event_idx(event->id)) {
    case (unsigned int)PFDI_MONITOR_EVENT_IDX_OOR_STATUS:

        /*! Check if the core is in a wrong state */
        if (core_ctx->core_state != PFDI_MONITOR_STATE_WAIT_FOR_OOR) {
            FWK_LOG_ERR(
                MOD_NAME
                "Received OoR PFDI status for %s after Onl PFDI status",
                fwk_module_get_element_name(event->target_id));

            return FWK_E_ACCESS;
        }

        /*! Stop the alarm */
        status = core_ctx->alarm_api->stop(core_cfg->alarm_id);
        if ((status != FWK_SUCCESS) && (status != FWK_E_STATE)) {
            FWK_LOG_ERR(
                MOD_NAME "Error! Failed to stop PFDI monitoring alarm for %s",
                fwk_module_get_element_name(event->target_id));

            return status;
        }

        /*! Check if the OoR PFDI succeeded */
        if (event->params[0] != 0) {
            FWK_LOG_ERR(
                MOD_NAME "OoR PFDI for %s failed, stopping PFDI monitoring",
                fwk_module_get_element_name(event->target_id));

            return FWK_SUCCESS;
        }

        /*! Restart the alarm to wait for boot */
        status = core_ctx->alarm_api->start(
            core_cfg->alarm_id,
            core_cfg->boot_timeout_us,
            MOD_TIMER_ALARM_TYPE_ONCE,
            pfdi_monitor_timeout,
            (uintptr_t)element_idx);

        if (status != FWK_SUCCESS) {
            FWK_LOG_ERR(
                MOD_NAME "Error! Failed to start PFDI monitoring alarm for %s",
                fwk_module_get_element_name(event->target_id));

            return status;
        }

        /*! Change the state to wait for boot */
        core_ctx->core_state = PFDI_MONITOR_STATE_WAIT_FOR_BOOT;

        FWK_LOG_DEBUG(
            MOD_NAME "OoR PFDI for %s succeeded, waiting for boot ...",
            fwk_module_get_element_name(event->target_id));

        return FWK_SUCCESS;

    case (unsigned int)PFDI_MONITOR_EVENT_IDX_ONL_STATUS:

        /*! Check if the core is in a wrong state */
        if (core_ctx->core_state == PFDI_MONITOR_STATE_WAIT_FOR_OOR) {
            FWK_LOG_ERR(
                MOD_NAME
                "Received Onl PFDI status for %s before OoR PFDI status",
                fwk_module_get_element_name(event->target_id));

            return FWK_E_ACCESS;
        }

        /*! Stop the alarm */
        status = core_ctx->alarm_api->stop(core_cfg->alarm_id);
        if ((status != FWK_SUCCESS) && (status != FWK_E_STATE)) {
            FWK_LOG_ERR(
                MOD_NAME "Error! Failed to stop PFDI monitoring alarm for %s",
                fwk_module_get_element_name(event->target_id));

            return status;
        }

        /*! Check if the Onl PFDI succeeded */
        if (event->params[0] != 0) {
            FWK_LOG_ERR(
                MOD_NAME "Onl PFDI for %s failed, stopping PFDI monitoring",
                fwk_module_get_element_name(event->target_id));

            return FWK_SUCCESS;
        }

        /*! Restart the alarm to wait for Onl PFDI again */
        status = core_ctx->alarm_api->start(
            core_cfg->alarm_id,
            core_cfg->onl_pfdi_period_us,
            MOD_TIMER_ALARM_TYPE_ONCE,
            pfdi_monitor_timeout,
            (uintptr_t)element_idx);
        if (status != FWK_SUCCESS) {
            FWK_LOG_ERR(
                MOD_NAME "Error! Failed to start PFDI monitoring alarm for %s",
                fwk_module_get_element_name(event->target_id));

            return status;
        }

        /*! Change the state to wait for Onl PFDI */
        core_ctx->core_state = PFDI_MONITOR_STATE_WAIT_FOR_ONL;

        FWK_LOG_DEBUG(
            MOD_NAME "Onl PFDI for %s succeeded",
            fwk_module_get_element_name(event->target_id));

        return FWK_SUCCESS;

    case (unsigned int)PFDI_MONITOR_EVENT_IDX_TIMEOUT:
        FWK_LOG_ERR(
            MOD_NAME "Error! PFDI monitor timeout for %s",
            fwk_module_get_element_name(event->target_id));

        return FWK_SUCCESS;

    default:
        FWK_LOG_ERR(
            MOD_NAME "Invalid PFDI monitor event: %s", FWK_ID_STR(event->id));

        return FWK_E_PARAM;
    }
}

static int pfdi_monitor_process_notificiation(
    const struct fwk_event *event,
    struct fwk_event *resp_event)
{
    struct mod_pd_power_state_transition_notification_params *params;
    struct pfdi_monitor_core_context *core_ctx;
    const struct mod_pfdi_monitor_core_config *core_cfg;
    unsigned int element_idx;
    int status;

    if (fwk_id_is_type(event->target_id, FWK_ID_TYPE_MODULE)) {
        return FWK_E_PARAM;
    }

    if (fwk_id_is_equal(event->id, pd_transition_notification_id)) {
        element_idx = fwk_id_get_element_idx(event->target_id);

        if (element_idx >= ctx.core_count) {
            return FWK_E_PARAM;
        }

        core_ctx = &ctx.core_ctx_table[element_idx];
        core_cfg = core_ctx->core_cfg;

        params = (struct mod_pd_power_state_transition_notification_params *)
                     event->params;

        switch (params->state) {
        case (unsigned int)MOD_PD_STATE_OFF:
        case (unsigned int)MOD_PD_STATE_OFF_0:
        case (unsigned int)MOD_PD_STATE_OFF_1:
        case (unsigned int)MOD_PD_STATE_OFF_2:
        case (unsigned int)MOD_PD_STATE_SLEEP:
            FWK_LOG_INFO(
                MOD_NAME
                "%s has been turned off, switching off PFDI monitoring",
                fwk_module_get_element_name(event->target_id));
            /* Stop the alarm */
            status = core_ctx->alarm_api->stop(core_cfg->alarm_id);
            if ((status != FWK_SUCCESS) && (status != FWK_E_STATE)) {
                FWK_LOG_ERR(
                    MOD_NAME
                    "Error! Failed to stop PFDI monitoring alarm for %s",
                    fwk_module_get_element_name(event->target_id));

                return status;
            }
            break;
        case (unsigned int)MOD_PD_STATE_ON:
            FWK_LOG_INFO(
                MOD_NAME "%s has been turned on, switching on PFDI monitoring",
                fwk_module_get_element_name(event->target_id));
            status = core_ctx->alarm_api->start(
                core_cfg->alarm_id,
                core_cfg->boot_timeout_us,
                MOD_TIMER_ALARM_TYPE_ONCE,
                pfdi_monitor_timeout,
                (uintptr_t)element_idx);
            if (status != FWK_SUCCESS) {
                FWK_LOG_ERR(
                    MOD_NAME
                    "Error! Failed to start PFDI monitoring alarm for %s",
                    fwk_module_get_element_name(event->target_id));

                return status;
            }
            break;
        default:
            /*! Unsupported Power Domain notification, do nothing */
            break;
        }
    } else {
        return FWK_E_PARAM;
    }

    return FWK_SUCCESS;
}

const struct fwk_module module_pfdi_monitor = {
    .type = FWK_MODULE_TYPE_SERVICE,
    .api_count = MOD_PFDI_MONITOR_API_IDX_COUNT,
    .init = pfdi_monitor_init,
    .element_init = pfdi_monitor_element_init,
    .start = pfdi_monitor_start,
    .bind = pfdi_monitor_bind,
    .process_bind_request = pfdi_monitor_process_bind_request,
    .process_event = pfdi_monitor_process_event,
    .process_notification = pfdi_monitor_process_notificiation,
    .event_count = PFDI_MONITOR_EVENT_IDX_COUNT,
};
