/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, STMicroelectronics and the Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Interface SCP-firmware clock module with OP-TEE STM32 OPP resources.
 */

#include <fwk_macros.h>
#include <fwk_mm.h>
#include <fwk_module.h>
#include <fwk_log.h>

#include <arch_main.h>

#include <mod_clock.h>
#include <mod_stm32_opp.h>

#include <drivers/stm32_cpu_opp.h>

#include <compiler.h>
#include <tee_api_types.h>

#include <stdbool.h>

#define MOD_NAME "[STM32 OPP] "

/* OP-TEE clock device context */
struct stm32_opp_dev_ctx {
    unsigned int opp_id;
};

/* OP-TEE clock module context */
struct stm32_opp_module_ctx {
    struct stm32_opp_dev_ctx *dev_ctx;
    unsigned int dev_count;
};

static struct stm32_opp_module_ctx module_ctx;

static struct stm32_opp_dev_ctx *elt_id_to_ctx(fwk_id_t dev_id)
{
    if (!fwk_module_is_valid_element_id(dev_id)) {
        return NULL;
    }

    return module_ctx.dev_ctx + fwk_id_get_element_idx(dev_id);
}

static bool is_exposed(struct stm32_opp_dev_ctx *ctx)
{
    return ctx->opp_id != OPP_ID_INVALID;
}

/*
 * Clock driver API functions
 */
static int get_rate(fwk_id_t dev_id, uint64_t *rate)
{
    struct stm32_opp_dev_ctx *ctx = elt_id_to_ctx(dev_id);

    if ((ctx == NULL) || (rate == NULL)) {
        return FWK_E_PARAM;
    }

    *rate = 0;

    if (!is_exposed(ctx)) {
        return FWK_SUCCESS;
    }

    *rate = stm32_opp_get_rate(ctx->opp_id);

    return FWK_SUCCESS;
}

static int set_state(fwk_id_t dev_id, enum mod_clock_state state)
{
    struct stm32_opp_dev_ctx *ctx = elt_id_to_ctx(dev_id);

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

    if (state == MOD_CLOCK_STATE_RUNNING) {
        return FWK_SUCCESS;
    } else {
        return FWK_E_ACCESS;
    }
}

static int get_state(fwk_id_t dev_id, enum mod_clock_state *state)
{
    struct stm32_opp_dev_ctx *ctx = elt_id_to_ctx(dev_id);

    if ((ctx == NULL) || (state == NULL)) {
        return FWK_E_PARAM;
    }

    if (!is_exposed(ctx)) {
        *state = MOD_CLOCK_STATE_STOPPED;
        return FWK_SUCCESS;
    }

    *state = MOD_CLOCK_STATE_RUNNING;

    return FWK_SUCCESS;
}

static int get_range(fwk_id_t dev_id, struct mod_clock_range *range)
{
    struct stm32_opp_dev_ctx *ctx = elt_id_to_ctx(dev_id);
    TEE_Result res;
    unsigned int min, max, opp_nb;

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

    opp_nb = stm32_opp_get_count(ctx->opp_id);
    if (!opp_nb) {
        return FWK_E_ACCESS;
    }

    res = stm32_opp_get_rate_for_level(ctx->opp_id, 0, &min);
    if (res)
        return scmi_tee_result_to_fwk_status(res);

    res = stm32_opp_get_rate_for_level(ctx->opp_id, opp_nb - 1, &max);
    if (res)
        return scmi_tee_result_to_fwk_status(res);

    range->rate_type = MOD_CLOCK_RATE_TYPE_DISCRETE;
    range->min = min;
    range->max = max;
    range->rate_count = opp_nb;

    return FWK_SUCCESS;
}

static int set_rate(fwk_id_t dev_id, uint64_t rate,
                    enum mod_clock_round_mode round_mode)
{
    struct stm32_opp_dev_ctx *ctx = elt_id_to_ctx(dev_id);
    TEE_Result res;

    if (ctx == NULL || round_mode != MOD_CLOCK_ROUND_MODE_NONE) {
        return FWK_E_PARAM;
    }

    if (!is_exposed(ctx)) {
        return FWK_E_ACCESS;
    }

    res = stm32_opp_set_rate(ctx->opp_id, rate);

    return scmi_tee_result_to_fwk_status(res);
}

static int get_rate_from_index(fwk_id_t dev_id,
                               unsigned int rate_index, uint64_t *rate)
{
    struct stm32_opp_dev_ctx *ctx = elt_id_to_ctx(dev_id);
    unsigned int opp_rate;
    TEE_Result res;

    if ((ctx == NULL) || (rate == NULL)) {
        return FWK_E_PARAM;
    }

    if (!is_exposed(ctx)) {
        *rate = 0;
        return FWK_SUCCESS;
    }

    res = stm32_opp_get_rate_for_level(ctx->opp_id, rate_index, &opp_rate);
    if (res)
        return scmi_tee_result_to_fwk_status(res);

    *rate = opp_rate;

    return FWK_SUCCESS;
}

static const struct mod_clock_drv_api api_stm32_opp = {
    .get_rate = get_rate,
    .set_state = set_state,
    .get_state = get_state,
    .get_range = get_range,
    .get_rate_from_index = get_rate_from_index,
    .set_rate = set_rate,
};

/*
 * Framework handler functions
 */

static int stm32_opp_init(fwk_id_t module_id, unsigned int count,
                            const void *data)
{
    if (count == 0) {
        return FWK_SUCCESS;
    }

    module_ctx.dev_count = count;
    module_ctx.dev_ctx = fwk_mm_calloc(count, sizeof(*module_ctx.dev_ctx));

    return FWK_SUCCESS;
}

static int stm32_opp_element_init(fwk_id_t element_id, unsigned int dev_count,
                                    const void *data)
{
    struct stm32_opp_dev_ctx *ctx = elt_id_to_ctx(element_id);
    const struct mod_stm32_opp_config *config = data;

    ctx->opp_id = config->opp_id;

    return FWK_SUCCESS;
}

static int stm32_opp_process_bind_request(fwk_id_t requester_id, fwk_id_t id,
                                            fwk_id_t api_type, const void **api)
{
    *api = &api_stm32_opp;

    return FWK_SUCCESS;
}

const struct fwk_module module_stm32_opp = {
    .type = FWK_MODULE_TYPE_DRIVER,
    .api_count = 1,
    .event_count = 0,
    .init = stm32_opp_init,
    .element_init = stm32_opp_element_init,
    .process_bind_request = stm32_opp_process_bind_request,
};
