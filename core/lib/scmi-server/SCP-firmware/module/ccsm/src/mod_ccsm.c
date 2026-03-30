/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "ccsm_drv.h"

#include <mod_ccsm.h>
#include <mod_clock.h>
#include <mod_power_domain.h>
#include <mod_timer.h>

#include <fwk_id.h>
#include <fwk_log.h>
#include <fwk_macros.h>
#include <fwk_mm.h>
#include <fwk_module.h>
#include <fwk_status.h>

#include <stddef.h>
#include <stdint.h>

/* Device context */
struct ccsm_dev_ctx {
    bool initialized;
    uint64_t current_nominal_clock_rate_hz;
    uint64_t current_fallback_clock_rate_hz;
    enum mod_clock_state current_state;
    struct mod_timer_api *timer_api;
    const struct mod_ccsm_dev_config *config;
};

/* Module context */
struct ccsm_ctx {
    struct ccsm_dev_ctx *dev_ctx_table;
    unsigned int dev_count;
};

static struct ccsm_ctx module_ctx;

/*
 * Static helper functions
 */
static inline struct ccsm_dev_ctx *get_context_from_id(fwk_id_t id)
{
    return module_ctx.dev_ctx_table + fwk_id_get_element_idx(id);
}

static int compare_rate_entry(const void *a, const void *b)
{
    struct mod_ccsm_clock_rate *key = (struct mod_ccsm_clock_rate *)a;
    struct mod_ccsm_clock_rate *element = (struct mod_ccsm_clock_rate *)b;

    return (key->nominal_clock_rate_hz - element->nominal_clock_rate_hz);
}

static int get_indexed_rates(
    struct ccsm_dev_ctx *ctx,
    uint64_t target_rate,
    struct mod_ccsm_clock_rate *clock_rates)
{
    struct mod_ccsm_clock_rate *current_rate_entry;

    if (ctx == NULL) {
        return FWK_E_PARAM;
    }

    if (clock_rates == NULL) {
        return FWK_E_PARAM;
    }

    /*
     * Perform a binary search to find the entry matching the requested rate.
     * The table must be sorted in ascending order.
     */
    current_rate_entry = (struct mod_ccsm_clock_rate *)bsearch(
        &target_rate,
        ctx->config->rate_table,
        ctx->config->rate_count,
        sizeof(struct mod_ccsm_clock_rate),
        compare_rate_entry);

    if (current_rate_entry == NULL) {
        return FWK_E_PARAM;
    }

    clock_rates->nominal_clock_rate_hz =
        current_rate_entry->nominal_clock_rate_hz;
    clock_rates->fallback_clock_rate_hz =
        current_rate_entry->fallback_clock_rate_hz;

    return FWK_SUCCESS;
}

static int compare_register_rate_entry(const void *a, const void *b)
{
    struct mod_ccsm_clock_rate *key = (struct mod_ccsm_clock_rate *)a;
    struct mod_ccsm_clock_rate *element = (struct mod_ccsm_clock_rate *)b;

    return (key->nominal_clock_rate_hz - element->nominal_clock_rate_hz);
}

static int get_register_value_rate(
    struct ccsm_dev_ctx *ctx,
    uint64_t rate,
    uint32_t *pll_settings_0,
    uint32_t *pll_settings_1)
{
    struct mod_ccsm_clock_rate_reg_value *current_register_rate_entry;

    if (ctx == NULL) {
        return FWK_E_PARAM;
    }

    if (!fwk_expect((pll_settings_0 != NULL) && (pll_settings_1 != NULL))) {
        return FWK_E_PARAM;
    }

    /*
     * Perform a binary search to find the entry matching the requested rate.
     * The table must be sorted in ascending order.
     */
    current_register_rate_entry =
        (struct mod_ccsm_clock_rate_reg_value *)bsearch(
            &rate,
            ctx->config->register_rate_table,
            ctx->config->register_rate_count,
            sizeof(struct mod_ccsm_clock_rate_reg_value),
            compare_register_rate_entry);

    if (current_register_rate_entry == NULL) {
        return FWK_E_PARAM;
    }

    *pll_settings_0 = current_register_rate_entry->pll_settings_0;
    *pll_settings_1 = current_register_rate_entry->pll_settings_1;

    return FWK_SUCCESS;
}

static int get_clear_irq_status(struct ccsm_dev_ctx *ctx, uint32_t *status)
{
    int ret;
    struct ccsm_reg *reg;

    reg = (struct ccsm_reg *)ctx->config->base_address;

    /* Get IRQ_STATUS from driver and clear register. */
    ret = ccsm_drv_get_clear_irq_status(reg, status);

    if (ret != FWK_SUCCESS) {
        return ret;
    }

    /* Wait for IRQ_STATUS to be clear. */
    return ctx->timer_api->wait(
        ctx->config->timer_id,
        MOD_CCSM_COMMAND_WAIT_TIMEOUT,
        ccsm_drv_is_irq_status_clear,
        (void *)reg);
}

static int ccsm_send_await_request(
    struct ccsm_dev_ctx *ctx,
    enum mod_ccsm_request_type req_type)
{
    int ret;
    uint32_t status;
    struct ccsm_reg *reg;

    reg = (struct ccsm_reg *)ctx->config->base_address;

    /* Set request through the driver. */
    ret = ccsm_drv_set_request(reg, req_type);

    if (ret != FWK_SUCCESS) {
        return ret;
    }

    /* Await completion of the request. */
    ret = ctx->timer_api->wait(
        ctx->config->timer_id,
        MOD_CCSM_COMMAND_WAIT_TIMEOUT,
        ccsm_drv_is_request_complete,
        (void *)reg);

    if (ret != FWK_SUCCESS) {
        return ret;
    }

    /* Clear IRQ_STATUS on completion. */
    ret = get_clear_irq_status(ctx, &status);

    if (ret != FWK_SUCCESS) {
        return ret;
    }

    /* Clear request registers to allow subsequent requests. */
    return ccsm_drv_clear_request(reg);
}

static int set_static_pll_settings(struct ccsm_dev_ctx *ctx)
{
    int status;

    status = ccsm_drv_set_pll_static_settings(
        (struct ccsm_reg *)ctx->config->base_address,
        MOD_CCSM_PLL_0,
        ctx->config->pll_0_static_reg_value);
    if (status != FWK_SUCCESS) {
        return status;
    }

    status = ccsm_drv_set_pll_static_settings(
        (struct ccsm_reg *)ctx->config->base_address,
        MOD_CCSM_PLL_1,
        ctx->config->pll_1_static_reg_value);

    return status;
}

static int set_fallback_dynamic_settings(
    struct ccsm_dev_ctx *ctx,
    uint64_t fallback_clock_rate_hz)
{
    int status;
    uint32_t settings_0_register_value;
    uint32_t settings_1_register_value;

    status = get_register_value_rate(
        ctx,
        fallback_clock_rate_hz,
        &settings_0_register_value,
        &settings_1_register_value);
    if (status != FWK_SUCCESS) {
        return status;
    }

    status = ccsm_drv_set_pll_dynamic_settings(
        (struct ccsm_reg *)ctx->config->base_address,
        MOD_CCSM_PLL_FALLBACK,
        settings_0_register_value,
        settings_1_register_value);

    return status;
}

static inline uint64_t clock_rate_round_nearest(
    uint64_t rate,
    uint64_t clock_rate_step)
{
    uint64_t rate_rounded, rate_rounded_alt;

    rate_rounded = FWK_ALIGN_PREVIOUS(rate, clock_rate_step);
    rate_rounded_alt = FWK_ALIGN_NEXT(rate, clock_rate_step);

    /* Select the rounded rate that is closest to the given rate */
    if ((rate - rate_rounded) > (rate_rounded_alt - rate)) {
        return rate_rounded_alt;
    } else {
        return rate_rounded;
    }
}

static inline int is_rate_entry_valid(
    struct ccsm_dev_ctx *ctx,
    struct mod_ccsm_clock_rate rate_entry)
{
    if (rate_entry.nominal_clock_rate_hz > ctx->config->max_clock_rate_hz) {
        return FWK_E_SUPPORT;
    } else if (
        rate_entry.nominal_clock_rate_hz < ctx->config->min_clock_rate_hz) {
        return FWK_E_SUPPORT;
    } else if (
        (rate_entry.fallback_clock_rate_hz > 0) &&
        (rate_entry.fallback_clock_rate_hz < ctx->config->min_clock_rate_hz)) {
        return FWK_E_SUPPORT;
    } else {
        return FWK_SUCCESS;
    }
}

static int get_rounded_rate(
    struct ccsm_dev_ctx *ctx,
    uint64_t *rate,
    enum mod_clock_round_mode round_mode)
{
    if ((*rate % ctx->config->clock_rate_step_hz) > 0) {
        switch (round_mode) {
        case MOD_CLOCK_ROUND_MODE_NONE:
            return FWK_E_RANGE;
        case MOD_CLOCK_ROUND_MODE_NEAREST:
            *rate = clock_rate_round_nearest(
                *rate, ctx->config->clock_rate_step_hz);
            break;
        case MOD_CLOCK_ROUND_MODE_DOWN:
            *rate = FWK_ALIGN_PREVIOUS(*rate, ctx->config->clock_rate_step_hz);
            break;
        case MOD_CLOCK_ROUND_MODE_UP:
            *rate = FWK_ALIGN_NEXT(*rate, ctx->config->clock_rate_step_hz);
            break;
        default:
            return FWK_E_SUPPORT;
        }
    }

    return FWK_SUCCESS;
}

static int ccsm_clock_select_rates(
    struct ccsm_dev_ctx *ctx,
    uint64_t rate,
    enum mod_clock_round_mode round_mode,
    struct mod_ccsm_clock_rate *clock_rates)
{
    int status;

    if (ctx->config->rate_lookup_table_is_provided) {
        status = get_indexed_rates(ctx, rate, clock_rates);
        if (status != FWK_SUCCESS) {
            return status;
        }
    } else {
        clock_rates->nominal_clock_rate_hz = rate;
    }

    status = get_rounded_rate(
        ctx, &(clock_rates->nominal_clock_rate_hz), round_mode);
    if (status != FWK_SUCCESS) {
        return status;
    }

    if (!ctx->config->rate_lookup_table_is_provided) {
        /* Set fallback as fraction of nominal frequency if above threshold. */
        if (clock_rates->nominal_clock_rate_hz >=
            ctx->config->minimum_clock_rate_fallback_enable_hz) {
            clock_rates->fallback_clock_rate_hz =
                ctx->config->fallback_clock_percentage *
                (clock_rates->nominal_clock_rate_hz / 100);
        } else {
            clock_rates->fallback_clock_rate_hz = 0;
        }
    }

    status = get_rounded_rate(
        ctx, &(clock_rates->fallback_clock_rate_hz), MOD_CLOCK_ROUND_MODE_DOWN);
    if (status != FWK_SUCCESS) {
        return status;
    }

    status = is_rate_entry_valid(ctx, *clock_rates);

    return status;
}

static int ccsm_mod_set_configuration(
    struct ccsm_dev_ctx *ctx,
    enum mod_ccsm_mod_select mod_id,
    const struct mod_ccsm_mod_config *config)
{
    uint32_t value;

    if (mod_id >= ctx->config->modulator_count) {
        return FWK_E_RANGE;
    }

    if ((config->numerator_regular < config->numerator_oc) ||
        (config->denominator < config->numerator_oc) ||
        (config->denominator < config->numerator_regular)) {
        return FWK_E_PARAM;
    }

    value =
        (((config->numerator_regular
           << CCSM_MODULATOR_SETTINGS_NUMERATOR_REGULAR_POS) &
          CCSM_MODULATOR_SETTINGS_NUMERATOR_REGULAR_MASK) |
         ((config->numerator_oc
           << CCSM_MODULATOR_SETTINGS_NUMERATOR_PMIC_OC_POS) &
          CCSM_MODULATOR_SETTINGS_NUMERATOR_PMIC_OC_MASK) |
         ((config->denominator << CCSM_MODULATOR_SETTINGS_DENOMINATOR_POS) &
          CCSM_MODULATOR_SETTINGS_DENOMINATOR_MASK));

    return ccsm_drv_set_mod_configuration(
        (struct ccsm_reg *)ctx->config->base_address, mod_id, value);
}

static int ccsm_dm_set_configuration(
    struct ccsm_dev_ctx *ctx,
    const struct mod_ccsm_dm_config *config)
{
    uint32_t mitigation_duration;
    uint32_t transition_pause;
    uint32_t value;

    mitigation_duration =
        ((config->mitigation_duration
          << CCSM_DROOP_MITIGATION_STRATEGY_MITIGATION_DURATION_POS) &
         CCSM_DROOP_MITIGATION_STRATEGY_MITIGATION_DURATION_MASK);

    transition_pause =
        ((config->transition_pause
          << CCSM_DROOP_MITIGATION_STRATEGY_TRANSITION_PAUSE_POS) &
         CCSM_DROOP_MITIGATION_STRATEGY_TRANSITION_PAUSE_MASK);

    value =
        (config->strategy | config->dd | config->soff | transition_pause |
         mitigation_duration);

    return ccsm_drv_set_dm_configuration(
        (struct ccsm_reg *)ctx->config->base_address, value);
}

static int ccsm_dm_get_configuration(
    struct ccsm_dev_ctx *ctx,
    struct mod_ccsm_dm_config *config)
{
    int status;
    uint32_t value;

    status = ccsm_drv_get_dm_configuration(
        (struct ccsm_reg *)ctx->config->base_address, &value);
    if (status != FWK_SUCCESS) {
        return status;
    }

    if (value & MOD_CCSM_DM_NOM_ONLY) {
        config->strategy = MOD_CCSM_DM_NOM_ONLY;
    } else if (value & MOD_CCSM_DM_SW_FB) {
        config->strategy = MOD_CCSM_DM_SW_FB;
    } else if (value & MOD_CCSM_DM_CLK_ON_DVFS) {
        config->strategy = MOD_CCSM_DM_CLK_ON_DVFS;
    } else {
        config->strategy = MOD_CCSM_DM_STOP_CLK;
    }

    if (value & MOD_CCSM_DM_TPIP) {
        config->dd = MOD_CCSM_DM_TPIP;
    } else {
        config->dd = MOD_CCSM_DM_ARM_DD;
    }

    if (value & MOD_CCSM_DM_SW_SOFF_STOP) {
        config->soff = MOD_CCSM_DM_SW_SOFF_STOP;
    } else {
        config->soff = MOD_CCSM_DM_SW_SOFF_IGNORE;
    }

    config->transition_pause =
        ((value & CCSM_DROOP_MITIGATION_STRATEGY_TRANSITION_PAUSE_MASK) >>
         CCSM_DROOP_MITIGATION_STRATEGY_TRANSITION_PAUSE_POS);

    config->mitigation_duration =
        ((value & CCSM_DROOP_MITIGATION_STRATEGY_MITIGATION_DURATION_MASK) >>
         CCSM_DROOP_MITIGATION_STRATEGY_MITIGATION_DURATION_POS);

    return FWK_SUCCESS;
}

static int sw_fb_dm_config_handler(
    struct ccsm_dev_ctx *ctx,
    struct mod_ccsm_clock_rate rate_entry)
{
    int status;
    struct mod_ccsm_dm_config dm_config;

    /* Fallback remains enabled, no change required. */
    if ((rate_entry.fallback_clock_rate_hz != 0) &&
        (ctx->current_fallback_clock_rate_hz != 0)) {
        return FWK_SUCCESS;
    }

    /* Fallback remains disabled, no change required. */
    if ((rate_entry.fallback_clock_rate_hz == 0) &&
        (ctx->current_fallback_clock_rate_hz == 0)) {
        return FWK_SUCCESS;
    }

    status = ccsm_dm_get_configuration(ctx, &dm_config);
    if (status != FWK_SUCCESS) {
        return status;
    }

    if ((rate_entry.fallback_clock_rate_hz == 0) &&
        (dm_config.strategy == MOD_CCSM_DM_SW_FB)) {
        /* Switch to nominal only so fallback can be disabled */
        dm_config.strategy = MOD_CCSM_DM_NOM_ONLY;

        status = ccsm_dm_set_configuration(ctx, &dm_config);
    } else if (
        (ctx->current_fallback_clock_rate_hz == 0) &&
        (dm_config.strategy == MOD_CCSM_DM_NOM_ONLY)) {
        /* Restore switch to fallback configuration */
        dm_config.strategy = MOD_CCSM_DM_SW_FB;

        status = ccsm_dm_set_configuration(ctx, &dm_config);
    } else {
        status = FWK_SUCCESS;
    }

    return status;
}

/*! Get register values from PLL driver and update CCSM. */
static int ccsm_clock_update_registers(
    struct ccsm_dev_ctx *ctx,
    struct mod_ccsm_clock_rate *clock_rates)
{
    int status;
    uint32_t settings_0_register_value;
    uint32_t settings_1_register_value;

    if (!ctx->initialized) {
        status = set_static_pll_settings(ctx);
        if (status != FWK_SUCCESS) {
            return status;
        }
    }

    status = get_register_value_rate(
        ctx,
        clock_rates->nominal_clock_rate_hz,
        &settings_0_register_value,
        &settings_1_register_value);
    if (status != FWK_SUCCESS) {
        return status;
    }

    status = ccsm_drv_set_pll_dynamic_settings(
        (struct ccsm_reg *)ctx->config->base_address,
        MOD_CCSM_PLL_NOMINAL,
        settings_0_register_value,
        settings_1_register_value);
    if (status != FWK_SUCCESS) {
        return status;
    }

    if (clock_rates->fallback_clock_rate_hz != 0) {
        status = set_fallback_dynamic_settings(
            ctx, clock_rates->fallback_clock_rate_hz);
    }

    return status;
}

/*! Handle CCSM initialization. */
static int ccsm_device_setup(struct ccsm_dev_ctx *ctx, uint64_t rate)
{
    int status;
    enum mod_ccsm_mod_select mod_n;
    struct mod_ccsm_clock_rate clock_rates;

    /* Select appropriate rates for nominal and fallback */
    status = ccsm_clock_select_rates(
        ctx, rate, MOD_CLOCK_ROUND_MODE_NEAREST, &clock_rates);
    if (status != FWK_SUCCESS) {
        return status;
    }

    status =
        ccsm_dm_set_configuration(ctx, ctx->config->droop_mitigation_default);
    if (status != FWK_SUCCESS) {
        return status;
    }

    status = ccsm_send_await_request(ctx, CCSM_REQUEST_TYPE_SET_DM);
    if (status != FWK_SUCCESS) {
        return status;
    }

    for (mod_n = MOD_CCSM_MOD_CORE0; mod_n < ctx->config->modulator_count;
         mod_n++) {
        status = ccsm_mod_set_configuration(
            ctx, mod_n, ctx->config->modulator_default);
        if (status != FWK_SUCCESS) {
            return status;
        }
    }

    status = ccsm_send_await_request(ctx, CCSM_REQUEST_TYPE_SET_MOD);
    if (status != FWK_SUCCESS) {
        return status;
    }

    status = ccsm_clock_update_registers(ctx, &clock_rates);
    if (status != FWK_SUCCESS) {
        return status;
    }

    status = ccsm_send_await_request(ctx, CCSM_REQUEST_TYPE_SET_DVFS);
    if (status != FWK_SUCCESS) {
        return status;
    }

    ctx->initialized = true;
    ctx->current_nominal_clock_rate_hz = clock_rates.nominal_clock_rate_hz;
    ctx->current_fallback_clock_rate_hz = clock_rates.fallback_clock_rate_hz;

    return FWK_SUCCESS;
}

/*
 * Clock driver API functions
 */

static int ccsm_clock_set_rate(
    fwk_id_t dev_id,
    uint64_t rate,
    enum mod_clock_round_mode round_mode)
{
    struct ccsm_dev_ctx *ctx;
    struct mod_ccsm_clock_rate clock_rates;
    int status;

    if (!fwk_module_is_valid_element_id(dev_id)) {
        return FWK_E_PARAM;
    }

    ctx = get_context_from_id(dev_id);
    if (ctx->initialized == false) {
        return ccsm_device_setup(ctx, rate);
    }

    if (ctx->current_state == MOD_CLOCK_STATE_STOPPED) {
        return FWK_E_PWRSTATE;
    }

    /* Select appropriate rates for nominal and fallback */
    status = ccsm_clock_select_rates(ctx, rate, round_mode, &clock_rates);
    if (status != FWK_SUCCESS) {
        return status;
    }

    if (clock_rates.nominal_clock_rate_hz ==
        ctx->current_nominal_clock_rate_hz) {
        return FWK_SUCCESS;
    }

    status = ccsm_clock_update_registers(ctx, &clock_rates);
    if (status != FWK_SUCCESS) {
        return status;
    }

    if (ctx->config->droop_mitigation_default->strategy == MOD_CCSM_DM_SW_FB) {
        status = sw_fb_dm_config_handler(ctx, clock_rates);
        if (status != FWK_SUCCESS) {
            return status;
        }
    }

    /* Send DVFS command as required */
    if (clock_rates.nominal_clock_rate_hz >
        ctx->current_nominal_clock_rate_hz) {
        status = ccsm_send_await_request(ctx, CCSM_REQUEST_TYPE_GO_UP);

        if (status != FWK_SUCCESS) {
            return status;
        }

        status = ccsm_send_await_request(ctx, CCSM_REQUEST_TYPE_GO_UP2);
    } else {
        status = ccsm_send_await_request(ctx, CCSM_REQUEST_TYPE_GO_DN);

        if (status != FWK_SUCCESS) {
            return status;
        }

        status = ccsm_send_await_request(ctx, CCSM_REQUEST_TYPE_GO_DN2);
    }

    if (status != FWK_SUCCESS) {
        return status;
    }

    ctx->current_nominal_clock_rate_hz = clock_rates.nominal_clock_rate_hz;
    ctx->current_fallback_clock_rate_hz = clock_rates.fallback_clock_rate_hz;

    return FWK_SUCCESS;
}

static int ccsm_clock_get_rate(fwk_id_t dev_id, uint64_t *rate)
{
    struct ccsm_dev_ctx *ctx;

    if (!fwk_module_is_valid_element_id(dev_id)) {
        return FWK_E_PARAM;
    }

    ctx = get_context_from_id(dev_id);

    *rate = ctx->current_nominal_clock_rate_hz;

    return FWK_SUCCESS;
}

static int ccsm_clock_get_rate_from_index(
    fwk_id_t dev_id,
    unsigned int rate_index,
    uint64_t *rate)
{
    struct ccsm_dev_ctx *ctx;

    if (!fwk_module_is_valid_element_id(dev_id)) {
        return FWK_E_PARAM;
    }

    if (rate == NULL) {
        return FWK_E_PARAM;
    }

    ctx = get_context_from_id(dev_id);

    if (ctx->config->rate_lookup_table_is_provided) {
        if (rate_index >= ctx->config->rate_count) {
            return FWK_E_PARAM;
        }

        *rate = ctx->config->rate_table[rate_index].nominal_clock_rate_hz;
        return FWK_SUCCESS;
    } else {
        /* Rate table is required to return rate from index. */
        return FWK_E_SUPPORT;
    }
}

static int ccsm_clock_set_state(fwk_id_t dev_id, enum mod_clock_state state)
{
    if (state == MOD_CLOCK_STATE_RUNNING) {
        return FWK_SUCCESS;
    }

    /* PLLs can only be stopped by a parent power domain state change. */
    return FWK_E_SUPPORT;
}

static int ccsm_clock_get_state(fwk_id_t dev_id, enum mod_clock_state *state)
{
    struct ccsm_dev_ctx *ctx;

    if (!fwk_module_is_valid_element_id(dev_id)) {
        return FWK_E_PARAM;
    }

    if (state == NULL) {
        return FWK_E_PARAM;
    }

    ctx = get_context_from_id(dev_id);
    *state = ctx->current_state;

    return FWK_SUCCESS;
}

static int ccsm_clock_get_range(fwk_id_t dev_id, struct mod_clock_range *range)
{
    struct ccsm_dev_ctx *ctx;

    if (!fwk_module_is_valid_element_id(dev_id)) {
        return FWK_E_PARAM;
    }

    if (range == NULL) {
        return FWK_E_PARAM;
    }

    ctx = get_context_from_id(dev_id);

    if (ctx->config->rate_lookup_table_is_provided) {
        range->rate_type = MOD_CLOCK_RATE_TYPE_DISCRETE;
        range->min = ctx->config->rate_table[0].nominal_clock_rate_hz;
        range->max = ctx->config->rate_table[ctx->config->rate_count - 1]
                         .nominal_clock_rate_hz;
        range->rate_count = ctx->config->rate_count;
    } else {
        range->rate_type = MOD_CLOCK_RATE_TYPE_CONTINUOUS;
        range->min = ctx->config->min_clock_rate_hz;
        range->max = ctx->config->max_clock_rate_hz;
        range->step = ctx->config->clock_rate_step_hz;
    }

    return FWK_SUCCESS;
}

static int ccsm_clock_power_state_change(fwk_id_t dev_id, unsigned int state)
{
    uint64_t rate;
    struct ccsm_dev_ctx *ctx;

    ctx = get_context_from_id(dev_id);

    if (state != MOD_PD_STATE_ON) {
        return FWK_SUCCESS;
    }

    ctx->current_state = MOD_CLOCK_STATE_RUNNING;

    if (ctx->initialized) {
        /* Restore the previous rate */
        rate = ctx->current_nominal_clock_rate_hz;
    } else {
        /* Initialize the PLL to its default rate */
        rate = ctx->config->default_rates_table->nominal_clock_rate_hz;
    }

    return ccsm_device_setup(ctx, rate);
}

static int ccsm_clock_power_state_pending_change(
    fwk_id_t dev_id,
    unsigned int current_state,
    unsigned int next_state)
{
    struct ccsm_dev_ctx *ctx;

    ctx = get_context_from_id(dev_id);

    if (next_state == MOD_PD_STATE_OFF) {
        /* Just mark the PLL as stopped */
        ctx->current_state = MOD_CLOCK_STATE_STOPPED;
    }

    return FWK_SUCCESS;
}

static const struct mod_clock_drv_api api_ccsm_clock = {
    .set_rate = ccsm_clock_set_rate,
    .get_rate = ccsm_clock_get_rate,
    .get_rate_from_index = ccsm_clock_get_rate_from_index,
    .set_state = ccsm_clock_set_state,
    .get_state = ccsm_clock_get_state,
    .get_range = ccsm_clock_get_range,
    .process_power_transition = ccsm_clock_power_state_change,
    .process_pending_power_transition = ccsm_clock_power_state_pending_change,
};

/*
 * Framework handler functions
 */
static int ccsm_init(
    fwk_id_t module_id,
    unsigned int element_count,
    const void *data)
{
    module_ctx.dev_count = element_count;

    if (element_count == 0) {
        return FWK_SUCCESS;
    }

    module_ctx.dev_ctx_table =
        fwk_mm_calloc(element_count, sizeof(struct ccsm_dev_ctx));

    return FWK_SUCCESS;
}

static int ccsm_element_init(
    fwk_id_t element_id,
    unsigned int unused,
    const void *data)
{
    int status;
    struct ccsm_dev_ctx *ctx;
    struct mod_ccsm_clock_rate rate_entry;
    const struct mod_ccsm_dev_config *dev_config = data;
    uint32_t element_idx = fwk_id_get_element_idx(element_id);
    uint32_t rate_n;
    uint64_t previous_nominal_rate_hz = 0;

    if (element_idx >= module_ctx.dev_count) {
        return FWK_E_DATA;
    }

    ctx = module_ctx.dev_ctx_table + element_idx;

    ctx->config = dev_config;

    for (rate_n = 0; rate_n < ctx->config->rate_count; rate_n++) {
        rate_entry = ctx->config->rate_table[rate_n];
        if (rate_entry.fallback_clock_rate_hz >
            rate_entry.nominal_clock_rate_hz) {
            return FWK_E_DATA;
        }

        if (rate_entry.nominal_clock_rate_hz < previous_nominal_rate_hz) {
            return FWK_E_DATA;
        }

        status = is_rate_entry_valid(ctx, rate_entry);
        if (status != FWK_SUCCESS) {
            return status;
        }

        previous_nominal_rate_hz = rate_entry.nominal_clock_rate_hz;
    }

    ctx->current_nominal_clock_rate_hz =
        dev_config->default_rates_table->nominal_clock_rate_hz;
    ctx->initialized = false;
    ctx->current_state = MOD_CLOCK_STATE_STOPPED;

    return FWK_SUCCESS;
}

static int ccsm_bind(fwk_id_t id, unsigned int round)
{
    int status;
    struct ccsm_dev_ctx *ctx;

    if ((round == 1) && fwk_id_is_type(id, FWK_ID_TYPE_ELEMENT)) {
        ctx = get_context_from_id(id);

        /* Bind to timer API */
        status = fwk_module_bind(
            ctx->config->timer_id,
            FWK_ID_API(FWK_MODULE_IDX_TIMER, MOD_TIMER_API_IDX_TIMER),
            &ctx->timer_api);

        if (status != FWK_SUCCESS) {
            return status;
        }

        if (ctx->timer_api->wait == NULL) {
            return FWK_E_DATA;
        }
    }

    return FWK_SUCCESS;
}

static int ccsm_process_bind_request(
    fwk_id_t requester_id,
    fwk_id_t id,
    fwk_id_t api_type,
    const void **api)
{
    switch (fwk_id_get_api_idx(api_type)) {
    case MOD_CCSM_CLOCK_API:
        *api = &api_ccsm_clock;
        break;
    default:
        return FWK_E_PARAM;
    }

    return FWK_SUCCESS;
}

const struct fwk_module module_ccsm = {
    .type = FWK_MODULE_TYPE_DRIVER,
    .api_count = (unsigned int)MOD_CCSM_API_COUNT,
    .bind = ccsm_bind,
    .event_count = 0,
    .init = ccsm_init,
    .element_init = ccsm_element_init,
    .process_bind_request = ccsm_process_bind_request,
};
