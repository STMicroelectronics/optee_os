/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *  Thermal Power estimator.
 *  This module only probes the temperature and calculates the power budget.
 */

#include <mod_pid_controller.h>
#include <mod_sensor.h>
#include <mod_thermal_power_estimator.h>

#include <interface_power_management.h>

#include <fwk_core.h>
#include <fwk_id.h>
#include <fwk_log.h>
#include <fwk_mm.h>
#include <fwk_module.h>
#include <fwk_status.h>

#include <stdint.h>
#include <stdlib.h>

#define MOD_NAME "[TPE]"

/* Device Context */
struct mod_thermal_power_estimator_dev_ctx {
    /* Thermal device configuration */
    struct mod_thermal_power_estimator_dev_config *config;

    /* Sensor data */
    struct mod_sensor_data sensor_data;

    /* Current temperature */
    uint32_t cur_temp;

    /* Sensor API */
    const struct mod_sensor_api *sensor_api;

    /* PID Controller API */
    const struct mod_pid_controller_api *pid_ctrl_api;
};

/* Module Context */
struct mod_thermal_power_estimator_ctx {
    /* Table of thermal domains */
    struct mod_thermal_power_estimator_dev_ctx *dev_ctx_table;

    /* Number of thermal domains */
    unsigned int dev_ctx_count;
};

struct mod_thermal_power_estimator_ctx mod_ctx;

/*
 * Helper functions.
 */

static inline struct mod_thermal_power_estimator_dev_ctx *get_dev_ctx(
    fwk_id_t dev_id)
{
    return &mod_ctx.dev_ctx_table[fwk_id_get_element_idx(dev_id)];
}

static int read_temperature(struct mod_thermal_power_estimator_dev_ctx *dev_ctx)
{
    int status;

    status = dev_ctx->sensor_api->get_data(
        dev_ctx->config->sensor_id, &dev_ctx->sensor_data);
    if (status == FWK_PENDING) {
        FWK_LOG_ERR(MOD_NAME "delayed sensor response is not supported");
        return FWK_E_SUPPORT;
    } else if (status != FWK_SUCCESS) {
        return status;
    }

    dev_ctx->cur_temp = (uint32_t)dev_ctx->sensor_data.value;

    return status;
}

static inline int thermal_allocatable_power_calculate(
    struct mod_thermal_power_estimator_dev_ctx *dev_ctx,
    int64_t pid_output,
    uint32_t *allocatable_power)
{
    int64_t output = pid_output + (int64_t)dev_ctx->config->tdp;

    if (output >= UINT32_MAX) {
        FWK_LOG_ERR(MOD_NAME "Allocatable power is out of range > UINT32_MAX");
        return FWK_E_DATA;
    }

    *allocatable_power = (uint32_t)FWK_MAX(output, 0);

    return FWK_SUCCESS;
}

/*
 * API functions.
 */

static int thermal_power_estimator_get_limit(fwk_id_t id, uint32_t *power_limit)
{
    struct mod_thermal_power_estimator_dev_ctx *dev_ctx;
    int64_t pid_output;
    int status;

    fwk_assert(power_limit != NULL);

    dev_ctx = get_dev_ctx(id);
    fwk_assert(dev_ctx != NULL);

    status = read_temperature(dev_ctx);
    if (status != FWK_SUCCESS) {
        return status;
    }

    status = dev_ctx->pid_ctrl_api->update(
        dev_ctx->config->pid_controller_id, dev_ctx->cur_temp, &pid_output);
    if (status != FWK_SUCCESS) {
        return status;
    }

    status =
        thermal_allocatable_power_calculate(dev_ctx, pid_output, power_limit);
    if (status != FWK_SUCCESS) {
        return status;
    }

    return FWK_SUCCESS;
}

static int thermal_power_estimator_set_limit(fwk_id_t id, uint32_t power_limit)
{
    FWK_LOG_INFO(MOD_NAME "set_limit() is not supported");

    return FWK_E_SUPPORT;
}

static struct interface_power_management_api thermal_power_estimator_api = {
    .get_power_limit = thermal_power_estimator_get_limit,
    .set_power_limit = thermal_power_estimator_set_limit,
};

/*
 * Framework handler functions.
 */

static int thermal_power_estimator_init(
    fwk_id_t module_id,
    unsigned int element_count,
    const void *data)
{
    mod_ctx.dev_ctx_table = fwk_mm_calloc(
        element_count, sizeof(struct mod_thermal_power_estimator_dev_ctx));
    mod_ctx.dev_ctx_count = element_count;

    return FWK_SUCCESS;
}

static int thermal_power_estimator_dev_init(
    fwk_id_t element_id,
    unsigned int sub_element_count,
    const void *data)
{
    struct mod_thermal_power_estimator_dev_ctx *dev_ctx;

    fwk_assert(data != NULL);

    dev_ctx = get_dev_ctx(element_id);
    dev_ctx->config = (struct mod_thermal_power_estimator_dev_config *)data;

    return FWK_SUCCESS;
}

static int thermal_power_estimator_bind(fwk_id_t id, unsigned int round)
{
    int status;
    struct mod_thermal_power_estimator_dev_ctx *dev_ctx;

    if (round > 0) {
        return FWK_SUCCESS;
    }

    if (fwk_id_is_type(id, FWK_ID_TYPE_MODULE)) {
        return FWK_SUCCESS;
    }

    dev_ctx = get_dev_ctx(id);
    fwk_assert(dev_ctx != NULL);

    /* Bind to sensor */
    status = fwk_module_bind(
        dev_ctx->config->sensor_id,
        dev_ctx->config->sensor_api_id,
        &dev_ctx->sensor_api);
    if (status != FWK_SUCCESS) {
        return FWK_E_PANIC;
    }

    /* Bind to PID Controller */
    status = fwk_module_bind(
        dev_ctx->config->pid_controller_id,
        dev_ctx->config->pid_controller_api_id,
        &dev_ctx->pid_ctrl_api);

    return status;
}

static int thermal_power_estimator_process_bind_request(
    fwk_id_t source_id,
    fwk_id_t target_id,
    fwk_id_t api_id,
    const void **api)
{
    enum mod_thermal_power_estimator_api_idx api_idx;
    int status;

    api_idx =
        (enum mod_thermal_power_estimator_api_idx)fwk_id_get_api_idx(api_id);
    switch (api_idx) {
    case MOD_THERMAL_POWER_ESTIMATOR_API_IDX_POWER_MANAGEMENT:
        *api = &thermal_power_estimator_api;
        status = FWK_SUCCESS;
        break;

    default:
        status = FWK_E_PARAM;
        break;
    }

    return status;
}

const struct fwk_module module_thermal_power_estimator = {
    .type = FWK_MODULE_TYPE_SERVICE,
    .api_count = (unsigned int)MOD_THERMAL_POWER_ESTIMATOR_API_IDX_COUNT,
    .event_count = (unsigned int)0,
    .init = thermal_power_estimator_init,
    .element_init = thermal_power_estimator_dev_init,
    .bind = thermal_power_estimator_bind,
    .process_bind_request = thermal_power_estimator_process_bind_request,
};
