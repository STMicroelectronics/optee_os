/*
 * Copyright (c) 2017-2019, Arm Limited and Contributors. All rights reserved.
 * Copyright (c) 2024, STMicroelectronics
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stddef.h>
#include <regulator.h>
#include <fwk_log.h>
#include <fwk_macros.h>
#include <fwk_mm.h>
#include <fwk_module.h>
#include <mod_scmi_std.h>
#include <mod_tfm_regu_consumer.h>
#include <mod_voltage_domain.h>

#include <inttypes.h>
#include <stdio.h>

#define MOD_NAME "[TFM SCMI VOLTD] "

/* Device context */
struct tfm_regu_consumer_dev_ctx {
    struct device *dev;
    bool enabled;
};

/* Module context */
struct tfm_regu_consumer_ctx {
    struct tfm_regu_consumer_dev_ctx *dev_ctx_table;
    unsigned int dev_count;
};

/* A single instance handles all voltage regulators abstracted by regulator.h */
static struct tfm_regu_consumer_ctx module_ctx;

static char *regu_name(struct device *dev)
{
    if (dev)
        return (char *)dev->name;

    return NULL;
}

static int find_ctx(fwk_id_t dev_id,
                    struct tfm_regu_consumer_dev_ctx **out_ctx)
{
    struct tfm_regu_consumer_dev_ctx *ctx;

    ctx = module_ctx.dev_ctx_table + fwk_id_get_element_idx(dev_id);

    if (!fwk_module_is_valid_element_id(dev_id))
        return FWK_E_PARAM;

    *out_ctx = ctx;

    return FWK_SUCCESS;
}

static int tfm_regu_consumer_get_config(fwk_id_t dev_id, uint8_t *mode_type,
                                          uint8_t *mode_id)
{
    struct tfm_regu_consumer_dev_ctx *ctx;
    int ret;

    ret = find_ctx(dev_id, &ctx);
    if (ret)
        return ret;

    *mode_type = MOD_VOLTD_MODE_TYPE_ARCH;

    if (ctx->dev == NULL) {
        *mode_id = MOD_VOLTD_MODE_ID_OFF;

        return FWK_SUCCESS;
    }

    if (ctx->enabled)
        *mode_id = MOD_VOLTD_MODE_ID_ON;
    else
        *mode_id = MOD_VOLTD_MODE_ID_OFF;

    FWK_LOG_INFO("SCMI voltd %u: get config PMIC %s = %#"PRIx8", %#"PRIx8,
              fwk_id_get_element_idx(dev_id), regu_name(ctx->dev), *mode_type,
              *mode_id);

    return FWK_SUCCESS;
}

static int tfm_regu_consumer_set_config(fwk_id_t dev_id, uint8_t mode_type,
                                          uint8_t mode_id)
{
    struct tfm_regu_consumer_dev_ctx *ctx;
    int ret;

    ret = find_ctx(dev_id, &ctx);
    if (ret)
        return ret;

    if (mode_type != MOD_VOLTD_MODE_TYPE_ARCH) {
        return FWK_E_PARAM;
    }

    switch (mode_id) {
    case MOD_VOLTD_MODE_ID_ON:
        if (!ctx->enabled) {
            if (regulator_enable(ctx->dev))
                return FWK_E_DEVICE;

            ctx->enabled = true;
        }
        break;

    case MOD_VOLTD_MODE_ID_OFF:
        if (ctx->enabled) {
            if (regulator_disable(ctx->dev))
                return FWK_E_DEVICE;

            ctx->enabled = false;
        }
        break;

    default:
        return FWK_E_PARAM;
    }

    FWK_LOG_INFO("SCMI voltd %u: set config PMIC %s to type %#"PRIx8" / mode %#"PRIx8,
              fwk_id_get_element_idx(dev_id), regu_name(ctx->dev), mode_type,
              mode_id);

    return FWK_SUCCESS;
}

static int tfm_regu_consumer_get_level(fwk_id_t dev_id, int32_t *level_uv)
{
    struct tfm_regu_consumer_dev_ctx *ctx;
    int ret;

    ret = find_ctx(dev_id, &ctx);
    if (ret)
        return ret;

    if (ctx->dev == NULL) {
        *level_uv = 0;

        return FWK_SUCCESS;
    }

    if (regulator_get_voltage(ctx->dev, level_uv))
        return FWK_E_PANIC;

    FWK_LOG_INFO("SCMI voltd %u: get level PMIC %s = %ld",
              fwk_id_get_element_idx(dev_id), regu_name(ctx->dev), *level_uv);

    return FWK_SUCCESS;
}

static int tfm_regu_consumer_set_level(fwk_id_t dev_id, int32_t level_uv)
{
    struct tfm_regu_consumer_dev_ctx *ctx = NULL;
    int ret = FWK_E_PANIC;

    ret = find_ctx(dev_id, &ctx);
    if (ret)
        return ret;

    if (ctx->dev == NULL)
        return FWK_E_ACCESS;

    if (regulator_set_voltage(ctx->dev, level_uv, level_uv))
        return FWK_E_DEVICE;

    FWK_LOG_INFO("SCMI voltd %u: set level PMIC %s to %ld",
              fwk_id_get_element_idx(dev_id), regu_name(ctx->dev), level_uv);

    return FWK_SUCCESS;
}

static int tfm_regu_consumer_get_info(fwk_id_t dev_id,
                                        struct mod_voltd_info *info)
{
    struct tfm_regu_consumer_dev_ctx *ctx;
    int full_count;
    int ret;
    unsigned int min_idx, max_idx;
    int32_t volt_uv[2];

    ret = find_ctx(dev_id, &ctx);
    if (ret != FWK_SUCCESS)
        return ret;

    if (ctx->dev == NULL) {
        static const char reserved[] = "reserved";

        memset(info, 0, sizeof(*info));
        info->name = reserved;
        info->level_range.level_type = MOD_VOLTD_VOLTAGE_LEVEL_DISCRETE;
        info->level_range.level_count = 1;
        info->level_range.min_uv = 0;
        info->level_range.max_uv = 0;

        return FWK_SUCCESS;
    }

    full_count = regulator_count_voltages(ctx->dev);

    if (full_count == 0)
       return FWK_E_SUPPORT;
    min_idx = 0;
    max_idx = full_count-1;
    if (regulator_list_voltage(ctx->dev, min_idx, &volt_uv[0])) {
        return FWK_E_SUPPORT;
    }
    if (regulator_list_voltage(ctx->dev, max_idx, &volt_uv[1])) {
        return FWK_E_SUPPORT;
    }


    memset(info, 0, sizeof(*info));
    info->name = regu_name(ctx->dev);
    info->level_range.level_type = MOD_VOLTD_VOLTAGE_LEVEL_DISCRETE;
    info->level_range.level_count = full_count;
    info->level_range.min_uv = volt_uv[0];
    info->level_range.max_uv = volt_uv[1];

    FWK_LOG_INFO("SCMI voltd %u: get_info PMIC %s, range [%ld %ld]",
              fwk_id_get_element_idx(dev_id), regu_name(ctx->dev),
              info->level_range.min_uv, info->level_range.max_uv);

    return FWK_SUCCESS;
}

static int tfm_voltd_regulator_level_from_index(fwk_id_t dev_id,
                                                  unsigned int index,
                                                  int32_t *level_uv)
{
    struct tfm_regu_consumer_dev_ctx *ctx;
    int full_count;
    int32_t volt_uv;
    int ret = find_ctx(dev_id, &ctx);

    if (ret) {
        return ret;
    }
    if (ctx == NULL) {
        return FWK_E_PARAM;
    }

    if (ctx->dev == NULL) {
        /* Treat unexposed voltage domain a stubbed 0V fixed level regulator */
        if (index > 0) {
            return FWK_E_RANGE;
        }
        *level_uv = 0;

        return FWK_SUCCESS;
    }

    full_count = regulator_count_voltages(ctx->dev);
    if (index >= full_count)
        return FWK_E_PARAM;

    if (regulator_list_voltage(ctx->dev, index, &volt_uv)) {
        return FWK_E_SUPPORT;
    }
    *level_uv = volt_uv;

    FWK_LOG_DEBUG(MOD_NAME
                  "Get level from index for %u/%s: index %u, level %"PRId32"uV",
                  fwk_id_get_element_idx(dev_id), regu_name(ctx->dev),
                  index, *level_uv);

    return FWK_SUCCESS;
}

static const struct mod_voltd_drv_api api_tfm_regu = {
    .get_level = tfm_regu_consumer_get_level,
    .set_level = tfm_regu_consumer_set_level,
    .set_config = tfm_regu_consumer_set_config,
    .get_config = tfm_regu_consumer_get_config,
    .get_info = tfm_regu_consumer_get_info,
    .get_level_from_index = tfm_voltd_regulator_level_from_index,
};

/*
 * Framework handler functions
 */

static int tfm_regu_consumer_init(fwk_id_t module_id,
                                    unsigned int element_count,
                                    const void *data)
{
    module_ctx.dev_count = element_count;

    if (element_count)
        module_ctx.dev_ctx_table =
            fwk_mm_calloc(element_count, sizeof(*module_ctx.dev_ctx_table));

    return FWK_SUCCESS;
}

static int tfm_regu_consumer_element_init(fwk_id_t element_id,
                                            unsigned int unused,
                                            const void *data)
{
    const struct mod_tfm_regu_consumer_dev_config *dev_config = data;
    struct tfm_regu_consumer_dev_ctx *ctx;

    if (!fwk_module_is_valid_element_id(element_id))
        return FWK_E_PANIC;

    ctx = module_ctx.dev_ctx_table + fwk_id_get_element_idx(element_id);

    ctx->dev = (struct device *)dev_config->dev;
    ctx->enabled = dev_config->default_enabled;

    if (ctx->enabled) {
        if (!ctx->dev)
            return FWK_E_PARAM;
        if (regulator_enable(ctx->dev))
            return FWK_E_DEVICE;
    }

    return FWK_SUCCESS;
}

static int tfm_regu_consumer_process_bind_request(fwk_id_t requester_id,
                                                    fwk_id_t target_id,
                                                    fwk_id_t api_type,
                                                    const void **api)
{
    *api = &api_tfm_regu;

    return FWK_SUCCESS;
}

const struct fwk_module module_tfm_regu_consumer = {
    .type = FWK_MODULE_TYPE_DRIVER,
    .api_count = 1,
    .init = tfm_regu_consumer_init,
    .element_init = tfm_regu_consumer_element_init,
    .process_bind_request = tfm_regu_consumer_process_bind_request,
};
