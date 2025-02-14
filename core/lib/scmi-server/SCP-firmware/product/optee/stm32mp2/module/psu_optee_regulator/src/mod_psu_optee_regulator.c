/*
 * Copyright (c) 2017-2019, Arm Limited and Contributors. All rights reserved.
 * Copyright (c) 2022, STMicroelectronics
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stddef.h>

#include <fwk_log.h>
#include <fwk_macros.h>
#include <fwk_mm.h>
#include <fwk_module.h>
#include <mod_psu.h>
#include <mod_psu_optee_regulator.h>
#include <mod_scmi_std.h>

#include <arch_main.h>

#include <assert.h>
#include <config.h>
#include <drivers/regulator.h>
#include <io.h>
#include <trace.h>

#define DEBUG_MSG(...)	FMSG(__VA_ARGS__)

/* Module context */
struct psu_optee_regulator_ctx {
    struct regulator *regulator;
    struct {
        bool enabled;
        uint32_t voltage;
    } fake_state;
};

/* A single instance handles all voltage regulators abstracted by regulator.h */
static struct psu_optee_regulator_ctx *module_ctx;

/*
 * Driver functions for the PSU API
 */
static int psu_optee_regulator_set_enabled(fwk_id_t id, bool enabled)
{
    struct psu_optee_regulator_ctx *ctx =
        module_ctx + fwk_id_get_element_idx(id);
    TEE_Result res = TEE_ERROR_GENERIC;
    struct regulator *regulator = ctx->regulator;

    if (!regulator) {
        ctx->fake_state.enabled = enabled;

        return FWK_SUCCESS;
    }

    DEBUG_MSG("PSU set %s %s", regulator_name(regulator),
              enabled ? "ON" : "OFF");

    if (enabled) {
        res = regulator_enable(regulator);
    } else {
        res = regulator_disable(regulator);
    }

    return scmi_tee_result_to_fwk_status(res);
}

static int psu_optee_regulator_get_enabled(fwk_id_t id, bool *enabled)
{
    struct psu_optee_regulator_ctx *ctx =
        module_ctx + fwk_id_get_element_idx(id);
    struct regulator *regulator = ctx->regulator;

    if (enabled == NULL) {
        return FWK_E_PARAM;
    }

    if (!regulator) {
        *enabled = ctx->fake_state.enabled;

        return FWK_SUCCESS;
    }

    *enabled = regulator_is_enabled(regulator);

    DEBUG_MSG("PSU get %s state: %s", regulator_name(regulator),
              *enabled ? "ON" : "OFF");

    return FWK_SUCCESS;
}

static int psu_optee_regulator_set_voltage(fwk_id_t id, uint32_t voltage)
{
    struct psu_optee_regulator_ctx *ctx =
        module_ctx + fwk_id_get_element_idx(id);
    TEE_Result res = TEE_ERROR_GENERIC;
    struct regulator *regulator = ctx->regulator;

    if (!regulator) {
        ctx->fake_state.voltage = voltage;

        return FWK_SUCCESS;
    }

    DEBUG_MSG("PSU set regulator %s level: %u mV",
              regulator_name(regulator), voltage);

    res = regulator_set_voltage(regulator, (int)voltage * 1000);

    return scmi_tee_result_to_fwk_status(res);
}

static int psu_optee_regulator_get_voltage(fwk_id_t id, uint32_t *voltage)
{
    struct psu_optee_regulator_ctx *ctx =
        module_ctx + fwk_id_get_element_idx(id);
    struct regulator *regulator = ctx->regulator;
    int level_mv;

    if (voltage == NULL) {
        return FWK_E_PARAM;
    }

    if (!regulator) {
        *voltage = ctx->fake_state.voltage;

        return FWK_SUCCESS;
    }

    level_mv = regulator_get_voltage(regulator) / 1000;

    DEBUG_MSG("PSU get regulator %s level: %d mV",
              regulator_name(regulator), level_mv);

    *voltage = (uint32_t)level_mv;

    return FWK_SUCCESS;
}

static struct mod_psu_driver_api psu_driver_api = {
    .set_enabled = psu_optee_regulator_set_enabled,
    .get_enabled = psu_optee_regulator_get_enabled,
    .set_voltage = psu_optee_regulator_set_voltage,
    .get_voltage = psu_optee_regulator_get_voltage,
};

static int psu_optee_regulator_init(fwk_id_t module_id,
                                    unsigned int element_count,
                                    const void *data)
{
    module_ctx = fwk_mm_calloc(element_count, sizeof(*module_ctx));

    return FWK_SUCCESS;
}

static int psu_optee_regulator_element_init(fwk_id_t element_id,
                                            unsigned int sub_element_count,
                                            const void *data)
{
    struct psu_optee_regulator_ctx *ctx =
        module_ctx + fwk_id_get_element_idx(element_id);
    const struct mod_psu_optee_regulator_dev_config *config = data;

    fwk_assert(config != NULL);

    ctx->regulator = config->regulator;

    return FWK_SUCCESS;
}

static int psu_optee_regulator_process_bind_request(fwk_id_t source_id,
                                                    fwk_id_t target_id,
                                                    fwk_id_t api_id,
                                                    const void **api)
{
    *api = &psu_driver_api;

    return FWK_SUCCESS;
}

const struct fwk_module module_psu_optee_regulator = {
    .api_count = 1,
    .type = FWK_MODULE_TYPE_DRIVER,
    .init = psu_optee_regulator_init,
    .element_init = psu_optee_regulator_element_init,
    .process_bind_request = psu_optee_regulator_process_bind_request,
};
