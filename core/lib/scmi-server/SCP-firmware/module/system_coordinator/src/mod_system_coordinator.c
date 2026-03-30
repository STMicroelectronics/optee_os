/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "mod_system_coordinator.h"

#include <mod_timer.h>

#include <fwk_core.h>
#include <fwk_id.h>
#include <fwk_log.h>
#include <fwk_mm.h>
#include <fwk_module.h>
#include <fwk_status.h>

#include <inttypes.h>

#define MOD_NAME          "[SYS_COORDINATOR] "
#define FIRST_PHASE_INDEX 0

enum mod_system_coordinator_event_idx {
    MOD_SYSTEM_COORDINATOR_EVENT_IDX_PHASE,
    MOD_SYSTEM_COORDINATOR_EVENT_IDX_COUNT,
};

/* Event parameter for EVENT_IDX_PHASE */
struct phase_event_params {
    /* The phase index of the API to be call in process_event() */
    unsigned int phase_idx;

    /* The cycle count which the phase originate */
    uint32_t cycle_count;
};

/* Phase context */
struct mod_system_coordinator_phase_ctx {
    /* Phase configuration */
    const struct mod_system_coordinator_phase_config *phase_config;

    /* Phase API */
    struct mod_system_coordinator_phase_api *phase_api;
};

/* Module context */
struct mod_system_coordinator_ctx {
    /* System Coordinator configuration */
    const struct mod_system_coordinator_config *config;

    /* Phase context */
    struct mod_system_coordinator_phase_ctx *phase_ctx;

    /* Cycle alarm API */
    const struct mod_timer_alarm_api *cycle_alarm_api;

    /* Phase alarm API */
    const struct mod_timer_alarm_api *phase_alarm_api;

    /* Total phase count */
    uint32_t phase_count;

    /* Cycle count */
    uint32_t cycle_count;
};

static const fwk_id_t mod_system_coordinator_event_phase = FWK_ID_EVENT_INIT(
    FWK_MODULE_IDX_SYSTEM_COORDINATOR,
    MOD_SYSTEM_COORDINATOR_EVENT_IDX_PHASE);

static struct mod_system_coordinator_ctx system_coordinator_ctx;

static int send_phase_event(struct phase_event_params *params)
{
    int status;
    struct fwk_event phase_event = {
        .source_id = FWK_ID_MODULE(FWK_MODULE_IDX_SYSTEM_COORDINATOR),
        .target_id = FWK_ID_MODULE(FWK_MODULE_IDX_SYSTEM_COORDINATOR),
        .id = mod_system_coordinator_event_phase,
    };
    struct phase_event_params *evt_params =
        (struct phase_event_params *)&phase_event.params;

    *evt_params = *params;
    status = fwk_put_event(&phase_event);
    if (status != FWK_SUCCESS) {
        FWK_LOG_ERR(
            MOD_NAME "%s@%u status=%u, phase_idx=%u",
            __func__,
            __LINE__,
            status,
            params->phase_idx);
    }

    return status;
}

/*
 * Alarm callback
 */

static void system_coordinator_cycle_alarm_callback(uintptr_t param)
{
    uint32_t *cycle_count = &system_coordinator_ctx.cycle_count;
    struct phase_event_params first_phase_params = { 0 };

    (*cycle_count)++;

    first_phase_params.phase_idx = FIRST_PHASE_INDEX;
    first_phase_params.cycle_count = *cycle_count;

    send_phase_event(&first_phase_params);
}

static void system_coordinator_phase_alarm_callback(uintptr_t param)
{
    send_phase_event((struct phase_event_params *)param);
}

static int start_timer_for_next_phase(
    const struct mod_system_coordinator_phase_ctx *phase_ctx,
    const struct phase_event_params *evt_params)
{
    return system_coordinator_ctx.phase_alarm_api->start(
        system_coordinator_ctx.config->phase_alarm_id,
        phase_ctx->phase_config->phase_us,
        MOD_TIMER_ALARM_TYPE_ONCE,
        system_coordinator_phase_alarm_callback,
        (uintptr_t)evt_params);
}

static int process_current_phase(const struct phase_event_params *params)
{
    int status = FWK_SUCCESS;
    struct phase_event_params next_phase_params = { 0 };
    struct mod_system_coordinator_phase_ctx *phase_ctx;

    if (params->phase_idx >= system_coordinator_ctx.phase_count) {
        return FWK_E_RANGE;
    }

    if (params->cycle_count != system_coordinator_ctx.cycle_count) {
        FWK_LOG_ERR(MOD_NAME "Cycle count mismatch");
        return FWK_E_STATE;
    }

    phase_ctx = &system_coordinator_ctx.phase_ctx[params->phase_idx];
    next_phase_params.phase_idx = params->phase_idx + 1;
    next_phase_params.cycle_count = params->cycle_count;
    if (next_phase_params.phase_idx < system_coordinator_ctx.phase_count) {
        if (phase_ctx->phase_config->phase_us == 0) {
            /* Send event to process next phase if current phase timer is 0 */
            status = send_phase_event(&next_phase_params);
        } else if (
            params->phase_idx <= (system_coordinator_ctx.phase_count - 1)) {
            /*
             * Start timer for next phase. Timer will be skip if the phase is
             * the last phase or the phase time value is 0.
             */
            status = start_timer_for_next_phase(phase_ctx, &next_phase_params);
        }

        if (status != FWK_SUCCESS) {
            return status;
        }
    }

    /* Call phase API */
    phase_ctx->phase_api->phase_handler();

    return FWK_SUCCESS;
}

/*
 * Framework handlers
 */

static int system_coordinator_init(
    fwk_id_t module_id,
    unsigned int element_count,
    const void *data)
{
    if ((data == NULL) || (element_count == 0U)) {
        return FWK_E_PARAM;
    }

    system_coordinator_ctx.phase_count = element_count;
    system_coordinator_ctx.phase_ctx = fwk_mm_calloc(
        element_count, sizeof(struct mod_system_coordinator_phase_ctx));
    system_coordinator_ctx.config =
        (const struct mod_system_coordinator_config *)data;

    return FWK_SUCCESS;
}

static int system_coordinator_element_init(
    fwk_id_t element_id,
    unsigned int sub_element_count,
    const void *data)
{
    if (data == NULL) {
        return FWK_E_PARAM;
    }

    uint32_t idx;

    idx = fwk_id_get_element_idx(element_id);
    if (idx >= system_coordinator_ctx.phase_count) {
        return FWK_E_PARAM;
    }

    system_coordinator_ctx.phase_ctx[idx].phase_config =
        (const struct mod_system_coordinator_phase_config *)data;

    return FWK_SUCCESS;
}

static int system_coordinator_post_init(fwk_id_t module_id)
{
    uint32_t total_phase_time = 0;

    /* Sum of all phase time is not allowed to be more than cycle time */
    for (uint32_t i = 0; i < system_coordinator_ctx.phase_count; i++) {
        total_phase_time +=
            system_coordinator_ctx.phase_ctx[i].phase_config->phase_us;
    }

    if (total_phase_time > system_coordinator_ctx.config->cycle_us) {
        return FWK_E_SUPPORT;
    }

    return FWK_SUCCESS;
}

static int system_coordinator_bind(fwk_id_t id, unsigned int round)
{
    int status = FWK_E_INIT;
    uint32_t phase_idx;
    struct mod_system_coordinator_phase_ctx *phase_ctx;

    /* Only bind in first round of calls. */
    if (round > 0) {
        return FWK_SUCCESS;
    }

    if (fwk_module_is_valid_module_id(id)) {
        /* Cycle alarm */
        status = fwk_module_bind(
            system_coordinator_ctx.config->cycle_alarm_id,
            MOD_TIMER_API_ID_ALARM,
            &system_coordinator_ctx.cycle_alarm_api);
        if (status != FWK_SUCCESS) {
            FWK_LOG_ERR(MOD_NAME "Error binding to cycle alarm API");
            return status;
        }

        /* Phase alarm */
        status = fwk_module_bind(
            system_coordinator_ctx.config->phase_alarm_id,
            MOD_TIMER_API_ID_ALARM,
            &system_coordinator_ctx.phase_alarm_api);
        if (status != FWK_SUCCESS) {
            FWK_LOG_ERR(MOD_NAME "Error binding to phase alarm API");
        }
    } else if (fwk_module_is_valid_element_id(id)) {
        phase_idx = fwk_id_get_element_idx(id);
        phase_ctx = &system_coordinator_ctx.phase_ctx[phase_idx];
        status = fwk_module_bind(
            phase_ctx->phase_config->module_id,
            phase_ctx->phase_config->api_id,
            &phase_ctx->phase_api);
        if (status != FWK_SUCCESS) {
            FWK_LOG_ERR(
                MOD_NAME "Error binding to phase[%" PRIu32 "] API", phase_idx);
        }
    }

    return status;
}

static int system_coordinator_start(fwk_id_t id)
{
    int status;
    struct phase_event_params params;

    if (!fwk_id_is_type(id, FWK_ID_TYPE_MODULE)) {
        return FWK_SUCCESS;
    }

    system_coordinator_ctx.cycle_count = 0;

    /* Start cycle timer */
    status = system_coordinator_ctx.cycle_alarm_api->start(
        system_coordinator_ctx.config->cycle_alarm_id,
        system_coordinator_ctx.config->cycle_us,
        MOD_TIMER_ALARM_TYPE_PERIODIC,
        system_coordinator_cycle_alarm_callback,
        (uintptr_t)NULL);
    if (status != FWK_SUCCESS) {
        return status;
    }

    params.cycle_count = system_coordinator_ctx.cycle_count;
    params.phase_idx = FIRST_PHASE_INDEX;

    /* Start first phase */
    status = process_current_phase(&params);

    return status;
}

static int system_coordinator_process_event(
    const struct fwk_event *event,
    struct fwk_event *resp_event)
{
    struct phase_event_params *params;

    /*
     * Event from cycle and phase timer callback. Both timer callback send the
     * same event. Cycle callback will process the first phase while phase
     * callback process the current phase.
     */
    if (fwk_id_is_equal(event->id, mod_system_coordinator_event_phase)) {
        params = (struct phase_event_params *)event->params;
        return process_current_phase(params);
    }

    return FWK_E_PARAM;
}

/* Module definition */
const struct fwk_module module_system_coordinator = {
    .type = FWK_MODULE_TYPE_SERVICE,
    .event_count = (unsigned int)MOD_SYSTEM_COORDINATOR_EVENT_IDX_COUNT,
    .init = system_coordinator_init,
    .element_init = system_coordinator_element_init,
    .post_init = system_coordinator_post_init,
    .bind = system_coordinator_bind,
    .start = system_coordinator_start,
    .process_event = system_coordinator_process_event,
};
