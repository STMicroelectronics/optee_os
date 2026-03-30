/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <mod_clock.h>
#include <mod_power_domain.h>
#include <mod_ros_clock.h>

#include <fwk_element.h>
#include <fwk_id.h>
#include <fwk_log.h>
#include <fwk_macros.h>
#include <fwk_mm.h>
#include <fwk_module.h>

#include <stddef.h>
#include <stdint.h>

/*
 * Offsets for clock control register.
 */
#define CLK_CONTROL_CLKSELECT_POS 0U
#define CLK_CONTROL_CLKDIV0_POS   4U
#define CLK_CONTROL_CLKDIV1_POS   12U

/*
 * Masks for register.
 */
#define CLK_CONTROL_CLKSELECT_MSK (UINT32_C(0x3) << CLK_CONTROL_CLKSELECT_POS)
#define CLK_CONTROL_CLKDIV0_MSK   (UINT32_C(0xFF) << CLK_CONTROL_CLKDIV0_POS)
#define CLK_CONTROL_CLKDIV1_MSK   (UINT32_C(0xFF) << CLK_CONTROL_CLKDIV1_POS)

/*
 * Clock divider bitfield width
 */
#define CLK_CONTROL_CLKDIV_BITFIELD_WIDTH 8U

/* Device context */
struct ros_clock_dev_ctx {
    uint64_t current_rate;
    uint8_t current_source;
    const struct mod_ros_clock_dev_config *config;
};

/* Module context */
struct ros_clock_ctx {
    struct ros_clock_dev_ctx *dev_ctx_table;
    unsigned int dev_count;
    uint32_t divider_max;
};

static struct ros_clock_ctx module_ctx;

/*
 * Static helper functions
 */
static const struct mod_ros_clock_rate *find_rate_entry(
    uint64_t target_rate,
    const struct mod_ros_clock_rate *const rate_table,
    unsigned int rate_count)
{
    for (unsigned int i = 0; i < rate_count; i++) {
        if (rate_table[i].rate == target_rate) {
            return &rate_table[i];
        }
    }

    return NULL;
}

static int get_rate_entry(
    struct ros_clock_dev_ctx *ctx,
    uint64_t target_rate,
    const struct mod_ros_clock_rate **entry)
{
    const struct mod_ros_clock_rate *current_rate_entry;

    if (ctx == NULL)
        return FWK_E_PARAM;
    if (entry == NULL)
        return FWK_E_PARAM;

    /* Perform a linear search to find the entry matching the requested rate */
    current_rate_entry = find_rate_entry(
        target_rate, ctx->config->rate_table, ctx->config->rate_count);

    if (current_rate_entry == NULL)
        return FWK_E_PARAM;

    *entry = current_rate_entry;

    return FWK_SUCCESS;
}

static int clock_set_div(
    struct ros_clock_dev_ctx *ctx,
    enum mod_ros_clock_clock_divider divider_type,
    uint32_t divider)
{
    uint32_t clkdiv, divider_msk, divider_pos;

    /* In case of REFCLK, there is no divider */
    if (divider_type == MOD_ROS_CLOCK_CLOCK_NO_DIVIDER)
        return FWK_SUCCESS;

    if (ctx == NULL)
        return FWK_E_PARAM;
    if (divider == 0)
        return FWK_E_PARAM;
    if (divider > module_ctx.divider_max)
        return FWK_E_PARAM;

    /* The resulting divider is the programmed value plus one */
    clkdiv = divider - 1;

    if (divider_type == MOD_ROS_CLOCK_CLOCK_DIVIDER_0) {
        divider_msk = CLK_CONTROL_CLKDIV0_MSK;
        divider_pos = CLK_CONTROL_CLKDIV0_POS;
    } else if (divider_type == MOD_ROS_CLOCK_CLOCK_DIVIDER_1) {
        divider_msk = CLK_CONTROL_CLKDIV1_MSK;
        divider_pos = CLK_CONTROL_CLKDIV1_POS;
    } else {
        return FWK_E_PARAM;
    }

    /* Set */
    *ctx->config->control_reg =
        (*ctx->config->control_reg & ~divider_msk) | (clkdiv << divider_pos);

    return FWK_SUCCESS;
}

static int clock_set_source(struct ros_clock_dev_ctx *ctx, uint8_t source)
{
    if (ctx == NULL)
        return FWK_E_PARAM;
    if (source > (CLK_CONTROL_CLKSELECT_MSK >> CLK_CONTROL_CLKSELECT_POS))
        return FWK_E_PARAM;

    /* Set */
    *ctx->config->control_reg =
        (*ctx->config->control_reg & ~CLK_CONTROL_CLKSELECT_MSK) |
        (source << CLK_CONTROL_CLKSELECT_POS);

    return FWK_SUCCESS;
}

static int do_ros_clock_set_rate(
    fwk_id_t dev_id,
    uint64_t rate,
    enum mod_clock_round_mode round_mode)
{
    int status;
    struct ros_clock_dev_ctx *ctx;
    const struct mod_ros_clock_rate *rate_entry;
    uint32_t element_idx = fwk_id_get_element_idx(dev_id);

    if (element_idx >= module_ctx.dev_count)
        return FWK_E_PARAM;

    ctx = module_ctx.dev_ctx_table + element_idx;

    /* Look up the divider and source settings */
    status = get_rate_entry(ctx, rate, &rate_entry);
    if (status != FWK_SUCCESS)
        return status;

    status = clock_set_div(ctx, rate_entry->source, rate_entry->divider);
    if (status != FWK_SUCCESS)
        return status;

    status = clock_set_source(ctx, rate_entry->source);
    if (status != FWK_SUCCESS)
        return status;

    ctx->current_source = rate_entry->source;
    ctx->current_rate = rate;

    return status;
}

/*
 * Clock driver API functions
 */

static int ros_clock_set_rate(
    fwk_id_t dev_id,
    uint64_t rate,
    enum mod_clock_round_mode round_mode)
{
    return do_ros_clock_set_rate(dev_id, rate, round_mode);
}

static int ros_clock_get_rate(fwk_id_t dev_id, uint64_t *rate)
{
    struct ros_clock_dev_ctx *ctx;
    uint32_t element_idx = fwk_id_get_element_idx(dev_id);

    if (rate == NULL)
        return FWK_E_PARAM;
    if (element_idx >= module_ctx.dev_count)
        return FWK_E_PARAM;

    ctx = module_ctx.dev_ctx_table + element_idx;

    *rate = ctx->current_rate;

    return FWK_SUCCESS;
}

static int ros_clock_get_rate_from_index(
    fwk_id_t dev_id,
    unsigned int rate_index,
    uint64_t *rate)
{
    struct ros_clock_dev_ctx *ctx;
    uint32_t element_idx = fwk_id_get_element_idx(dev_id);

    if (rate == NULL)
        return FWK_E_PARAM;
    if (element_idx >= module_ctx.dev_count)
        return FWK_E_PARAM;

    ctx = module_ctx.dev_ctx_table + element_idx;

    if (rate_index >= ctx->config->rate_count)
        return FWK_E_PARAM;

    *rate = ctx->config->rate_table[rate_index].rate;
    return FWK_SUCCESS;
}

static int ros_clock_set_state(
    fwk_id_t dev_id,
    enum mod_clock_state target_state)
{
    if (target_state == MOD_CLOCK_STATE_STOPPED)
        /* RoS clock can not be stopped/gated */
        return FWK_E_PARAM;

    return FWK_SUCCESS;
}

static int ros_clock_get_range(fwk_id_t dev_id, struct mod_clock_range *range)
{
    struct ros_clock_dev_ctx *ctx;
    uint32_t element_idx = fwk_id_get_element_idx(dev_id);

    if (range == NULL)
        return FWK_E_PARAM;
    if (element_idx >= module_ctx.dev_count)
        return FWK_E_PARAM;

    ctx = module_ctx.dev_ctx_table + element_idx;

    range->rate_type = MOD_CLOCK_RATE_TYPE_DISCRETE;

    /* We can do the following as the rate table is in increasing order */
    range->min = ctx->config->rate_table[0].rate;
    range->max = ctx->config->rate_table[ctx->config->rate_count - 1].rate;

    range->rate_count = ctx->config->rate_count;

    return FWK_SUCCESS;
}

static int ros_clock_get_state(fwk_id_t dev_id, enum mod_clock_state *state)
{
    if (state == NULL)
        return FWK_E_PARAM;

    /* RoS clock is always running by default */
    *state = MOD_CLOCK_STATE_RUNNING;

    return FWK_SUCCESS;
}

static int ros_clock_power_state_change(fwk_id_t dev_id, unsigned int state)
{
    /* RoS clock can not be stopped/gated */
    if (state != (unsigned int)MOD_PD_STATE_ON) {
        return FWK_E_PARAM;
    }

    return FWK_SUCCESS;
}

static const struct mod_clock_drv_api api_clock = {
    .set_rate = ros_clock_set_rate,
    .get_rate = ros_clock_get_rate,
    .get_rate_from_index = ros_clock_get_rate_from_index,
    .set_state = ros_clock_set_state,
    .get_state = ros_clock_get_state,
    .get_range = ros_clock_get_range,
    .process_power_transition = ros_clock_power_state_change,
};

/*
 * Framework handler functions
 */

static int ros_clock_init(
    fwk_id_t module_id,
    unsigned int element_count,
    const void *data)
{
    struct mod_ros_clock_module_config *config =
        (struct mod_ros_clock_module_config *)data;

    module_ctx.dev_count = element_count;

    if (element_count == 0)
        return FWK_SUCCESS;

    module_ctx.divider_max =
        ((config == NULL) ? (1UL << CLK_CONTROL_CLKDIV_BITFIELD_WIDTH) :
                            FWK_MIN(
                                1UL << CLK_CONTROL_CLKDIV_BITFIELD_WIDTH,
                                config->divider_max));

    module_ctx.dev_ctx_table =
        fwk_mm_calloc(element_count, sizeof(struct ros_clock_dev_ctx));

    return FWK_SUCCESS;
}

static int ros_clock_element_init(
    fwk_id_t element_id,
    unsigned int sub_element_count,
    const void *data)
{
    uint64_t last_rate = 0;
    struct ros_clock_dev_ctx *ctx;
    const struct mod_ros_clock_dev_config *dev_config;
    uint32_t element_idx = fwk_id_get_element_idx(element_id);

    if (data == NULL)
        return FWK_E_PARAM;
    if (element_idx >= module_ctx.dev_count)
        return FWK_E_PARAM;

    dev_config = data;

    /*
     * Verify that the rate entries in the device's lookup table are ordered.
     * This is done to simplify the implementation of ros_clock_get_range.
     */
    for (uint32_t i = 0; i < dev_config->rate_count; i++) {
        /* The rate entries must be in ascending order */
        if (dev_config->rate_table[i].rate < last_rate)
            return FWK_E_DATA;

        last_rate = dev_config->rate_table[i].rate;
    }

    ctx = module_ctx.dev_ctx_table + element_idx;

    ctx->config = dev_config;

    /* Begin with an invalid source */
    ctx->current_source = 0xFF;

    return do_ros_clock_set_rate(
        element_id, dev_config->initial_rate, MOD_CLOCK_ROUND_MODE_NONE);
}

static int ros_clock_process_bind_request(
    fwk_id_t source_id,
    fwk_id_t target_id,
    fwk_id_t api_id,
    const void **api)
{
    /* Only elements can be bound to as the API depends on the element type */
    if (!fwk_id_is_type(target_id, FWK_ID_TYPE_ELEMENT))
        return FWK_E_ACCESS;

    *api = &api_clock;

    return FWK_SUCCESS;
}

const struct fwk_module module_ros_clock = {
    .type = FWK_MODULE_TYPE_DRIVER,
    .api_count = MOD_ROS_CLOCK_API_COUNT,
    .event_count = 0,
    .init = ros_clock_init,
    .element_init = ros_clock_element_init,
    .process_bind_request = ros_clock_process_bind_request,
};
