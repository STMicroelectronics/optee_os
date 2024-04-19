/*
 * Copyright (c) 2024, STMicroelectronics and the Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <clk.h>
#include <regulator.h>
#include <firewall.h>

#include <mod_power_domain.h>
#include <mod_stm32_pd.h>

#include <fwk_id.h>
#include <fwk_log.h>
#include <fwk_mm.h>
#include <fwk_module.h>

/*
 * For now, we consider only 1 power domain exposed, that is
 * the GPU power domain (aside the system power domain, required
 * by SCP-firmware implementation). Therefore, GPU PD module element
 * index is expected to be 0.
 */
 #define PD_SCMI_GPU        0

struct mod_stm32_pd {
    const struct mod_stm32_pd_config *config;
    unsigned int current_state;
    fwk_id_t requester_id;
    struct mod_pd_driver_input_api *pd_driver_input_api;
};

static struct mod_stm32_pd *mod_stm32_pd_ctx;

static int stm32_pd_get_state(fwk_id_t pd_id, unsigned int *state)
{
    struct mod_stm32_pd *ctx = mod_stm32_pd_ctx + fwk_id_get_element_idx(pd_id);

    *state = ctx->current_state;
    return FWK_SUCCESS;
}

static int stm32_pd_set_state(fwk_id_t pd_id, unsigned int state)
{
    struct mod_stm32_pd *ctx = mod_stm32_pd_ctx + fwk_id_get_element_idx(pd_id);

    ctx->current_state = state;
    return FWK_SUCCESS;
}

static int stm32_pd_reset(fwk_id_t pd_id)
{
    FWK_LOG_DEBUG("Power domain %s reset not supported",
                  fwk_module_get_element_name(pd_id));
    return FWK_E_SUPPORT;
}

static int stm32_pd_shutdown(fwk_id_t dev_id,
                             enum mod_pd_system_shutdown system_shutdown)
{
    /*
     * Since request come from scmi system power, the agent m33 ns is in charge
     * to handle the request
     */
    FWK_LOG_DEBUG("[STM32_PD] %s @%d", __func__, __LINE__);

    return FWK_PENDING;
}

/*
 * stm32_pd_set_state_gpu
 *
 * A firewall configuration may be defined to fix Errata Section 2 3 5
 * cf: https://www.st.com/resource/en/errata_sheet/es0598-stm32mp251x3x5x7x-device-errata-stmicroelectronics.pdf
 * Compartment filtering of PWR_CR11 and PWR_CR12 registers is not functional
 *
 * Description:
 * Once the compartment filtering is enabled on PWR_CR11 (RCC internal
 * resource 104) and PWR_CR12 (RIFSC peripheral #79), the registers are
 * not writeable by any CIDs even the permitted one.
 *
 * Workaround:
 * CID filtering should be disabled before accessing PWR_CR11 and PWR_CR12 registers.
 *
 * - Set errata configuration (defined in DT) to disable filtering.
 * - Access to clock and regulator
 * - Release this configuration to return at initial runtime configuration.
 */
static int stm32_pd_set_state_gpu(fwk_id_t pd_id, unsigned int state)
{
    struct mod_stm32_pd *ctx = mod_stm32_pd_ctx + fwk_id_get_element_idx(pd_id);
    const struct mod_stm32_pd_config *cfg = ctx->config;
    const struct device *regu = cfg->regu;
    struct firewall_spec *firewall;
    struct clk *clk = cfg->clk;
    int status = FWK_E_DEVICE;
    int i;
    int err;

    fwk_assert(ctx->pd_driver_input_api != NULL);
    fwk_assert(fwk_id_get_element_idx(pd_id) == PD_SCMI_GPU);

    /* Nothing to do */
    if (state == ctx->current_state) {
        return FWK_SUCCESS;
    }

    err = set_for_each_firewall(cfg->firewall, firewall, cfg->n_firewall, i);
    if (err) {
        FWK_LOG_ERR("%s: set firewall[%d] fail, err:%d", __func__, i, err);
        status = FWK_E_DEVICE;
        goto out;
    }

    switch (state) {
        case MOD_PD_STATE_ON:
            if (regu != NULL && regulator_enable(regu) != 0) {
                status = FWK_E_DEVICE;
                break;
            }

            if (clk != NULL && clk_enable(clk) != 0) {
                if (regu != NULL)
                    regulator_disable(regu);

                status = FWK_E_DEVICE;
                break;
            }
            status = FWK_SUCCESS;
            break;
        case MOD_PD_STATE_OFF:
            if (clk != NULL) {
                clk_disable(clk);
            }
            if (regu != NULL && regulator_disable(regu) != 0) {
                if (clk != NULL) {
                    clk_enable(clk);
                }

                status = FWK_E_DEVICE;
                break;
            }
            status = FWK_SUCCESS;
            break;
        default:
            FWK_LOG_ERR("State %d not supported on power domain %s",
                        state,
                        fwk_module_get_element_name(pd_id));
    }

out:
    err = release_for_each_firewall(cfg->firewall, firewall, cfg->n_firewall, i);
    if (err) {
        FWK_LOG_ERR("%s: release firewall[%d] fail, err:%d", __func__, i, err);
        status = FWK_E_DEVICE;
    }

    if (status != FWK_SUCCESS)
        return status;

    status = ctx->pd_driver_input_api->report_power_state_transition(
        ctx->requester_id,
        state);

    if (status != FWK_SUCCESS) {
        fwk_unexpected();
        return status;
    }

    FWK_LOG_DEBUG("Power domain %s change state to %s",
                  fwk_module_get_element_name(pd_id),
                  state == MOD_PD_STATE_ON ? "ON" : "OFF");

    ctx->current_state = state;
    return FWK_SUCCESS;
}

static int stm32_pd_bind(fwk_id_t id, unsigned int round)
{
    struct mod_stm32_pd *ctx;

    /* Nothing to do for the module */
    if (fwk_module_is_valid_module_id(id)) {
        return FWK_SUCCESS;
    }

    /* Element id is not valid */
    if (!fwk_module_is_valid_element_id(id)) {
        FWK_LOG_ERR("Cannot bind %s, element id is needed",
                    fwk_module_get_element_name(id));
        return FWK_E_SUPPORT;
    }

    ctx = mod_stm32_pd_ctx + fwk_id_get_element_idx(id);

    /* stm32_pd_process_bind_request probably isn't been called right now */
    if (!fwk_module_is_valid_element_id(ctx->requester_id)) {
        FWK_LOG_ERR("stm32_pd_process_bind_request hasn't been called yet");
        return FWK_E_ACCESS;
    }

    /* Only get an input API from power domain framework */
    if (fwk_id_get_module_idx(ctx->requester_id) !=
            FWK_MODULE_IDX_POWER_DOMAIN) {
        FWK_LOG_ERR("Requester is not power domain module");
        return FWK_E_SUPPORT;
    }

    return fwk_module_bind(ctx->requester_id,
                           mod_pd_api_id_driver_input,
                           &ctx->pd_driver_input_api);
}

static int stm32_pd_element_init(fwk_id_t element_id,
                                 unsigned int unused,
                                 const void *data)
{
    struct mod_stm32_pd *ctx =
                        mod_stm32_pd_ctx + fwk_id_get_element_idx(element_id);

    ctx->config = (const struct mod_stm32_pd_config *)data;

    return FWK_SUCCESS;
}

static const struct mod_pd_driver_api stm32_pd_api_gpu = {
    .set_state = stm32_pd_set_state_gpu,
    .get_state = stm32_pd_get_state,
    .reset = stm32_pd_reset,
    .shutdown = stm32_pd_shutdown,
};

static const struct mod_pd_driver_api stm32_pd_api = {
    .set_state = stm32_pd_set_state,
    .get_state = stm32_pd_get_state,
    .reset = stm32_pd_reset,
    .shutdown = stm32_pd_shutdown,
};

static int stm32_pd_process_bind_request(fwk_id_t requester_id,
                                         fwk_id_t target_id,
                                         fwk_id_t api_type,
                                         const void **api)
{
    struct mod_stm32_pd *ctx = NULL;

    ctx = mod_stm32_pd_ctx + fwk_id_get_element_idx(target_id);
    ctx->requester_id = requester_id;

    switch (fwk_id_get_api_idx(api_type)) {
        case MOD_STM32_PD_API_IDX_BASIC:
            *api = &stm32_pd_api;
            ctx->current_state = MOD_PD_STATE_ON;
            break;
        case MOD_STM32_PD_API_IDX_GPU:
            *api = &stm32_pd_api_gpu;
            ctx->current_state = MOD_PD_STATE_OFF;
            break;
        default:
            return FWK_E_PARAM;
    }

    return FWK_SUCCESS;
}

static int stm32_pd_init(fwk_id_t module_id,
    unsigned int element_count, const void *data)
{
    mod_stm32_pd_ctx = fwk_mm_calloc(element_count, sizeof(*mod_stm32_pd_ctx));

    return FWK_SUCCESS;
}

const struct fwk_module module_stm32_pd = {
    .type = FWK_MODULE_TYPE_DRIVER,
    .api_count = MOD_STM32_PD_API_IDX_COUNT,
    .init = stm32_pd_init,
    .element_init = stm32_pd_element_init,
    .bind = stm32_pd_bind,
    .process_bind_request = stm32_pd_process_bind_request,
};
