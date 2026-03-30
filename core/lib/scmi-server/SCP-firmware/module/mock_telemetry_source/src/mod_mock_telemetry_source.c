/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <mod_mock_telemetry_source.h>
#include <mod_telemetry.h>

#include <fwk_id.h>
#include <fwk_log.h>
#include <fwk_mm.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>
#include <fwk_string.h>
#include <fwk_time.h>

#include <string.h>

#define MOD_NAME "[telemetry_source]"

#define DE_SIZE_MAX (3)
struct telemetry_de_ctx {
    bool enabled;
};

struct telemetry_source_ctx {
    const struct mod_mock_telemetry_source_config *config;
    struct telemetry_de_ctx *de_ctx_table;
    uint64_t *mock_values;
    struct mod_telemetry_de_data *de_data_list;
    const mod_mock_telemetry_source_mutator *value_mutators;
    size_t de_count;
    uint32_t de_enabled_count;
};

static struct telemetry_source_ctx telemetry_source_ctx;

static void mock_apply_mutator(
    struct telemetry_source_ctx *ctx,
    uint32_t de_index);

static void telemetry_remove_de_data_entry(
    struct telemetry_source_ctx *ctx,
    size_t remove_idx);

static inline size_t get_de_size_in_qwords(
    struct telemetry_source_ctx *src_ctx,
    size_t de_idx)
{
    return src_ctx->config->de_desc_table[de_idx].de_data_size /
        sizeof(*src_ctx->de_data_list->data);
}

/*
 * Mock telemetry source driver API Implementation
 */

static int telemetry_get_de_list(
    uint32_t *num_de,
    const struct mod_telemetry_de_desc **de_list)
{
    const struct telemetry_source_ctx *ctx = &telemetry_source_ctx;

    if (num_de == NULL || de_list == NULL) {
        return FWK_E_PARAM;
    }

    if (ctx->config == NULL || ctx->config->de_desc_table == NULL) {
        return FWK_E_STATE;
    }

    if (ctx->de_count > UINT32_MAX) {
        return FWK_E_RANGE;
    }

    *num_de = (uint32_t)ctx->de_count;
    *de_list = ctx->config->de_desc_table;

    return FWK_SUCCESS;
}

static int telemetry_source_get_name(uint32_t de_index, char *name)
{
    const struct telemetry_source_ctx *ctx = &telemetry_source_ctx;
    const char *de_name;
    size_t copy_len;

    if (name == NULL) {
        return FWK_E_PARAM;
    }

    if ((ctx->config == NULL) || (ctx->config->de_names == NULL)) {
        return FWK_E_STATE;
    }

    if (de_index >= ctx->de_count) {
        return FWK_E_PARAM;
    }

    de_name = ctx->config->de_names[de_index];
    if (de_name == NULL) {
        return FWK_E_STATE;
    }

    copy_len = strlen(de_name) + 1;
    fwk_str_strncpy(name, de_name, copy_len);

    return FWK_SUCCESS;
}

static int telemetry_source_get_de_fch_attr(
    uint32_t de_index,
    struct mod_telemetry_de_fch_attr *de_fch_attr)
{
    const struct telemetry_source_ctx *ctx = &telemetry_source_ctx;

    if (de_fch_attr == NULL) {
        return FWK_E_PARAM;
    }

    if (ctx->config == NULL) {
        return FWK_E_STATE;
    }

    if (de_index >= ctx->de_count) {
        return FWK_E_PARAM;
    }

    if (ctx->config->de_fch_attr_table == NULL) {
        memset(de_fch_attr, 0, sizeof(*de_fch_attr));
        return FWK_SUCCESS;
    }

    memcpy(
        de_fch_attr,
        &ctx->config->de_fch_attr_table[de_index],
        sizeof(struct mod_telemetry_de_fch_attr));

    return FWK_SUCCESS;
}

static int telemetry_source_enable_de(uint32_t idx)
{
    struct telemetry_source_ctx *ctx = &telemetry_source_ctx;
    struct telemetry_de_ctx *de_ctx;

    if ((ctx->config == NULL) || (ctx->de_ctx_table == NULL)) {
        return FWK_E_STATE;
    }

    if (idx >= ctx->de_count) {
        return FWK_E_RANGE;
    }

    de_ctx = &ctx->de_ctx_table[idx];
    if (de_ctx->enabled) {
        return FWK_SUCCESS;
    }

    de_ctx->enabled = true;

    size_t de_enabled_idx = ctx->de_enabled_count++;
    size_t data_qword_count = get_de_size_in_qwords(ctx, idx);
    ctx->de_data_list[de_enabled_idx].timestamp = fwk_time_current();
    ctx->de_data_list[de_enabled_idx].data_qword_count = data_qword_count;
    for (size_t i = 0; i < data_qword_count; ++i) {
        ctx->de_data_list[de_enabled_idx].data[i] = ctx->mock_values[idx];
    }
    ctx->de_data_list[de_enabled_idx].source_de_index = idx;

    return FWK_SUCCESS;
}

static void telemetry_remove_de_data_entry(
    struct telemetry_source_ctx *ctx,
    size_t remove_idx)
{
    size_t shift_count;
    struct mod_telemetry_de_data *vacated;

    if ((ctx == NULL) || (ctx->de_data_list == NULL)) {
        return;
    }

    if (remove_idx >= ctx->de_enabled_count) {
        return;
    }

    shift_count = ctx->de_enabled_count - remove_idx - 1;
    if (shift_count > 0) {
        memmove(
            &ctx->de_data_list[remove_idx],
            &ctx->de_data_list[remove_idx + 1],
            shift_count * sizeof(ctx->de_data_list[0]));
    }

    vacated = &ctx->de_data_list[ctx->de_enabled_count - 1];
    vacated->data_qword_count = 0;
    vacated->timestamp = 0;
    vacated->source_de_index = ctx->de_count;
}

static int telemetry_disable_de(uint32_t idx)
{
    struct telemetry_source_ctx *ctx = &telemetry_source_ctx;
    struct telemetry_de_ctx *de_ctx;

    if ((ctx->config == NULL) || (ctx->de_ctx_table == NULL)) {
        return FWK_E_STATE;
    }

    if (idx >= ctx->de_count) {
        return FWK_E_RANGE;
    }

    de_ctx = &ctx->de_ctx_table[idx];
    if (!de_ctx->enabled) {
        return FWK_SUCCESS;
    }
    bool entry_removed = false;
    for (size_t i = 0; i < ctx->de_enabled_count; i++) {
        if (ctx->de_data_list[i].source_de_index == idx) {
            telemetry_remove_de_data_entry(ctx, i);
            entry_removed = true;
            break;
        }
    }

    de_ctx->enabled = false;
    if (entry_removed && (ctx->de_enabled_count > 0)) {
        ctx->de_enabled_count--;
    }

    return FWK_SUCCESS;
}

static int telemetry_disable_de_all(void)
{
    struct telemetry_source_ctx *ctx = &telemetry_source_ctx;
    int status;
    size_t i;

    if ((ctx->config == NULL) || (ctx->de_ctx_table == NULL)) {
        return FWK_E_STATE;
    }

    for (i = 0; i < ctx->de_count; i++) {
        status = telemetry_disable_de((uint32_t)i);
        if (status != FWK_SUCCESS) {
            FWK_LOG_CRIT(
                MOD_NAME " %s:%d failed to disable DE(%u)",
                __func__,
                __LINE__,
                (unsigned int)i);
        }
    }

    return FWK_SUCCESS;
}

static void mock_apply_mutator(
    struct telemetry_source_ctx *ctx,
    uint32_t de_index)
{
    mod_mock_telemetry_source_mutator mutator;

    if ((ctx == NULL) || (ctx->mock_values == NULL)) {
        return;
    }

    if ((ctx->value_mutators == NULL) || (de_index >= ctx->de_count)) {
        return;
    }

    mutator = ctx->value_mutators[de_index];
    if (mutator != NULL) {
        mutator(&ctx->mock_values[de_index]);
    }
}

static int telemetry_source_get_data(
    struct mod_telemetry_de_data **data_array,
    uint32_t num_des)
{
    struct telemetry_source_ctx *ctx = &telemetry_source_ctx;
    *data_array = ctx->de_data_list;
    for (size_t i = 0; i < num_des; i++) {
        size_t de_idx = ctx->de_data_list[i].source_de_index;
        size_t data_qwords_count = get_de_size_in_qwords(ctx, de_idx);
        for (size_t j = 0; j < data_qwords_count; j++) {
            ctx->de_data_list[i].data[j] = ctx->mock_values[de_idx];
            mock_apply_mutator(ctx, de_idx);
        }
        ctx->de_data_list[i].timestamp = fwk_time_current();
    }
    return FWK_SUCCESS;
}

static const struct mod_telemetry_driver_api telemetry_driver_api = {
    .get_de_list = telemetry_get_de_list,
    .get_de_name = telemetry_source_get_name,
    .get_de_fch_attr = telemetry_source_get_de_fch_attr,
    .disable_de = telemetry_disable_de,
    .disable_de_all = telemetry_disable_de_all,
    .enable_de = telemetry_source_enable_de,
    .get_data = telemetry_source_get_data,
};

/*
 * Framework Module APIs
 */

static int telemetry_source_init(
    fwk_id_t module_id,
    unsigned int unused_data1,
    const void *data)
{
    struct telemetry_source_ctx *ctx = &telemetry_source_ctx;
    const struct mod_mock_telemetry_source_config *config = data;
    uint64_t *mock_values;
    struct telemetry_de_ctx *de_ctx_table;
    struct mod_telemetry_de_data *de_data_list;

    (void)module_id;
    (void)unused_data1;

    if (config == NULL) {
        return FWK_E_PARAM;
    }

    if ((config->de_count == 0) || (config->de_desc_table == NULL) ||
        (config->de_names == NULL)) {
        return FWK_E_PARAM;
    }

    ctx->de_count = config->de_count;
    ctx->de_enabled_count = 0;
    ctx->value_mutators = config->value_mutators;

    de_ctx_table =
        fwk_mm_calloc(ctx->de_count, sizeof(struct telemetry_de_ctx));
    if (de_ctx_table == NULL) {
        return FWK_E_NOMEM;
    }

    ctx->de_ctx_table = de_ctx_table;

    mock_values = fwk_mm_alloc(ctx->de_count, sizeof(uint64_t));
    if (mock_values == NULL) {
        fwk_mm_free(ctx->de_ctx_table);
        ctx->de_ctx_table = NULL;
        return FWK_E_NOMEM;
    }

    de_data_list = fwk_mm_alloc(ctx->de_count, sizeof(de_data_list[0]));
    if (de_data_list == NULL) {
        return FWK_E_NOMEM;
    }

    for (size_t i = 0; i < ctx->de_count; ++i) {
        de_data_list[i].data = fwk_mm_alloc(DE_SIZE_MAX, sizeof(uint64_t));
        de_data_list[i].source_de_index = ctx->de_count;
        mock_values[i] =
            (config->initial_values != NULL) ? config->initial_values[i] : 0;
    }

    ctx->mock_values = mock_values;
    ctx->de_data_list = de_data_list;
    ctx->config = config;

    return FWK_SUCCESS;
}

static int telemetry_source_process_bind_request(
    fwk_id_t requester_id,
    fwk_id_t target_id,
    fwk_id_t api_id,
    const void **api)
{
    if (api == NULL) {
        return FWK_E_PARAM;
    }

    *api = &telemetry_driver_api;

    return FWK_SUCCESS;
}

const struct fwk_module module_mock_telemetry_source = {
    .type = FWK_MODULE_TYPE_DRIVER,
    .api_count = 1,
    .init = telemetry_source_init,
    .process_bind_request = telemetry_source_process_bind_request,
};
