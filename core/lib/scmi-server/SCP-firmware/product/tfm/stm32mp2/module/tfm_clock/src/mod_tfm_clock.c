/*
 * Arm SCP/MCP Software
 * Copyright (c) 2022-2023, Linaro Limited and Contributors. All rights reserved.
 * Copyright (c) 2024, STMicroelectronics and the Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Interface SCP-firmware clock module with stm32mp2 clock resources.
 */

#include <fwk_macros.h>
#include <fwk_mm.h>
#include <fwk_module.h>
#include <fwk_log.h>

#include <mod_clock.h>
#include <mod_tfm_clock.h>

#include <clk.h>

#include <stdbool.h>
#include <inttypes.h>


#define MOD_NAME "[TFM CLOCK] "

/* Clock device context */
struct tfm_clock_dev_ctx {
    struct clk *clk;
    bool enabled;
};

/* Clock module context */
struct tfm_clock_module_ctx {
    struct tfm_clock_dev_ctx *dev_ctx;
    unsigned int dev_count;
};

static struct tfm_clock_module_ctx module_ctx;

static struct tfm_clock_dev_ctx *elt_id_to_ctx(fwk_id_t dev_id)
{
    if (!fwk_module_is_valid_element_id(dev_id)) {
        return NULL;
    }

    return module_ctx.dev_ctx + fwk_id_get_element_idx(dev_id);
}

static bool is_exposed(struct tfm_clock_dev_ctx *ctx)
{
    return ctx->clk != NULL;
}

/*
 * Clock driver API functions
 */
static int get_rate(fwk_id_t dev_id, uint64_t *rate)
{
    struct tfm_clock_dev_ctx *ctx = elt_id_to_ctx(dev_id);

    if ((ctx == NULL) || (rate == NULL)) {
        return FWK_E_PARAM;
    }

    if (!is_exposed(ctx)) {
        *rate = 0;
        return FWK_SUCCESS;
    }

    *rate = clk_get_rate(ctx->clk);

    FWK_LOG_DEBUG(
        MOD_NAME "(%u/%s) rate: %ul",
        fwk_id_get_element_idx(dev_id), clk_get_name(ctx->clk),
        *rate);

    return FWK_SUCCESS;
}

static int set_state(fwk_id_t dev_id, enum mod_clock_state state)
{
    struct tfm_clock_dev_ctx *ctx = elt_id_to_ctx(dev_id);

    if (ctx == NULL) {
        return FWK_E_PARAM;
    }

    switch (state) {
    case MOD_CLOCK_STATE_STOPPED:
    case MOD_CLOCK_STATE_RUNNING:
        break;
    default:
        return FWK_E_PARAM;
    }

    if (!is_exposed(ctx)) {
        if (state == MOD_CLOCK_STATE_STOPPED) {
            return FWK_SUCCESS;
        } else {
            return FWK_E_ACCESS;
        }
    }

    if (state == MOD_CLOCK_STATE_STOPPED) {
        if (ctx->enabled) {
            FWK_LOG_DEBUG(
                MOD_NAME "(%u/%s) disable",
                fwk_id_get_element_idx(dev_id), clk_get_name(ctx->clk));

            clk_disable(ctx->clk);
            ctx->enabled = false;
        } else {
            FWK_LOG_DEBUG(
                MOD_NAME "(%u/%s) is already OFF",
                fwk_id_get_element_idx(dev_id), clk_get_name(ctx->clk));
        }
    } else {
        if (!ctx->enabled) {
            FWK_LOG_DEBUG(
                MOD_NAME "(%u/%s) enable",
                fwk_id_get_element_idx(dev_id), clk_get_name(ctx->clk));

            if (clk_enable(ctx->clk)) {
                return FWK_E_PARAM;
            }

            ctx->enabled = true;
        } else {
            FWK_LOG_DEBUG(
                MOD_NAME "(%u/%s) is already ON",
                fwk_id_get_element_idx(dev_id), clk_get_name(ctx->clk));
        }
    }

    return FWK_SUCCESS;
}

static int get_state(fwk_id_t dev_id, enum mod_clock_state *state)
{
    struct tfm_clock_dev_ctx *ctx = elt_id_to_ctx(dev_id);

    if ((ctx == NULL) || (state == NULL)) {
        return FWK_E_PARAM;
    }

    if (!is_exposed(ctx)) {
        *state = MOD_CLOCK_STATE_STOPPED;
        return FWK_SUCCESS;
    }

    if (ctx->enabled) {
        *state = MOD_CLOCK_STATE_RUNNING;
    } else {
        *state = MOD_CLOCK_STATE_STOPPED;
    }

    FWK_LOG_DEBUG(
        MOD_NAME "(%u/%s) is %s",
        fwk_id_get_element_idx(dev_id), clk_get_name(ctx->clk),
        *state == MOD_CLOCK_STATE_STOPPED ? "OFF" : "ON");

    return FWK_SUCCESS;
}

static int get_range(fwk_id_t dev_id, struct mod_clock_range *range)
{
    struct tfm_clock_dev_ctx *ctx = elt_id_to_ctx(dev_id);
    int res;
    size_t rate_count = 0;
    size_t count;
    unsigned long min, max;

    if ((ctx == NULL) || (range == NULL)) {
        return FWK_E_PARAM;
    }

    if (!is_exposed(ctx)) {
        range->rate_type = MOD_CLOCK_RATE_TYPE_DISCRETE;
        range->min = 0;
        range->max = 0;
        range->rate_count = 1;

        return FWK_SUCCESS;
    }

    res = clk_get_rates_array(ctx->clk, 0, NULL, &rate_count);
    if (res == -ENOENT) {
        unsigned long max = 0;
        unsigned long min = 0;
        unsigned long step = 0;

        res = clk_get_rates_steps(ctx->clk, &min, &max, &step);

        if (res == -ENOENT) {
            range->rate_type = MOD_CLOCK_RATE_TYPE_DISCRETE;
            range->min = clk_get_rate(ctx->clk);
            range->max = range->min;
            range->rate_count = 1;
            return FWK_SUCCESS;
        } else if (res) {
            return FWK_E_DEVICE;
        }

        range->rate_type = MOD_CLOCK_RATE_TYPE_CONTINUOUS;
        range->min = min;
        range->max = max;
        range->step = step;

        return FWK_SUCCESS;
    } else if (res) {
        return FWK_E_DEVICE;
    }

    range->rate_type = MOD_CLOCK_RATE_TYPE_DISCRETE;
    range->rate_count = rate_count;
    range->min = UINT64_MAX;
    range->max = 0;

    count = 1;
    res = clk_get_rates_array(ctx->clk, 0, &min, &count);
    fwk_assert(!res && count == 1);
    range->min = min;

    count = 1;
    res = clk_get_rates_array(ctx->clk, rate_count - 1, &max, &count);
    fwk_assert(!res && count == 1);
    range->max = max;

    return FWK_SUCCESS;
}

static int set_rate(fwk_id_t dev_id, uint64_t rate,
                    enum mod_clock_round_mode round_mode)
{
    struct tfm_clock_dev_ctx *ctx = elt_id_to_ctx(dev_id);
    int res;

    if (ctx == NULL) {
        return FWK_E_PARAM;
    }

    if (!is_exposed(ctx)) {
        return FWK_E_ACCESS;
    }

    res = clk_set_rate(ctx->clk, rate);
    if (res) {
        return FWK_E_SUPPORT;
    }

    FWK_LOG_DEBUG(
        MOD_NAME "(%u/%s) rate= %lu",
        fwk_id_get_element_idx(dev_id), clk_get_name(ctx->clk), rate);

    return FWK_SUCCESS;
}

static int get_rate_from_index(
    fwk_id_t dev_id,
    unsigned int rate_index,
    uint64_t *rate)
{
    struct tfm_clock_dev_ctx *ctx = elt_id_to_ctx(dev_id);
    unsigned long rate_ul;
    size_t rate_count;
    int res;

    if ((ctx == NULL) || (rate == NULL)) {
        return FWK_E_PARAM;
    }

    if (!is_exposed(ctx)) {
        *rate = 0;
        return FWK_SUCCESS;
    }

    res = clk_get_rates_array(ctx->clk, 0, NULL, &rate_count);
    if (res) {
        if (rate_index > 0) {
            return FWK_E_PARAM;
        }

        *rate = clk_get_rate(ctx->clk);
        return FWK_SUCCESS;
    }

    if (rate_index > rate_count) {
        return FWK_E_PARAM;
    }

    rate_count = 1;
    res = clk_get_rates_array(ctx->clk, rate_index, &rate_ul, &rate_count);
    fwk_assert(!res && rate_count == 1);

    *rate = rate_ul;

    FWK_LOG_DEBUG(
        MOD_NAME "(%u/%s) rate(index %u): %lu",
        fwk_id_get_element_idx(dev_id), clk_get_name(ctx->clk),
        rate_index, rate_ul);

    return FWK_SUCCESS;
}

static int get_duty_cycle(fwk_id_t dev_id, uint32_t *num, uint32_t *den)
{
    struct tfm_clock_dev_ctx *ctx = elt_id_to_ctx(dev_id);
    struct clk_duty duty = { 0 };
    int res;

    if (ctx == NULL) {
        return FWK_E_PARAM;
    }

    if (!is_exposed(ctx)) {
        /* Return a dummy value to prevent an error trace */
        *num = 1;
        *den = 2;
        return FWK_SUCCESS;
    }

    res = clk_get_duty_cycle(ctx->clk, &duty);

    if (res) {
        /* Assume a 50% duty cycle */
        duty = (struct clk_duty){ .num = 1, .den = 2 };
    }

    *num = duty.num;
    *den = duty.den;

    FWK_LOG_DEBUG(
        MOD_NAME "(%u/%s) duty_cycle: %d/%d (res:%d)",
        fwk_id_get_element_idx(dev_id), clk_get_name(ctx->clk),
        duty.num, duty.den, res);

    return FWK_SUCCESS;
}

static int stub_process_power_transition(fwk_id_t dev_id, unsigned int state)
{
    return FWK_E_SUPPORT;
}

static int stub_pending_power_transition(fwk_id_t dev_id,
                                         unsigned int current_state,
                                         unsigned int next_state)
{
    return FWK_E_SUPPORT;
}

static const struct mod_clock_drv_api api_tfm_clock = {
    .get_rate = get_rate,
    .set_state = set_state,
    .get_state = get_state,
    .get_range = get_range,
    .get_rate_from_index = get_rate_from_index,
    .set_rate = set_rate,
    .get_duty_cycle = get_duty_cycle,
    /* Not supported */
    .process_power_transition = stub_process_power_transition,
    .process_pending_power_transition = stub_pending_power_transition,
};

/*
 * Framework handler functions
 */

static int tfm_clock_init(fwk_id_t module_id, unsigned int count,
                            const void *data)
{
    if (count == 0) {
        return FWK_SUCCESS;
    }

    module_ctx.dev_count = count;
    module_ctx.dev_ctx = fwk_mm_calloc(count, sizeof(*module_ctx.dev_ctx));

    return FWK_SUCCESS;
}

static int tfm_clock_element_init(fwk_id_t element_id, unsigned int dev_count,
                                    const void *data)
{
    struct tfm_clock_dev_ctx *ctx = elt_id_to_ctx(element_id);
    const struct mod_tfm_clock_config *config = data;
    int res;

    if (ctx == NULL) {
        return FWK_E_PARAM;
    }

    ctx->clk = (struct clk *)config->clk;
    if (ctx->clk) {
        ctx->enabled = config->default_enabled;

        if (ctx->enabled) {
            res = clk_enable(ctx->clk);
            if (res) {
                return FWK_E_DEVICE;
            }
        }
    }

    return FWK_SUCCESS;
}

static int tfm_clock_process_bind_request(fwk_id_t requester_id, fwk_id_t id,
                                            fwk_id_t api_type, const void **api)
{
    *api = &api_tfm_clock;

    return FWK_SUCCESS;
}

const struct fwk_module module_tfm_clock = {
    .type = FWK_MODULE_TYPE_DRIVER,
    .api_count = 1,
    .event_count = 0,
    .init = tfm_clock_init,
    .element_init = tfm_clock_element_init,
    .process_bind_request = tfm_clock_process_bind_request,
};
