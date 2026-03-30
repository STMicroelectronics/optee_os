/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <internal/mpmm_v2.h>

#include <mod_metrics_analyzer.h>
#include <mod_mpmm_v2.h>
#include <mod_power_domain.h>
#ifdef BUILD_HAS_MOD_PERF_CONTROLLER
#    include <mod_perf_controller.h>
#endif

#include <interface_amu.h>
#include <interface_power_management.h>

#include <fwk_assert.h>
#include <fwk_core.h>
#include <fwk_id.h>
#include <fwk_log.h>
#include <fwk_mm.h>
#include <fwk_module.h>
#include <fwk_notification.h>
#include <fwk_status.h>
#include <fwk_string.h>

#include <inttypes.h>

static struct mod_mpmm_v2_ctx {
    /* Number of MPMM_V2 domains */
    uint32_t mpmm_v2_domain_count;

    /* Domain context table */
    struct mod_mpmm_v2_domain_ctx *domain_ctx;

    /* AMU driver API */
    struct amu_api *amu_driver_api;

    /* AMU driver API for access AMU Auxiliry registers */
    fwk_id_t amu_driver_api_id;
} mpmm_v2_ctx;

/*
 * MPMM and AMU Registers access functions
 */

/* Read the number of MPMM gears. */
static inline uint32_t mpmm_v2_core_get_number_of_gears(
    const struct mod_mpmm_v2_core_ctx *core_ctx)
{
    return (
        (core_ctx->mpmm_v2->PPMCR >> MPMM_PPMCR_NUM_GEARS_POS) &
        MPMM_PPMCR_NUM_GEARS_MASK);
}

/* Check if the MPMM throttling and MPMM counters are enabled for a core.
 */
static inline bool mpmm_v2_core_check_enabled(
    const struct mod_mpmm_v2_core_ctx *core_ctx)
{
    return (
        bool)((core_ctx->mpmm_v2->MPMMCR >> MPMM_MPMMCR_EN_POS) & MPMM_MPMMCR_EN_MASK);
}

/* Set the MPMM gear for a specific core. */
static void mpmm_v2_core_set_gear(struct mod_mpmm_v2_core_ctx *core_ctx)
{
    core_ctx->mpmm_v2->MPMMCR |=
        ((core_ctx->selected_gear & MPMM_MPMMCR_GEAR_MASK)
         << MPMM_MPMMCR_GEAR_POS);
}

/*
 * MPMM_V2 Module Helper Functions
 */
static struct mod_mpmm_v2_domain_ctx *get_domain_ctx(fwk_id_t domain_id)
{
    uint32_t idx = fwk_id_get_element_idx(domain_id);

    if (idx < mpmm_v2_ctx.mpmm_v2_domain_count) {
        return &mpmm_v2_ctx.domain_ctx[idx];
    } else {
        return NULL;
    }
}

static void mpmm_v2_core_counters_delta(
    struct mod_mpmm_v2_domain_ctx *domain_ctx,
    struct mod_mpmm_v2_core_ctx *core_ctx)
{
    int status;
    uint32_t gear_cnt = domain_ctx->domain_config->num_of_gears;
    uint32_t i;
    uint64_t counter_buff[MPMM_MAX_GEAR_COUNT];

    fwk_str_memset(counter_buff, 0, sizeof(counter_buff));

    status = mpmm_v2_ctx.amu_driver_api->get_counters(
        core_ctx->base_aux_counter_id, counter_buff, gear_cnt);
    if (status != FWK_SUCCESS) {
        FWK_LOG_DEBUG(
            "[MPMM_V2] %s @%d: AMU counter read fail, error=%d",
            __func__,
            __LINE__,
            status);
        return;
    }

    /*
     * Each MPMM gear has an associated counter. The counters are
     * indexed in the same order as the MPMM gears for the platform.
     */
    for (i = 0; i < gear_cnt; i++) {
        /* Calculate the delta */
        if (counter_buff[i] < core_ctx->cached_counters[i]) {
            /* Counter wraparound case */
            core_ctx->delta[i] = UINT64_MAX - core_ctx->cached_counters[i];
            core_ctx->delta[i] += counter_buff[i];
        } else {
            core_ctx->delta[i] = counter_buff[i] - core_ctx->cached_counters[i];
        }
        /* Store the last value */
        core_ctx->cached_counters[i] = counter_buff[i];
    }
}

/*
 * This function returns the selected gear based on the base_throtl_count value.
 */
static uint32_t mpmm_v2_core_gear_policy(
    struct mod_mpmm_v2_domain_ctx *domain_ctx,
    struct mod_mpmm_v2_core_ctx *core_ctx)
{
    uint32_t gear;
    uint32_t const num_of_gears = domain_ctx->domain_config->num_of_gears;
    uint32_t const highest_gear = num_of_gears - 1;

    /*
     * Select the most aggressive throttling gear whose counter delta is just
     * below the base_throtl_count value.
     */
    for (gear = 0; gear < num_of_gears; gear++) {
        if (core_ctx->delta[gear] <=
            domain_ctx->domain_config->base_throtl_count) {
            return gear;
        }
    }

    /*
     * It is not expected that all counters will cross the base_throtl_count. If
     * this scenario is encountered set throttling to a minimum.
     */
    return highest_gear;
}

/* set the gear for all cores */
static void mpmm_v2_domain_set_gears(struct mod_mpmm_v2_domain_ctx *ctx)
{
    uint32_t core_idx;
    struct mod_mpmm_v2_core_ctx *core_ctx;

    for (core_idx = 0; core_idx < ctx->num_cores; core_idx++) {
        core_ctx = &ctx->core_ctx[core_idx];
        if (core_ctx->online && core_ctx->needs_gear_update) {
            FWK_LOG_INFO(
                "domain %d: core %d: setting gear to %" PRIu32 "\n",
                ctx->domain_id.element.element_idx,
                core_ctx->core_id.sub_element.sub_element_idx,
                core_ctx->selected_gear & MPMM_MPMMCR_GEAR_MASK);
            mpmm_v2_core_set_gear(core_ctx);
        }
    }
}

static void mpmm_v2_core_evaluate_gear(
    struct mod_mpmm_v2_domain_ctx *domain_ctx,
    struct mod_mpmm_v2_core_ctx *core_ctx)
{
    uint32_t updated_gear;

    /* If counters are not enabled exit */
    if (!mpmm_v2_core_check_enabled(core_ctx)) {
        core_ctx->selected_gear = domain_ctx->domain_config->num_of_gears;
        return;
    }

    /* Read counters */
    mpmm_v2_core_counters_delta(domain_ctx, core_ctx);

    /* Gear selection policy */
    updated_gear = mpmm_v2_core_gear_policy(domain_ctx, core_ctx);
    if (updated_gear != core_ctx->selected_gear) {
        core_ctx->selected_gear = updated_gear;
        core_ctx->needs_gear_update = true;
    } else {
        core_ctx->needs_gear_update = false;
    }

    return;
}

static uint32_t mpmm_v2_evaluate_power_limit(
    struct mod_mpmm_v2_domain_ctx *domain_ctx)
{
    unsigned int i;
    uint32_t power_limit;
    uint8_t gear;
    int32_t power_factor = 0;
    const struct mod_mpmm_v2_domain_config *config = domain_ctx->domain_config;
    uint32_t num_of_offline_cores =
        domain_ctx->num_cores - domain_ctx->num_cores_online;

    for (i = 0; i < domain_ctx->num_cores; ++i) {
        if (!domain_ctx->core_ctx[i].online) {
            continue;
        }
        gear = domain_ctx->core_ctx[i].selected_gear;
        if (gear < config->num_of_gears) {
            power_factor += config->gear_weights[gear] -
                config->gear_weights[MPMM_DEFAULT_GEAR];
        }
    }

    /* Ignore negative power factor as we'll drop to minimum allowed power. */
    power_factor = FWK_MAX(0, power_factor + 100);

    /* Step down from max_power because weights are normalised to maximum power.
     */
    power_limit = ((1.0 * power_factor * config->max_power) / 100);

    /* Account for the offline cores. */
    power_limit += (num_of_offline_cores * config->core_offline_factor);

    /* Clamp power if it is out of allowed power range for the domain. */
    if (power_limit < config->min_power) {
        power_limit = config->min_power;
    } else if (power_limit > config->max_power) {
        power_limit = config->max_power;
    }

    return power_limit;
}

/* Check CPU status and update power limits accordingly */
static void mpmm_v2_monitor_and_control(
    struct mod_mpmm_v2_domain_ctx *domain_ctx)
{
    uint32_t core_idx;
    struct mod_mpmm_v2_core_ctx *core_ctx;

    if (domain_ctx->num_cores_online == 0) {
        return;
    }

    /* Core level algorithm */
    for (core_idx = 0; core_idx < domain_ctx->num_cores; core_idx++) {
        core_ctx = &domain_ctx->core_ctx[core_idx];

        if (!core_ctx->online) {
            continue;
        }

        mpmm_v2_core_evaluate_gear(domain_ctx, core_ctx);
    }

    /* Cache the last value */
    domain_ctx->power_limit = mpmm_v2_evaluate_power_limit(domain_ctx);
}

#ifdef BUILD_HAS_MOD_PERF_CONTROLLER
static int handle_controller_notification(
    struct mod_mpmm_v2_domain_ctx *domain_ctx,
    const struct fwk_event *event,
    struct fwk_event *resp_event)
{
    int status;
    uint32_t core_idx;
    struct fwk_event pd_resp_notif;
    struct mod_pd_power_state_pre_transition_notification_resp_params
        *pd_resp_params;
    struct mod_perf_controller_notification_params *controller_notif_params =
        (struct mod_perf_controller_notification_params *)event->params;

    domain_ctx->current_power_level = controller_notif_params->power_limit;

    if (!domain_ctx->wait_for_perf_transition) {
        return FWK_SUCCESS;
    }

    domain_ctx->wait_for_perf_transition = false;

    mpmm_v2_domain_set_gears(domain_ctx);

    /*
     * If a previous core wake-up sequence was delayed to re-evaluate the
     * MPMM gears and Perf Transition, then respond to the power domain
     * notification so the core can now be turned on.
     */
    pd_resp_params =
        (struct mod_pd_power_state_pre_transition_notification_resp_params *)
            pd_resp_notif.params;
    for (core_idx = 0; core_idx < domain_ctx->num_cores; core_idx++) {
        if (domain_ctx->core_ctx[core_idx].pd_blocked) {
            domain_ctx->core_ctx[core_idx].pd_blocked = false;

            status = fwk_get_delayed_response(
                domain_ctx->domain_id,
                domain_ctx->core_ctx[core_idx].cookie,
                &pd_resp_notif);
            if (status != FWK_SUCCESS) {
                return status;
            }

            pd_resp_params->status = FWK_SUCCESS;
            status = fwk_put_event(&pd_resp_notif);
            if (status != FWK_SUCCESS) {
                return status;
            }
        }
    }
    return FWK_SUCCESS;
}
#endif

static int mpmm_v2_core_init(
    struct mod_mpmm_v2_domain_ctx *domain_ctx,
    struct mod_mpmm_v2_core_ctx *core_ctx)
{
    uint32_t num_gears = mpmm_v2_core_get_number_of_gears(core_ctx);

    if (num_gears != domain_ctx->domain_config->num_of_gears) {
        return FWK_E_DEVICE;
    }

    /* Create counters storage */
    core_ctx->cached_counters = fwk_mm_calloc(
        domain_ctx->domain_config->num_of_gears, sizeof(uint64_t));
    core_ctx->delta = fwk_mm_calloc(
        domain_ctx->domain_config->num_of_gears, sizeof(uint64_t));

    core_ctx->initialized = true;

    return FWK_SUCCESS;
}

static int handle_pd_notification(
    struct mod_mpmm_v2_domain_ctx *domain_ctx,
    const struct fwk_event *event,
    struct fwk_event *resp_event)
{
    int status;
    struct mod_pd_power_state_pre_transition_notification_params
        *pre_state_params;
    struct mod_pd_power_state_transition_notification_params *post_state_params;
    struct mod_pd_power_state_pre_transition_notification_resp_params
        *pd_resp_params =
            (struct mod_pd_power_state_pre_transition_notification_resp_params
                 *)resp_event->params;
    uint32_t core_idx;

    /* Find the corresponding core */
    for (core_idx = 0; core_idx < domain_ctx->num_cores; core_idx++) {
        if (fwk_id_is_equal(
                domain_ctx->domain_config->core_config[core_idx].pd_id,
                event->source_id)) {
            break;
        }
    }

    if (core_idx >= domain_ctx->num_cores) {
        return FWK_E_PARAM;
    }

    if (fwk_id_is_equal(
            event->id, mod_pd_notification_id_power_state_pre_transition)) {
        pre_state_params =
            (struct mod_pd_power_state_pre_transition_notification_params *)
                event->params;
        pd_resp_params->status = FWK_SUCCESS;
        if (pre_state_params->target_state == MOD_PD_STATE_ON) {
            /* The core is transitioning to online */
            domain_ctx->num_cores_online++;
            domain_ctx->core_ctx[core_idx].online = true;
            /*
             * After core transition to ON the gear is set to zero as
             * defined by the hardware.
             */
            domain_ctx->core_ctx[core_idx].selected_gear = 0;
            domain_ctx->power_limit = mpmm_v2_evaluate_power_limit(domain_ctx);

            /*
             * If the power limit requested will not trigger a dvfs change,
             * there is no need to block the power domain state transition.
             */
            if (domain_ctx->power_limit >= domain_ctx->current_power_level) {
                return FWK_SUCCESS;
            }

            /* Block the power domain until the new level is applied */
            domain_ctx->core_ctx[core_idx].pd_blocked = true;
            domain_ctx->wait_for_perf_transition = true;
            resp_event->is_delayed_response = true;
            domain_ctx->core_ctx[core_idx].cookie = event->cookie;
        }

    } else if (fwk_id_is_equal(
                   event->id, mod_pd_notification_id_power_state_transition)) {
        post_state_params =
            (struct mod_pd_power_state_transition_notification_params *)
                event->params;
        if (post_state_params->state != MOD_PD_STATE_ON) {
            /* The core transitioned to offline */
            domain_ctx->num_cores_online--;
            domain_ctx->core_ctx[core_idx].online = false;
        } else {
            if (domain_ctx->core_ctx[core_idx].initialized != true) {
                status = mpmm_v2_core_init(
                    domain_ctx, &domain_ctx->core_ctx[core_idx]);
                if (status != FWK_SUCCESS) {
                    return status;
                }
            }
        }
    }

    return FWK_SUCCESS;
}

static int mpmm_v2_get_power_limit(fwk_id_t domain_id, uint32_t *power_limit)
{
    struct mod_mpmm_v2_domain_ctx *domain_ctx;
    uint32_t domain_idx;

    /*
     * Get the performance element id from the sub-element provided in the
     * function argument.
     */
    fwk_id_t perf_id = FWK_ID_ELEMENT(
#ifdef BUILD_HAS_MOD_PERF_CONTROLLER
        FWK_MODULE_IDX_PERF_CONTROLLER,
#else
        FWK_MODULE_IDX_DVFS,
#endif
        fwk_id_get_element_idx(domain_id));

    for (domain_idx = 0; domain_idx < mpmm_v2_ctx.mpmm_v2_domain_count;
         domain_idx++) {
        if (fwk_id_is_equal(
                mpmm_v2_ctx.domain_ctx[domain_idx].domain_config->perf_id,
                perf_id)) {
            break;
        }
    }

    if (domain_idx == mpmm_v2_ctx.mpmm_v2_domain_count) {
        return FWK_E_PARAM;
    }

    domain_ctx = get_domain_ctx(domain_id);

    if (domain_ctx->num_cores_online == 0) {
        *power_limit = domain_ctx->domain_config->max_power;
        return FWK_SUCCESS;
    }

    mpmm_v2_monitor_and_control(domain_ctx);

    *power_limit = domain_ctx->power_limit;

    /* set the flag to wait for the transition to complete. */
    if (domain_ctx->power_limit < domain_ctx->current_power_level) {
        domain_ctx->wait_for_perf_transition = true;
        return FWK_SUCCESS;
    }

    mpmm_v2_domain_set_gears(domain_ctx);
    return FWK_SUCCESS;
}

static struct interface_power_management_api mpmm_v2_pm_api = {
    .get_power_limit = mpmm_v2_get_power_limit,
    .set_power_limit = NULL,
};

/*
 * Framework handlers
 */
static int mpmm_v2_init(
    fwk_id_t module_id,
    unsigned int element_count,
    const void *data)
{
    if (element_count == 0) {
        return FWK_E_PARAM;
    }

    mpmm_v2_ctx.mpmm_v2_domain_count = element_count;
    mpmm_v2_ctx.domain_ctx =
        fwk_mm_calloc(element_count, sizeof(struct mod_mpmm_v2_domain_ctx));
    mpmm_v2_ctx.amu_driver_api_id = *(fwk_id_t *)data;

    return FWK_SUCCESS;
}

static int mpmm_v2_element_init(
    fwk_id_t domain_id,
    unsigned int sub_element_count,
    const void *data)
{
    int status;
    struct mod_mpmm_v2_domain_ctx *domain_ctx;
    struct mod_mpmm_v2_core_ctx *core_ctx;
    struct mod_mpmm_v2_core_config const *core_config;
    uint32_t core_idx;

    if ((sub_element_count == 0) ||
        (sub_element_count > MPMM_MAX_NUM_CORES_IN_DOMAIN)) {
        return FWK_E_PARAM;
    }

    domain_ctx = get_domain_ctx(domain_id);
    domain_ctx->domain_id = domain_id;
    domain_ctx->num_cores = sub_element_count;
    domain_ctx->wait_for_perf_transition = false;

    /* Initialize the configuration */
    domain_ctx->domain_config = data;

    if (domain_ctx->domain_config->num_of_gears > MPMM_MAX_GEAR_COUNT) {
        return FWK_E_SUPPORT;
    }

    /* Initialize each core */
    for (core_idx = 0; core_idx < domain_ctx->num_cores; core_idx++) {
        core_ctx = &domain_ctx->core_ctx[core_idx];
        core_ctx->core_id = fwk_id_build_sub_element_id(domain_id, core_idx);
        core_config = &domain_ctx->domain_config->core_config[core_idx];
        core_ctx->mpmm_v2 = (struct mpmm_reg *)core_config->mpmm_reg_base;
        core_ctx->base_aux_counter_id = core_config->base_aux_counter_id;

        if (core_config->core_starts_online) {
            status = mpmm_v2_core_init(domain_ctx, core_ctx);
            if (status != FWK_SUCCESS) {
                return status;
            }
            domain_ctx->num_cores_online++;
            core_ctx->online = true;
        }
    }

    return FWK_SUCCESS;
}

static int mpmm_v2_start(fwk_id_t id)
{
    int status;
    unsigned int i;
    struct mod_mpmm_v2_domain_ctx *domain_ctx;

    if (fwk_module_is_valid_module_id(id)) {
        return FWK_SUCCESS;
    }

    /* Subscribe to core power state transition */
    domain_ctx = get_domain_ctx(id);

    for (i = 0; i < domain_ctx->num_cores; i++) {
        status = fwk_notification_subscribe(
            mod_pd_notification_id_power_state_pre_transition,
            domain_ctx->domain_config->core_config[i].pd_id,
            domain_ctx->domain_id);
        if (status != FWK_SUCCESS) {
            return status;
        }

        status = fwk_notification_subscribe(
            mod_pd_notification_id_power_state_transition,
            domain_ctx->domain_config->core_config[i].pd_id,
            domain_ctx->domain_id);
        if (status != FWK_SUCCESS) {
            return status;
        }
    }

#ifdef BUILD_HAS_MOD_PERF_CONTROLLER
    status = fwk_notification_subscribe(
        FWK_ID_NOTIFICATION(
            FWK_MODULE_IDX_PERF_CONTROLLER,
            MOD_PERF_CONTROLLER_NOTIFICATION_IDX_PERF_SET),
        domain_ctx->domain_config->perf_id,
        domain_ctx->domain_id);
    if (status != FWK_SUCCESS) {
        return status;
    }
#endif

    return FWK_SUCCESS;
}

static int mpmm_v2_process_notification(
    const struct fwk_event *event,
    struct fwk_event *resp_event)
{
    int status;
    struct mod_mpmm_v2_domain_ctx *domain_ctx;

    fwk_assert(fwk_module_is_valid_element_id(event->target_id));
    domain_ctx = get_domain_ctx(event->target_id);
    if (domain_ctx == NULL) {
        return FWK_E_PARAM;
    }

    switch (fwk_id_get_module_idx(event->id)) {
    case FWK_MODULE_IDX_POWER_DOMAIN:
        status = handle_pd_notification(domain_ctx, event, resp_event);
        break;
#ifdef BUILD_HAS_MOD_PERF_CONTROLLER
    case FWK_MODULE_IDX_PERF_CONTROLLER:
        status = handle_controller_notification(domain_ctx, event, resp_event);
        break;
#endif
    default:
        status = FWK_SUCCESS;
        break;
    }

    return status;
}

static int mpmm_v2_bind(fwk_id_t id, unsigned int round)
{
    /* Bind in the second round */
    if ((round == 0) || (!fwk_module_is_valid_module_id(id))) {
        return FWK_SUCCESS;
    }

    return fwk_module_bind(
        FWK_ID_MODULE(mpmm_v2_ctx.amu_driver_api_id.common.module_idx),
        mpmm_v2_ctx.amu_driver_api_id,
        &mpmm_v2_ctx.amu_driver_api);
}

static int mpmm_v2_process_bind_request(
    fwk_id_t source_id,
    fwk_id_t target_id,
    fwk_id_t api_id,
    const void **api)
{
    if (!fwk_id_is_equal(
            api_id,
            FWK_ID_API(
                FWK_MODULE_IDX_MPMM_V2, MOD_MPMM_V2_GET_LIMIT_API_IDX))) {
        return FWK_E_PARAM;
    }

    *api = &mpmm_v2_pm_api;

    return FWK_SUCCESS;
}

const struct fwk_module module_mpmm_v2 = {
    .type = FWK_MODULE_TYPE_SERVICE,
    .api_count = 1,
    .init = mpmm_v2_init,
    .element_init = mpmm_v2_element_init,
    .start = mpmm_v2_start,
    .bind = mpmm_v2_bind,
    .process_bind_request = mpmm_v2_process_bind_request,
    .process_notification = mpmm_v2_process_notification,
};
