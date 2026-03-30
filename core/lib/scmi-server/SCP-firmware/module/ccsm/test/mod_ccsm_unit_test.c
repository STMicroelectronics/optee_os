/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "scp_unity.h"
#include "unity.h"

#ifdef TEST_ON_TARGET
#    include <fwk_id.h>
#    include <fwk_module.h>
#else
#    include <Mockfwk_id.h>
#    include <Mockfwk_module.h>
#endif

#include <Mockmod_ccsm_extra.h>

#include <mod_ccsm.h>

#include <fwk_element.h>
#include <fwk_macros.h>

#include UNIT_TEST_SRC

#define BUILD_HAS_BASE_PROTOCOL

#define FAKE_MODULE_ID 0x5

enum fake_ccsm {
    FAKE_CCSM_IDX_CONTINUOUS,
    FAKE_CCSM_IDX_INDEXED,
    FAKE_CCSM_IDX_INDEXED_SWFB,
    FAKE_CCSM_IDX_COUNT,
};

#define FAKE_API_IDX_TIMER 0x4

#define CLOCK_RATE_MAX 2000 * FWK_MHZ
#define CLOCK_RATE_MIN 300 * FWK_MHZ

static const struct mod_ccsm_clock_rate clock_config_default = { 1500 * FWK_MHZ,
                                                                 0 };

static const struct mod_ccsm_dm_config dm_config_default_nom_only = {
    .strategy = MOD_CCSM_DM_NOM_ONLY,
    .dd = MOD_CCSM_DM_ARM_DD,
    .soff = MOD_CCSM_DM_SW_SOFF_STOP,
    .transition_pause = 6,
    .mitigation_duration = 144
};

static const struct mod_ccsm_dm_config dm_config_default_sw_fb = {
    .strategy = MOD_CCSM_DM_SW_FB,
    .dd = MOD_CCSM_DM_ARM_DD,
    .soff = MOD_CCSM_DM_SW_SOFF_STOP,
    .transition_pause = 6,
    .mitigation_duration = 144
};

static const struct mod_ccsm_mod_config mod_config_default = {
    .numerator_oc = 1,
    .numerator_regular = 1,
    .denominator = 1
};

static const struct mod_ccsm_clock_rate_reg_value reg_rate_table_fake[] = {
    { .clock_rate_hz = 100 * FWK_MHZ,
      .pll_settings_0 = 0x01000000,
      .pll_settings_1 = 0xFFFF0100 },
    { .clock_rate_hz = 200 * FWK_MHZ,
      .pll_settings_0 = 0x02000000,
      .pll_settings_1 = 0xFFFF0200 },
    { .clock_rate_hz = 300 * FWK_MHZ,
      .pll_settings_0 = 0x03000000,
      .pll_settings_1 = 0xFFFF0300 },
    { .clock_rate_hz = 400 * FWK_MHZ,
      .pll_settings_0 = 0x04000000,
      .pll_settings_1 = 0xFFFF0400 },
    { .clock_rate_hz = 500 * FWK_MHZ,
      .pll_settings_0 = 0x05000000,
      .pll_settings_1 = 0xFFFF0500 },
    { .clock_rate_hz = 600 * FWK_MHZ,
      .pll_settings_0 = 0x06000000,
      .pll_settings_1 = 0xFFFF0600 },
    { .clock_rate_hz = 700 * FWK_MHZ,
      .pll_settings_0 = 0x07000000,
      .pll_settings_1 = 0xFFFF0700 },
    { .clock_rate_hz = 800 * FWK_MHZ,
      .pll_settings_0 = 0x08000000,
      .pll_settings_1 = 0xFFFF0800 },
    { .clock_rate_hz = 900 * FWK_MHZ,
      .pll_settings_0 = 0x09000000,
      .pll_settings_1 = 0xFFFF0900 },
    { .clock_rate_hz = 1000 * FWK_MHZ,
      .pll_settings_0 = 0x10000000,
      .pll_settings_1 = 0xFFFF1000 },
    { .clock_rate_hz = 1100 * FWK_MHZ,
      .pll_settings_0 = 0x11000000,
      .pll_settings_1 = 0xFFFF1100 },
    { .clock_rate_hz = 1200 * FWK_MHZ,
      .pll_settings_0 = 0x12000000,
      .pll_settings_1 = 0xFFFF1200 },
    { .clock_rate_hz = 1300 * FWK_MHZ,
      .pll_settings_0 = 0x13000000,
      .pll_settings_1 = 0xFFFF1300 },
    { .clock_rate_hz = 1400 * FWK_MHZ,
      .pll_settings_0 = 0x14000000,
      .pll_settings_1 = 0xFFFF1400 },
    { .clock_rate_hz = 1500 * FWK_MHZ,
      .pll_settings_0 = 0x15000000,
      .pll_settings_1 = 0xFFFF1500 },
    { .clock_rate_hz = 1600 * FWK_MHZ,
      .pll_settings_0 = 0x16000000,
      .pll_settings_1 = 0xFFFF1600 },
    { .clock_rate_hz = 1700 * FWK_MHZ,
      .pll_settings_0 = 0x17000000,
      .pll_settings_1 = 0xFFFF1700 },
    { .clock_rate_hz = 1800 * FWK_MHZ,
      .pll_settings_0 = 0x18000000,
      .pll_settings_1 = 0xFFFF1800 },
    { .clock_rate_hz = 1900 * FWK_MHZ,
      .pll_settings_0 = 0x19000000,
      .pll_settings_1 = 0xFFFF1900 },
    { .clock_rate_hz = 2000 * FWK_MHZ,
      .pll_settings_0 = 0x20000000,
      .pll_settings_1 = 0xFFFF2000 }
};

static const struct mod_ccsm_clock_rate rate_table_fake[] = {
    { /* Super Underdrive */
      .nominal_clock_rate_hz = 300 * FWK_MHZ,
      .fallback_clock_rate_hz = 0 },
    { /* Nominal */
      .nominal_clock_rate_hz = 1500 * FWK_MHZ,
      .fallback_clock_rate_hz = 0 },
    { /* Overdrive */
      .nominal_clock_rate_hz = 1800 * FWK_MHZ,
      .fallback_clock_rate_hz = 1600 * FWK_MHZ },
    { /* Super Overdrive */
      .nominal_clock_rate_hz = 2000 * FWK_MHZ,
      .fallback_clock_rate_hz = 1800 * FWK_MHZ },
};

static const struct fwk_element element_table[] = {
    [FAKE_CCSM_IDX_CONTINUOUS] = { .name = "CCSM continuous",
                                   .data =
                                       &(struct mod_ccsm_dev_config){
                                           .rate_lookup_table_is_provided =
                                               false,
                                           .minimum_clock_rate_fallback_enable_hz =
                                               1700 * FWK_MHZ,
                                           .fallback_clock_percentage = 90,
                                           .min_clock_rate_hz = CLOCK_RATE_MIN,
                                           .max_clock_rate_hz = CLOCK_RATE_MAX,
                                           .clock_rate_step_hz = 100 * FWK_MHZ,
                                           .base_address = 0xBA5E,
                                           .default_rates_table =
                                               &clock_config_default,
                                           .register_rate_table =
                                               reg_rate_table_fake,
                                           .register_rate_count = 20,
                                           .droop_mitigation_default =
                                               &dm_config_default_nom_only,
                                           .modulator_default =
                                               &mod_config_default,
                                           .modulator_count = 1 } },
    [FAKE_CCSM_IDX_INDEXED] = { .name = "CCSM indexed",
                                .data =
                                    &(struct mod_ccsm_dev_config){
                                        .rate_lookup_table_is_provided = true,
                                        .base_address = 0x1DBA5E,
                                        .min_clock_rate_hz = CLOCK_RATE_MIN,
                                        .max_clock_rate_hz = CLOCK_RATE_MAX,
                                        .clock_rate_step_hz = 100 * FWK_MHZ,
                                        .default_rates_table =
                                            &clock_config_default,
                                        .droop_mitigation_default =
                                            &dm_config_default_nom_only,
                                        .modulator_default =
                                            &mod_config_default,
                                        .modulator_count = 1,
                                        .register_rate_table =
                                            reg_rate_table_fake,
                                        .register_rate_count = 20,
                                        .rate_table = rate_table_fake,
                                        .rate_count = 4 } },
    [FAKE_CCSM_IDX_INDEXED_SWFB] = { .name = "CCSM indexed switch to fallback",
                                     .data =
                                         &(struct mod_ccsm_dev_config){
                                             .rate_lookup_table_is_provided =
                                                 true,
                                             .base_address = 0xFBBA5E,
                                             .min_clock_rate_hz =
                                                 CLOCK_RATE_MIN,
                                             .max_clock_rate_hz =
                                                 CLOCK_RATE_MAX,
                                             .clock_rate_step_hz =
                                                 100 * FWK_MHZ,
                                             .default_rates_table =
                                                 &clock_config_default,
                                             .droop_mitigation_default =
                                                 &dm_config_default_sw_fb,
                                             .modulator_default =
                                                 &mod_config_default,
                                             .modulator_count = 1,
                                             .register_rate_table =
                                                 reg_rate_table_fake,
                                             .register_rate_count = 20,
                                             .rate_table = rate_table_fake,
                                             .rate_count = 4 } },
    [FAKE_CCSM_IDX_COUNT] = { 0 },
};

static const struct fwk_element *get_element_table(fwk_id_t module_id)
{
    return element_table;
}

struct fwk_module_config config_ccsm = {
    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(get_element_table),
};

static int ccsm_dummy_wait(
    fwk_id_t dev_id,
    uint32_t microseconds,
    bool (*cond)(void *),
    void *data)
{
    return FWK_SUCCESS;
}

struct mod_timer_api *timer_api =
    &(struct mod_timer_api){ .wait = ccsm_dummy_wait };

void setUp(void)
{
    struct ccsm_dev_ctx *ctx;

    module_ctx.dev_ctx_table =
        fwk_mm_calloc(FAKE_CCSM_IDX_COUNT, sizeof(module_ctx.dev_ctx_table[0]));
    module_ctx.dev_count = FAKE_CCSM_IDX_COUNT;

    ctx = &module_ctx.dev_ctx_table[FAKE_CCSM_IDX_CONTINUOUS];
    ctx->config =
        (struct mod_ccsm_dev_config *)element_table[FAKE_CCSM_IDX_CONTINUOUS]
            .data;
    ctx->initialized = true;
    ctx->current_nominal_clock_rate_hz =
        ctx->config->default_rates_table->nominal_clock_rate_hz;
    ctx->current_fallback_clock_rate_hz =
        ctx->config->default_rates_table->fallback_clock_rate_hz;
    ctx->current_state = MOD_CLOCK_STATE_RUNNING;
    ctx->timer_api = timer_api;

    ctx = &module_ctx.dev_ctx_table[FAKE_CCSM_IDX_INDEXED];
    ctx->config =
        (struct mod_ccsm_dev_config *)element_table[FAKE_CCSM_IDX_INDEXED].data;
    ctx->initialized = true;
    ctx->current_nominal_clock_rate_hz =
        ctx->config->default_rates_table->nominal_clock_rate_hz;
    ctx->current_fallback_clock_rate_hz =
        ctx->config->default_rates_table->fallback_clock_rate_hz;
    ctx->current_state = MOD_CLOCK_STATE_RUNNING;
    ctx->timer_api = timer_api;

    ctx = &module_ctx.dev_ctx_table[FAKE_CCSM_IDX_INDEXED_SWFB];
    ctx->config =
        (struct mod_ccsm_dev_config *)element_table[FAKE_CCSM_IDX_INDEXED_SWFB]
            .data;
    ctx->initialized = true;
    ctx->current_nominal_clock_rate_hz =
        ctx->config->default_rates_table->nominal_clock_rate_hz;
    ctx->current_fallback_clock_rate_hz =
        ctx->config->default_rates_table->fallback_clock_rate_hz;
    ctx->current_state = MOD_CLOCK_STATE_RUNNING;
    ctx->timer_api = timer_api;
}

void tearDown(void)
{
}

void test_function_ccsm_clock_select_rates_pass_continuous_in_range(void)
{
    int status;
    struct mod_ccsm_clock_rate clock_rates;
    struct ccsm_dev_ctx *ctx;

    ctx = &module_ctx.dev_ctx_table[FAKE_CCSM_IDX_CONTINUOUS];

    status = ccsm_clock_select_rates(
        ctx,
        ((1800 * FWK_MHZ) - (90 * FWK_MHZ)),
        MOD_CLOCK_ROUND_MODE_UP,
        &clock_rates);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL(1800 * FWK_MHZ, clock_rates.nominal_clock_rate_hz);
    TEST_ASSERT_EQUAL(1600 * FWK_MHZ, clock_rates.fallback_clock_rate_hz);
}

void test_function_ccsm_clock_select_rates_pass_continuous_min(void)
{
    int status;
    struct mod_ccsm_clock_rate clock_rates;
    struct ccsm_dev_ctx *ctx;

    ctx = &module_ctx.dev_ctx_table[FAKE_CCSM_IDX_CONTINUOUS];

    status = ccsm_clock_select_rates(
        ctx,
        ((400 * FWK_MHZ) - (10 * FWK_MHZ)),
        MOD_CLOCK_ROUND_MODE_DOWN,
        &clock_rates);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL(300 * FWK_MHZ, clock_rates.nominal_clock_rate_hz);
    TEST_ASSERT_EQUAL(0, clock_rates.fallback_clock_rate_hz);
}

void test_function_ccsm_clock_select_rates_pass_continuous_max(void)
{
    int status;
    struct mod_ccsm_clock_rate clock_rates;
    struct ccsm_dev_ctx *ctx;

    ctx = &module_ctx.dev_ctx_table[FAKE_CCSM_IDX_CONTINUOUS];

    status = ccsm_clock_select_rates(
        ctx,
        ((2000 * FWK_MHZ) - (49 * FWK_MHZ)),
        MOD_CLOCK_ROUND_MODE_NEAREST,
        &clock_rates);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL(2000 * FWK_MHZ, clock_rates.nominal_clock_rate_hz);
    TEST_ASSERT_EQUAL(1800 * FWK_MHZ, clock_rates.fallback_clock_rate_hz);
}

void test_function_ccsm_clock_select_rates_fail_continuous_low(void)
{
    int status;
    struct mod_ccsm_clock_rate clock_rates;
    struct ccsm_dev_ctx *ctx;

    ctx = &module_ctx.dev_ctx_table[FAKE_CCSM_IDX_CONTINUOUS];

    status = ccsm_clock_select_rates(
        ctx,
        ((200 * FWK_MHZ) - (10 * FWK_MHZ)),
        MOD_CLOCK_ROUND_MODE_DOWN,
        &clock_rates);

    TEST_ASSERT_EQUAL(FWK_E_SUPPORT, status);
}

void test_function_ccsm_clock_select_rates_fail_continuous_high(void)
{
    int status;
    struct mod_ccsm_clock_rate clock_rates;
    struct ccsm_dev_ctx *ctx;

    ctx = &module_ctx.dev_ctx_table[FAKE_CCSM_IDX_CONTINUOUS];

    status = ccsm_clock_select_rates(
        ctx,
        ((2200 * FWK_MHZ) - (10 * FWK_MHZ)),
        MOD_CLOCK_ROUND_MODE_DOWN,
        &clock_rates);

    TEST_ASSERT_EQUAL(FWK_E_SUPPORT, status);
}

void test_function_ccsm_clock_select_rates_pass_indexed_in_range(void)
{
    int status;
    struct mod_ccsm_clock_rate clock_rates;
    struct ccsm_dev_ctx *ctx;

    ctx = &module_ctx.dev_ctx_table[FAKE_CCSM_IDX_INDEXED];

    status = ccsm_clock_select_rates(
        ctx, (1800 * FWK_MHZ), MOD_CLOCK_ROUND_MODE_UP, &clock_rates);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL(1800 * FWK_MHZ, clock_rates.nominal_clock_rate_hz);
    TEST_ASSERT_EQUAL(1600 * FWK_MHZ, clock_rates.fallback_clock_rate_hz);
}

void test_function_ccsm_clock_select_rates_pass_indexed_min(void)
{
    int status;
    struct mod_ccsm_clock_rate clock_rates;
    struct ccsm_dev_ctx *ctx;

    ctx = &module_ctx.dev_ctx_table[FAKE_CCSM_IDX_INDEXED];

    status = ccsm_clock_select_rates(
        ctx, (300 * FWK_MHZ), MOD_CLOCK_ROUND_MODE_DOWN, &clock_rates);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL(300 * FWK_MHZ, clock_rates.nominal_clock_rate_hz);
    TEST_ASSERT_EQUAL(0, clock_rates.fallback_clock_rate_hz);
}

void test_function_ccsm_clock_select_rates_pass_indexed_max(void)
{
    int status;
    struct mod_ccsm_clock_rate clock_rates;
    struct ccsm_dev_ctx *ctx;

    ctx = &module_ctx.dev_ctx_table[FAKE_CCSM_IDX_INDEXED];

    status = ccsm_clock_select_rates(
        ctx, (2000 * FWK_MHZ), MOD_CLOCK_ROUND_MODE_NEAREST, &clock_rates);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL(2000 * FWK_MHZ, clock_rates.nominal_clock_rate_hz);
    TEST_ASSERT_EQUAL(1800 * FWK_MHZ, clock_rates.fallback_clock_rate_hz);
}

void test_function_ccsm_clock_select_rates_fail_indexed_low(void)
{
    int status;
    struct mod_ccsm_clock_rate clock_rates;
    struct ccsm_dev_ctx *ctx;

    ctx = &module_ctx.dev_ctx_table[FAKE_CCSM_IDX_INDEXED];

    status = ccsm_clock_select_rates(
        ctx, (200 * FWK_MHZ), MOD_CLOCK_ROUND_MODE_DOWN, &clock_rates);

    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
}

void test_function_ccsm_clock_select_rates_fail_indexed_high(void)
{
    int status;
    struct mod_ccsm_clock_rate clock_rates;
    struct ccsm_dev_ctx *ctx;

    ctx = &module_ctx.dev_ctx_table[FAKE_CCSM_IDX_INDEXED];

    status = ccsm_clock_select_rates(
        ctx, (2200 * FWK_MHZ), MOD_CLOCK_ROUND_MODE_DOWN, &clock_rates);

    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
}

void test_function_ccsm_clock_select_rates_fail_round_none(void)
{
    int status;
    struct mod_ccsm_clock_rate clock_rates;
    struct ccsm_dev_ctx *ctx;

    ctx = &module_ctx.dev_ctx_table[FAKE_CCSM_IDX_CONTINUOUS];

    status = ccsm_clock_select_rates(
        ctx,
        ((1800 * FWK_MHZ) - (100 * FWK_KHZ)),
        MOD_CLOCK_ROUND_MODE_NONE,
        &clock_rates);

    TEST_ASSERT_EQUAL(FWK_E_RANGE, status);
}

void test_function_ccsm_clock_set_rate_pass_continuous_uninitialized(void)
{
    int status;
    uint64_t rate = 1800 * FWK_MHZ;
    uint64_t read_rate;
    struct ccsm_dev_ctx *ctx;

    fwk_id_t dev_id =
        FWK_ID_ELEMENT_INIT(FAKE_MODULE_ID, FAKE_CCSM_IDX_CONTINUOUS);

    ctx = &module_ctx.dev_ctx_table[FAKE_CCSM_IDX_CONTINUOUS];
    ctx->initialized = false;

    fwk_module_is_valid_element_id_ExpectAndReturn(dev_id, true);
    fwk_id_get_element_idx_ExpectAndReturn(dev_id, FAKE_CCSM_IDX_CONTINUOUS);

    ccsm_drv_set_mod_configuration_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    ccsm_drv_set_dm_configuration_ExpectAnyArgsAndReturn(FWK_SUCCESS);

    ccsm_drv_set_request_ExpectAndReturn(
        (struct ccsm_reg *)ctx->config->base_address,
        CCSM_REQUEST_TYPE_SET_DM,
        FWK_SUCCESS);
    ccsm_drv_get_clear_irq_status_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    ccsm_drv_clear_request_ExpectAndReturn(
        (struct ccsm_reg *)ctx->config->base_address, FWK_SUCCESS);

    ccsm_drv_set_request_ExpectAndReturn(
        (struct ccsm_reg *)ctx->config->base_address,
        CCSM_REQUEST_TYPE_SET_MOD,
        FWK_SUCCESS);
    ccsm_drv_get_clear_irq_status_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    ccsm_drv_clear_request_ExpectAndReturn(
        (struct ccsm_reg *)ctx->config->base_address, FWK_SUCCESS);

    ccsm_drv_set_pll_static_settings_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    ccsm_drv_set_pll_static_settings_ExpectAnyArgsAndReturn(FWK_SUCCESS);

    ccsm_drv_set_pll_dynamic_settings_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    ccsm_drv_set_pll_dynamic_settings_ExpectAnyArgsAndReturn(FWK_SUCCESS);

    ccsm_drv_set_request_ExpectAndReturn(
        (struct ccsm_reg *)ctx->config->base_address,
        CCSM_REQUEST_TYPE_SET_DVFS,
        FWK_SUCCESS);
    ccsm_drv_get_clear_irq_status_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    ccsm_drv_clear_request_ExpectAndReturn(
        (struct ccsm_reg *)ctx->config->base_address, FWK_SUCCESS);

    status = ccsm_clock_set_rate(dev_id, rate, MOD_CLOCK_ROUND_MODE_NEAREST);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

    fwk_module_is_valid_element_id_ExpectAndReturn(dev_id, true);
    fwk_id_get_element_idx_ExpectAndReturn(dev_id, FAKE_CCSM_IDX_CONTINUOUS);

    status = ccsm_clock_get_rate(dev_id, &read_rate);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL(1800 * FWK_MHZ, read_rate);

    ctx->initialized = true;
}

void test_function_ccsm_clock_set_rate_pass_indexed_initialized(void)
{
    int status;
    uint64_t rate = 1800 * FWK_MHZ;
    uint64_t read_rate;
    struct ccsm_dev_ctx *ctx;
    fwk_id_t dev_id =
        FWK_ID_ELEMENT_INIT(FAKE_MODULE_ID, FAKE_CCSM_IDX_INDEXED);

    ctx = &module_ctx.dev_ctx_table[FAKE_CCSM_IDX_INDEXED];
    ctx->initialized = true;

    fwk_module_is_valid_element_id_ExpectAndReturn(dev_id, true);
    fwk_id_get_element_idx_ExpectAndReturn(dev_id, FAKE_CCSM_IDX_INDEXED);

    ccsm_drv_set_pll_dynamic_settings_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    ccsm_drv_set_pll_dynamic_settings_ExpectAnyArgsAndReturn(FWK_SUCCESS);

    ccsm_drv_set_request_ExpectAndReturn(
        (struct ccsm_reg *)ctx->config->base_address,
        CCSM_REQUEST_TYPE_GO_UP,
        FWK_SUCCESS);
    ccsm_drv_get_clear_irq_status_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    ccsm_drv_clear_request_ExpectAndReturn(
        (struct ccsm_reg *)ctx->config->base_address, FWK_SUCCESS);

    ccsm_drv_set_request_ExpectAndReturn(
        (struct ccsm_reg *)ctx->config->base_address,
        CCSM_REQUEST_TYPE_GO_UP2,
        FWK_SUCCESS);
    ccsm_drv_get_clear_irq_status_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    ccsm_drv_clear_request_ExpectAndReturn(
        (struct ccsm_reg *)ctx->config->base_address, FWK_SUCCESS);

    status = ccsm_clock_set_rate(dev_id, rate, MOD_CLOCK_ROUND_MODE_NEAREST);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

    fwk_module_is_valid_element_id_ExpectAndReturn(dev_id, true);
    fwk_id_get_element_idx_ExpectAndReturn(dev_id, FAKE_CCSM_IDX_INDEXED);

    status = ccsm_clock_get_rate(dev_id, &read_rate);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL(1800 * FWK_MHZ, read_rate);
}

void test_function_ccsm_clock_set_rate_pass_indexed_uninitialized(void)
{
    int status;
    uint64_t rate = 1800 * FWK_MHZ;
    uint64_t read_rate;
    struct ccsm_dev_ctx *ctx;
    fwk_id_t dev_id =
        FWK_ID_ELEMENT_INIT(FAKE_MODULE_ID, FAKE_CCSM_IDX_INDEXED);

    ctx = &module_ctx.dev_ctx_table[FAKE_CCSM_IDX_INDEXED];
    ctx->initialized = false;

    fwk_module_is_valid_element_id_ExpectAndReturn(dev_id, true);
    fwk_id_get_element_idx_ExpectAndReturn(dev_id, FAKE_CCSM_IDX_INDEXED);

    ccsm_drv_set_mod_configuration_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    ccsm_drv_set_dm_configuration_ExpectAnyArgsAndReturn(FWK_SUCCESS);

    ccsm_drv_set_request_ExpectAndReturn(
        (struct ccsm_reg *)ctx->config->base_address,
        CCSM_REQUEST_TYPE_SET_DM,
        FWK_SUCCESS);
    ccsm_drv_get_clear_irq_status_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    ccsm_drv_clear_request_ExpectAndReturn(
        (struct ccsm_reg *)ctx->config->base_address, FWK_SUCCESS);

    ccsm_drv_set_request_ExpectAndReturn(
        (struct ccsm_reg *)ctx->config->base_address,
        CCSM_REQUEST_TYPE_SET_MOD,
        FWK_SUCCESS);
    ccsm_drv_get_clear_irq_status_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    ccsm_drv_clear_request_ExpectAndReturn(
        (struct ccsm_reg *)ctx->config->base_address, FWK_SUCCESS);

    ccsm_drv_set_pll_static_settings_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    ccsm_drv_set_pll_static_settings_ExpectAnyArgsAndReturn(FWK_SUCCESS);

    ccsm_drv_set_pll_dynamic_settings_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    ccsm_drv_set_pll_dynamic_settings_ExpectAnyArgsAndReturn(FWK_SUCCESS);

    ccsm_drv_set_request_ExpectAndReturn(
        (struct ccsm_reg *)ctx->config->base_address,
        CCSM_REQUEST_TYPE_SET_DVFS,
        FWK_SUCCESS);
    ccsm_drv_get_clear_irq_status_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    ccsm_drv_clear_request_ExpectAndReturn(
        (struct ccsm_reg *)ctx->config->base_address, FWK_SUCCESS);

    status = ccsm_clock_set_rate(dev_id, rate, MOD_CLOCK_ROUND_MODE_NEAREST);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

    fwk_module_is_valid_element_id_ExpectAndReturn(dev_id, true);
    fwk_id_get_element_idx_ExpectAndReturn(dev_id, FAKE_CCSM_IDX_INDEXED);

    status = ccsm_clock_get_rate(dev_id, &read_rate);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL(1800 * FWK_MHZ, read_rate);
}

void test_function_ccsm_clock_set_rate_pass_continuous_initialized(void)
{
    int status;
    uint64_t rate = 1800 * FWK_MHZ;
    uint64_t read_rate;
    struct ccsm_dev_ctx *ctx;
    fwk_id_t dev_id =
        FWK_ID_ELEMENT_INIT(FAKE_MODULE_ID, FAKE_CCSM_IDX_CONTINUOUS);

    ctx = &module_ctx.dev_ctx_table[FAKE_CCSM_IDX_CONTINUOUS];

    fwk_module_is_valid_element_id_ExpectAndReturn(dev_id, true);
    fwk_id_get_element_idx_ExpectAndReturn(dev_id, FAKE_CCSM_IDX_CONTINUOUS);

    ccsm_drv_set_pll_dynamic_settings_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    ccsm_drv_set_pll_dynamic_settings_ExpectAnyArgsAndReturn(FWK_SUCCESS);

    ccsm_drv_set_request_ExpectAndReturn(
        (struct ccsm_reg *)ctx->config->base_address,
        CCSM_REQUEST_TYPE_GO_UP,
        FWK_SUCCESS);
    ccsm_drv_get_clear_irq_status_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    ccsm_drv_clear_request_ExpectAndReturn(
        (struct ccsm_reg *)ctx->config->base_address, FWK_SUCCESS);

    ccsm_drv_set_request_ExpectAndReturn(
        (struct ccsm_reg *)ctx->config->base_address,
        CCSM_REQUEST_TYPE_GO_UP2,
        FWK_SUCCESS);
    ccsm_drv_get_clear_irq_status_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    ccsm_drv_clear_request_ExpectAndReturn(
        (struct ccsm_reg *)ctx->config->base_address, FWK_SUCCESS);

    status = ccsm_clock_set_rate(dev_id, rate, MOD_CLOCK_ROUND_MODE_NEAREST);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

    fwk_module_is_valid_element_id_ExpectAndReturn(dev_id, true);
    fwk_id_get_element_idx_ExpectAndReturn(dev_id, FAKE_CCSM_IDX_CONTINUOUS);

    status = ccsm_clock_get_rate(dev_id, &read_rate);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL(1800 * FWK_MHZ, read_rate);
}

void test_function_ccsm_clock_set_rate_pass_continuous_initialized_dn(void)
{
    int status;
    uint64_t rate = 1800 * FWK_MHZ;
    uint64_t read_rate;
    struct ccsm_dev_ctx *ctx;
    fwk_id_t dev_id =
        FWK_ID_ELEMENT_INIT(FAKE_MODULE_ID, FAKE_CCSM_IDX_CONTINUOUS);

    ctx = &module_ctx.dev_ctx_table[FAKE_CCSM_IDX_CONTINUOUS];
    ctx->current_nominal_clock_rate_hz = 2000 * FWK_MHZ;

    fwk_module_is_valid_element_id_ExpectAndReturn(dev_id, true);
    fwk_id_get_element_idx_ExpectAndReturn(dev_id, FAKE_CCSM_IDX_CONTINUOUS);

    ccsm_drv_set_pll_dynamic_settings_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    ccsm_drv_set_pll_dynamic_settings_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    ccsm_drv_set_pll_dynamic_settings_ExpectAnyArgsAndReturn(FWK_SUCCESS);

    ccsm_drv_set_request_ExpectAndReturn(
        (struct ccsm_reg *)ctx->config->base_address,
        CCSM_REQUEST_TYPE_GO_DN,
        FWK_SUCCESS);
    ccsm_drv_get_clear_irq_status_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    ccsm_drv_clear_request_ExpectAndReturn(
        (struct ccsm_reg *)ctx->config->base_address, FWK_SUCCESS);

    ccsm_drv_set_request_ExpectAndReturn(
        (struct ccsm_reg *)ctx->config->base_address,
        CCSM_REQUEST_TYPE_GO_DN2,
        FWK_SUCCESS);
    ccsm_drv_get_clear_irq_status_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    ccsm_drv_clear_request_ExpectAndReturn(
        (struct ccsm_reg *)ctx->config->base_address, FWK_SUCCESS);

    status = ccsm_clock_set_rate(dev_id, rate, MOD_CLOCK_ROUND_MODE_NEAREST);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

    fwk_module_is_valid_element_id_ExpectAndReturn(dev_id, true);
    fwk_id_get_element_idx_ExpectAndReturn(dev_id, FAKE_CCSM_IDX_CONTINUOUS);

    status = ccsm_clock_get_rate(dev_id, &read_rate);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL(1800 * FWK_MHZ, read_rate);
}

void test_function_ccsm_clock_set_rate_pass_swfb_up(void)
{
    int status;
    uint64_t rate = 1800 * FWK_MHZ;
    uint64_t read_rate;
    uint32_t ccsm_strategy_nom_only = 0x000200FFu;
    uint32_t ccsm_strategy_swfb = 0x000100FFu;
    struct ccsm_dev_ctx *ctx;
    fwk_id_t dev_id =
        FWK_ID_ELEMENT_INIT(FAKE_MODULE_ID, FAKE_CCSM_IDX_INDEXED_SWFB);

    ctx = &module_ctx.dev_ctx_table[FAKE_CCSM_IDX_INDEXED_SWFB];
    ctx->initialized = true;
    ctx->current_nominal_clock_rate_hz = 1500 * FWK_MHZ;
    ctx->current_fallback_clock_rate_hz = 0;

    fwk_module_is_valid_element_id_ExpectAndReturn(dev_id, true);
    fwk_id_get_element_idx_ExpectAndReturn(dev_id, FAKE_CCSM_IDX_INDEXED_SWFB);

    ccsm_drv_set_pll_dynamic_settings_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    ccsm_drv_set_pll_dynamic_settings_ExpectAnyArgsAndReturn(FWK_SUCCESS);

    ccsm_drv_get_dm_configuration_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    ccsm_drv_get_dm_configuration_ReturnThruPtr_config(&ccsm_strategy_nom_only);

    ccsm_drv_set_dm_configuration_ExpectAndReturn(
        (struct ccsm_reg *)ctx->config->base_address,
        ccsm_strategy_swfb,
        FWK_SUCCESS);

    ccsm_drv_set_request_ExpectAndReturn(
        (struct ccsm_reg *)ctx->config->base_address,
        CCSM_REQUEST_TYPE_GO_UP,
        FWK_SUCCESS);
    ccsm_drv_get_clear_irq_status_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    ccsm_drv_clear_request_ExpectAndReturn(
        (struct ccsm_reg *)ctx->config->base_address, FWK_SUCCESS);

    ccsm_drv_set_request_ExpectAndReturn(
        (struct ccsm_reg *)ctx->config->base_address,
        CCSM_REQUEST_TYPE_GO_UP2,
        FWK_SUCCESS);
    ccsm_drv_get_clear_irq_status_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    ccsm_drv_clear_request_ExpectAndReturn(
        (struct ccsm_reg *)ctx->config->base_address, FWK_SUCCESS);

    status = ccsm_clock_set_rate(dev_id, rate, MOD_CLOCK_ROUND_MODE_NEAREST);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

    fwk_module_is_valid_element_id_ExpectAndReturn(dev_id, true);
    fwk_id_get_element_idx_ExpectAndReturn(dev_id, FAKE_CCSM_IDX_INDEXED_SWFB);

    status = ccsm_clock_get_rate(dev_id, &read_rate);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL(1800 * FWK_MHZ, read_rate);
    TEST_ASSERT_EQUAL(1600 * FWK_MHZ, ctx->current_fallback_clock_rate_hz);
}

void test_function_ccsm_clock_set_rate_pass_swfb_down(void)
{
    int status;
    uint64_t rate = 1500 * FWK_MHZ;
    uint64_t read_rate;
    uint32_t ccsm_strategy_nom_only = 0x000200FFu;
    uint32_t ccsm_strategy_swfb = 0x000100FFu;
    struct ccsm_dev_ctx *ctx;
    fwk_id_t dev_id =
        FWK_ID_ELEMENT_INIT(FAKE_MODULE_ID, FAKE_CCSM_IDX_INDEXED_SWFB);

    ctx = &module_ctx.dev_ctx_table[FAKE_CCSM_IDX_INDEXED_SWFB];
    ctx->initialized = true;
    ctx->current_nominal_clock_rate_hz = 1800 * FWK_MHZ;
    ctx->current_fallback_clock_rate_hz = 1600;

    fwk_module_is_valid_element_id_ExpectAndReturn(dev_id, true);
    fwk_id_get_element_idx_ExpectAndReturn(dev_id, FAKE_CCSM_IDX_INDEXED_SWFB);

    ccsm_drv_set_pll_dynamic_settings_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    ccsm_drv_set_pll_dynamic_settings_ExpectAnyArgsAndReturn(FWK_SUCCESS);

    ccsm_drv_get_dm_configuration_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    ccsm_drv_get_dm_configuration_ReturnThruPtr_config(&ccsm_strategy_swfb);

    ccsm_drv_set_dm_configuration_ExpectAndReturn(
        (struct ccsm_reg *)ctx->config->base_address,
        ccsm_strategy_nom_only,
        FWK_SUCCESS);

    ccsm_drv_set_request_ExpectAndReturn(
        (struct ccsm_reg *)ctx->config->base_address,
        CCSM_REQUEST_TYPE_GO_DN,
        FWK_SUCCESS);
    ccsm_drv_get_clear_irq_status_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    ccsm_drv_clear_request_ExpectAndReturn(
        (struct ccsm_reg *)ctx->config->base_address, FWK_SUCCESS);

    ccsm_drv_set_request_ExpectAndReturn(
        (struct ccsm_reg *)ctx->config->base_address,
        CCSM_REQUEST_TYPE_GO_DN2,
        FWK_SUCCESS);
    ccsm_drv_get_clear_irq_status_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    ccsm_drv_clear_request_ExpectAndReturn(
        (struct ccsm_reg *)ctx->config->base_address, FWK_SUCCESS);

    status = ccsm_clock_set_rate(dev_id, rate, MOD_CLOCK_ROUND_MODE_NEAREST);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

    fwk_module_is_valid_element_id_ExpectAndReturn(dev_id, true);
    fwk_id_get_element_idx_ExpectAndReturn(dev_id, FAKE_CCSM_IDX_INDEXED_SWFB);

    status = ccsm_clock_get_rate(dev_id, &read_rate);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL(1500 * FWK_MHZ, read_rate);
    TEST_ASSERT_EQUAL(0, ctx->current_fallback_clock_rate_hz);
}

void test_function_ccsm_clock_set_rate_fail_stopped(void)
{
    int status;
    uint64_t rate = 1800 * FWK_MHZ;
    struct ccsm_dev_ctx *ctx;
    fwk_id_t dev_id =
        FWK_ID_ELEMENT_INIT(FAKE_MODULE_ID, FAKE_CCSM_IDX_CONTINUOUS);

    ctx = &module_ctx.dev_ctx_table[FAKE_CCSM_IDX_CONTINUOUS];
    ctx->current_state = MOD_CLOCK_STATE_STOPPED;

    fwk_module_is_valid_element_id_ExpectAndReturn(dev_id, true);
    fwk_id_get_element_idx_ExpectAndReturn(dev_id, FAKE_CCSM_IDX_CONTINUOUS);

    status = ccsm_clock_set_rate(dev_id, rate, MOD_CLOCK_ROUND_MODE_NEAREST);
    TEST_ASSERT_EQUAL(FWK_E_PWRSTATE, status);
}

void test_function_ccsm_clock_set_rate_pass_same_rate(void)
{
    int status;
    uint64_t rate;
    uint64_t read_rate;
    struct ccsm_dev_ctx *ctx;
    fwk_id_t dev_id =
        FWK_ID_ELEMENT_INIT(FAKE_MODULE_ID, FAKE_CCSM_IDX_CONTINUOUS);

    ctx = &module_ctx.dev_ctx_table[FAKE_CCSM_IDX_CONTINUOUS];
    rate = ctx->config->default_rates_table->nominal_clock_rate_hz;

    fwk_module_is_valid_element_id_ExpectAndReturn(dev_id, true);
    fwk_id_get_element_idx_ExpectAndReturn(dev_id, FAKE_CCSM_IDX_CONTINUOUS);

    status = ccsm_clock_set_rate(dev_id, rate, MOD_CLOCK_ROUND_MODE_NEAREST);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

    fwk_module_is_valid_element_id_ExpectAndReturn(dev_id, true);
    fwk_id_get_element_idx_ExpectAndReturn(dev_id, FAKE_CCSM_IDX_CONTINUOUS);

    status = ccsm_clock_get_rate(dev_id, &read_rate);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL(rate, read_rate);
}

void test_function_ccsm_clock_get_rate_pass(void)
{
    int status;
    uint64_t rate;
    struct ccsm_dev_ctx *ctx;

    fwk_id_t dev_id =
        FWK_ID_ELEMENT_INIT(FAKE_MODULE_ID, FAKE_CCSM_IDX_CONTINUOUS);

    ctx = &module_ctx.dev_ctx_table[FAKE_CCSM_IDX_CONTINUOUS];

    fwk_module_is_valid_element_id_ExpectAndReturn(dev_id, true);
    fwk_id_get_element_idx_ExpectAndReturn(dev_id, FAKE_CCSM_IDX_CONTINUOUS);

    status = ccsm_clock_get_rate(dev_id, &rate);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL(
        ctx->config->default_rates_table->nominal_clock_rate_hz, rate);
}

void test_function_ccsm_clock_get_rate_from_index_pass(void)
{
    int status;
    uint64_t rate;
    fwk_id_t dev_id =
        FWK_ID_ELEMENT_INIT(FAKE_MODULE_ID, FAKE_CCSM_IDX_INDEXED);

    fwk_module_is_valid_element_id_ExpectAndReturn(dev_id, true);
    fwk_id_get_element_idx_ExpectAndReturn(dev_id, FAKE_CCSM_IDX_INDEXED);

    status = ccsm_clock_get_rate_from_index(dev_id, 0, &rate);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL(300 * FWK_MHZ, rate);
}

void test_function_ccsm_clock_get_rate_from_index_fail_rate_null(void)
{
    int status;
    uint64_t *rate = NULL;
    fwk_id_t dev_id =
        FWK_ID_ELEMENT_INIT(FAKE_MODULE_ID, FAKE_CCSM_IDX_INDEXED);

    fwk_module_is_valid_element_id_ExpectAndReturn(dev_id, true);

    status = ccsm_clock_get_rate_from_index(dev_id, 0, rate);
    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
}

void test_function_ccsm_clock_get_rate_from_index_fail_count_exceeded(void)
{
    int status;
    uint64_t rate;
    fwk_id_t dev_id =
        FWK_ID_ELEMENT_INIT(FAKE_MODULE_ID, FAKE_CCSM_IDX_INDEXED);

    fwk_module_is_valid_element_id_ExpectAndReturn(dev_id, true);
    fwk_id_get_element_idx_ExpectAndReturn(dev_id, FAKE_CCSM_IDX_INDEXED);

    status = ccsm_clock_get_rate_from_index(dev_id, 4, &rate);
    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
}
void test_function_ccsm_clock_get_rate_from_index_fail_continuous(void)
{
    int status;
    uint64_t rate;
    fwk_id_t dev_id =
        FWK_ID_ELEMENT_INIT(FAKE_MODULE_ID, FAKE_CCSM_IDX_CONTINUOUS);

    fwk_module_is_valid_element_id_ExpectAndReturn(dev_id, true);
    fwk_id_get_element_idx_ExpectAndReturn(dev_id, FAKE_CCSM_IDX_CONTINUOUS);

    status = ccsm_clock_get_rate_from_index(dev_id, 0, &rate);
    TEST_ASSERT_EQUAL(FWK_E_SUPPORT, status);
}

void test_function_ccsm_dm_get_configuration_pass(void)
{
    int status;
    uint32_t ccsm_strategy_in;
    struct mod_ccsm_dm_config dm_config = { 0 };
    struct ccsm_dev_ctx *ctx;

    ccsm_strategy_in = 0x11011FFFu;

    ctx = &module_ctx.dev_ctx_table[FAKE_CCSM_IDX_CONTINUOUS];

    ccsm_drv_get_dm_configuration_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    ccsm_drv_get_dm_configuration_ReturnThruPtr_config(&ccsm_strategy_in);

    status = ccsm_dm_get_configuration(ctx, &dm_config);

    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);

    TEST_ASSERT_EQUAL(ccsm_strategy_in, 0x11011FFFu);
    TEST_ASSERT_EQUAL(MOD_CCSM_DM_SW_FB, dm_config.strategy);
    TEST_ASSERT_EQUAL(MOD_CCSM_DM_TPIP, dm_config.dd);
    TEST_ASSERT_EQUAL(MOD_CCSM_DM_SW_SOFF_STOP, dm_config.soff);
    TEST_ASSERT_EQUAL(15, dm_config.transition_pause);
    TEST_ASSERT_EQUAL(511, dm_config.mitigation_duration);

    ccsm_strategy_in = 0x11021FFFu;

    ccsm_drv_get_dm_configuration_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    ccsm_drv_get_dm_configuration_ReturnThruPtr_config(&ccsm_strategy_in);

    status = ccsm_dm_get_configuration(ctx, &dm_config);

    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);

    TEST_ASSERT_EQUAL(MOD_CCSM_DM_NOM_ONLY, dm_config.strategy);
    TEST_ASSERT_EQUAL(MOD_CCSM_DM_TPIP, dm_config.dd);
    TEST_ASSERT_EQUAL(MOD_CCSM_DM_SW_SOFF_STOP, dm_config.soff);
    TEST_ASSERT_EQUAL(15, dm_config.transition_pause);
    TEST_ASSERT_EQUAL(511, dm_config.mitigation_duration);

    ccsm_strategy_in = 0x01001FFFu;

    ccsm_drv_get_dm_configuration_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    ccsm_drv_get_dm_configuration_ReturnThruPtr_config(&ccsm_strategy_in);

    status = ccsm_dm_get_configuration(ctx, &dm_config);

    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);

    TEST_ASSERT_EQUAL(MOD_CCSM_DM_STOP_CLK, dm_config.strategy);
    TEST_ASSERT_EQUAL(MOD_CCSM_DM_ARM_DD, dm_config.dd);
    TEST_ASSERT_EQUAL(MOD_CCSM_DM_SW_SOFF_STOP, dm_config.soff);
    TEST_ASSERT_EQUAL(15, dm_config.transition_pause);
    TEST_ASSERT_EQUAL(511, dm_config.mitigation_duration);

    ccsm_strategy_in = 0x00101FFFu;

    ccsm_drv_get_dm_configuration_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    ccsm_drv_get_dm_configuration_ReturnThruPtr_config(&ccsm_strategy_in);

    status = ccsm_dm_get_configuration(ctx, &dm_config);

    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);

    TEST_ASSERT_EQUAL(MOD_CCSM_DM_CLK_ON_DVFS, dm_config.strategy);
    TEST_ASSERT_EQUAL(MOD_CCSM_DM_ARM_DD, dm_config.dd);
    TEST_ASSERT_EQUAL(MOD_CCSM_DM_SW_SOFF_IGNORE, dm_config.soff);
    TEST_ASSERT_EQUAL(15, dm_config.transition_pause);
    TEST_ASSERT_EQUAL(511, dm_config.mitigation_duration);
}

void test_function_ccsm_dm_set_configuration_pass(void)
{
    int status;

    struct ccsm_dev_ctx *ctx;

    struct mod_ccsm_dm_config dm_config = { .strategy = MOD_CCSM_DM_NOM_ONLY,
                                            .dd = MOD_CCSM_DM_ARM_DD,
                                            .soff = MOD_CCSM_DM_SW_SOFF_IGNORE,
                                            .transition_pause = 6,
                                            .mitigation_duration = 705 };

    ctx = &module_ctx.dev_ctx_table[FAKE_CCSM_IDX_CONTINUOUS];

    ccsm_drv_set_dm_configuration_ExpectAndReturn(
        (struct ccsm_reg *)ctx->config->base_address, 0x00022C16, FWK_SUCCESS);

    status = ccsm_dm_set_configuration(ctx, &dm_config);
    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
}

void test_function_ccsm_mod_set_configuration_pass(void)
{
    int status;
    struct ccsm_dev_ctx *ctx;
    struct mod_ccsm_mod_config mod_config = { .numerator_regular = 240,
                                              .numerator_oc = 31,
                                              .denominator = 255 };

    ctx = &module_ctx.dev_ctx_table[FAKE_CCSM_IDX_CONTINUOUS];

    ccsm_drv_set_mod_configuration_ExpectAndReturn(
        (struct ccsm_reg *)ctx->config->base_address,
        0,
        0x001FFFF0,
        FWK_SUCCESS);

    status = ccsm_mod_set_configuration(ctx, 0, &mod_config);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void test_function_ccsm_mod_set_configuration_fail(void)
{
    int status;
    struct ccsm_dev_ctx *ctx;
    struct mod_ccsm_mod_config mod_config = { .numerator_regular = 255,
                                              .numerator_oc = 31,
                                              .denominator = 100 };

    ctx = &module_ctx.dev_ctx_table[FAKE_CCSM_IDX_CONTINUOUS];

    status = ccsm_mod_set_configuration(ctx, 0, &mod_config);
    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
}

void test_function_ccsm_clock_set_state_pass(void)
{
    int status;
    struct ccsm_dev_ctx *ctx;
    fwk_id_t dev_id =
        FWK_ID_ELEMENT_INIT(FAKE_MODULE_ID, FAKE_CCSM_IDX_CONTINUOUS);

    ctx = &module_ctx.dev_ctx_table[FAKE_CCSM_IDX_CONTINUOUS];

    status = ccsm_clock_set_state(dev_id, MOD_CLOCK_STATE_RUNNING);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL(MOD_CLOCK_STATE_RUNNING, ctx->current_state);
}

void test_function_ccsm_clock_set_state_fail(void)
{
    int status;
    struct ccsm_dev_ctx *ctx;
    fwk_id_t dev_id =
        FWK_ID_ELEMENT_INIT(FAKE_MODULE_ID, FAKE_CCSM_IDX_CONTINUOUS);

    ctx = &module_ctx.dev_ctx_table[FAKE_CCSM_IDX_CONTINUOUS];

    status = ccsm_clock_set_state(dev_id, MOD_CLOCK_STATE_STOPPED);
    TEST_ASSERT_EQUAL(FWK_E_SUPPORT, status);
    TEST_ASSERT_EQUAL(MOD_CLOCK_STATE_RUNNING, ctx->current_state);
}

void test_function_ccsm_clock_get_state_pass(void)
{
    int status;
    enum mod_clock_state cur_state;
    fwk_id_t dev_id =
        FWK_ID_ELEMENT_INIT(FAKE_MODULE_ID, FAKE_CCSM_IDX_CONTINUOUS);

    fwk_module_is_valid_element_id_ExpectAndReturn(dev_id, true);
    fwk_id_get_element_idx_ExpectAndReturn(dev_id, FAKE_CCSM_IDX_CONTINUOUS);

    status = ccsm_clock_get_state(dev_id, &cur_state);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL(MOD_CLOCK_STATE_RUNNING, cur_state);
}

void test_function_ccsm_clock_get_state_fail(void)
{
    int status;
    enum mod_clock_state *cur_state = NULL;
    fwk_id_t dev_id =
        FWK_ID_ELEMENT_INIT(FAKE_MODULE_ID, FAKE_CCSM_IDX_CONTINUOUS);

    fwk_module_is_valid_element_id_ExpectAndReturn(dev_id, true);

    status = ccsm_clock_get_state(dev_id, cur_state);
    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
}

void test_function_ccsm_clock_get_range_continuous_pass(void)
{
    int status;
    struct mod_clock_range range;
    struct ccsm_dev_ctx *ctx;
    fwk_id_t dev_id =
        FWK_ID_ELEMENT_INIT(FAKE_MODULE_ID, FAKE_CCSM_IDX_CONTINUOUS);

    ctx = &module_ctx.dev_ctx_table[FAKE_CCSM_IDX_CONTINUOUS];

    fwk_module_is_valid_element_id_ExpectAndReturn(dev_id, true);
    fwk_id_get_element_idx_ExpectAndReturn(dev_id, FAKE_CCSM_IDX_CONTINUOUS);

    status = ccsm_clock_get_range(dev_id, &range);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL(MOD_CLOCK_RATE_TYPE_CONTINUOUS, range.rate_type);
    TEST_ASSERT_EQUAL(ctx->config->min_clock_rate_hz, range.min);
    TEST_ASSERT_EQUAL(ctx->config->max_clock_rate_hz, range.max);
    TEST_ASSERT_EQUAL(ctx->config->clock_rate_step_hz, range.step);
}

void test_function_ccsm_clock_get_range_fail_null(void)
{
    int status;
    struct mod_clock_range *range = NULL;
    fwk_id_t dev_id =
        FWK_ID_ELEMENT_INIT(FAKE_MODULE_ID, FAKE_CCSM_IDX_CONTINUOUS);

    fwk_module_is_valid_element_id_ExpectAndReturn(dev_id, true);

    status = ccsm_clock_get_range(dev_id, range);
    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
}

void test_function_ccsm_clock_get_range_indexed_pass(void)
{
    int status;
    struct mod_clock_range range;
    struct ccsm_dev_ctx *ctx;
    fwk_id_t dev_id =
        FWK_ID_ELEMENT_INIT(FAKE_MODULE_ID, FAKE_CCSM_IDX_INDEXED);

    ctx = &module_ctx.dev_ctx_table[FAKE_CCSM_IDX_INDEXED];

    fwk_module_is_valid_element_id_ExpectAndReturn(dev_id, true);
    fwk_id_get_element_idx_ExpectAndReturn(dev_id, FAKE_CCSM_IDX_INDEXED);

    status = ccsm_clock_get_range(dev_id, &range);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL(MOD_CLOCK_RATE_TYPE_DISCRETE, range.rate_type);
    TEST_ASSERT_EQUAL(
        ctx->config->rate_table[0].nominal_clock_rate_hz, range.min);
    TEST_ASSERT_EQUAL(
        ctx->config->rate_table[ctx->config->rate_count - 1]
            .nominal_clock_rate_hz,
        range.max);
    TEST_ASSERT_EQUAL(ctx->config->rate_count, range.rate_count);
}

void test_function_ccsm_power_state_change_off_pass(void)
{
    int status;
    fwk_id_t dev_id =
        FWK_ID_ELEMENT_INIT(FAKE_MODULE_ID, FAKE_CCSM_IDX_CONTINUOUS);

    fwk_id_get_element_idx_ExpectAndReturn(dev_id, FAKE_CCSM_IDX_CONTINUOUS);

    status = ccsm_clock_power_state_change(dev_id, MOD_PD_STATE_OFF);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void test_function_ccsm_power_state_change_on_pass(void)
{
    int status;
    uint64_t rate = 1900 * FWK_MHZ;
    uint64_t read_rate;
    struct ccsm_dev_ctx *ctx;
    fwk_id_t dev_id =
        FWK_ID_ELEMENT_INIT(FAKE_MODULE_ID, FAKE_CCSM_IDX_CONTINUOUS);

    ctx = &module_ctx.dev_ctx_table[FAKE_CCSM_IDX_CONTINUOUS];
    ctx->initialized = true;
    ctx->current_nominal_clock_rate_hz = rate;

    fwk_id_get_element_idx_ExpectAndReturn(dev_id, FAKE_CCSM_IDX_CONTINUOUS);

    ccsm_drv_set_mod_configuration_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    ccsm_drv_get_dm_configuration_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    ccsm_drv_set_dm_configuration_ExpectAnyArgsAndReturn(FWK_SUCCESS);

    ccsm_drv_set_request_ExpectAndReturn(
        (struct ccsm_reg *)ctx->config->base_address,
        CCSM_REQUEST_TYPE_SET_DM,
        FWK_SUCCESS);
    ccsm_drv_get_clear_irq_status_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    ccsm_drv_clear_request_ExpectAndReturn(
        (struct ccsm_reg *)ctx->config->base_address, FWK_SUCCESS);

    ccsm_drv_set_request_ExpectAndReturn(
        (struct ccsm_reg *)ctx->config->base_address,
        CCSM_REQUEST_TYPE_SET_MOD,
        FWK_SUCCESS);
    ccsm_drv_get_clear_irq_status_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    ccsm_drv_clear_request_ExpectAndReturn(
        (struct ccsm_reg *)ctx->config->base_address, FWK_SUCCESS);

    ccsm_drv_set_pll_static_settings_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    ccsm_drv_set_pll_static_settings_ExpectAnyArgsAndReturn(FWK_SUCCESS);

    ccsm_drv_set_pll_dynamic_settings_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    ccsm_drv_set_pll_dynamic_settings_ExpectAnyArgsAndReturn(FWK_SUCCESS);

    ccsm_drv_set_request_ExpectAndReturn(
        (struct ccsm_reg *)ctx->config->base_address,
        CCSM_REQUEST_TYPE_SET_DVFS,
        FWK_SUCCESS);
    ccsm_drv_get_clear_irq_status_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    ccsm_drv_clear_request_ExpectAndReturn(
        (struct ccsm_reg *)ctx->config->base_address, FWK_SUCCESS);

    status = ccsm_clock_power_state_change(dev_id, MOD_PD_STATE_ON);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

    fwk_module_is_valid_element_id_ExpectAndReturn(dev_id, true);
    fwk_id_get_element_idx_ExpectAndReturn(dev_id, FAKE_CCSM_IDX_CONTINUOUS);

    status = ccsm_clock_get_rate(dev_id, &read_rate);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL(rate, read_rate);
}

void test_function_ccsm_clock_power_state_pending_change_pass(void)
{
    int status;
    struct ccsm_dev_ctx *ctx;
    fwk_id_t dev_id =
        FWK_ID_ELEMENT_INIT(FAKE_MODULE_ID, FAKE_CCSM_IDX_CONTINUOUS);

    ctx = &module_ctx.dev_ctx_table[FAKE_CCSM_IDX_CONTINUOUS];

    fwk_module_is_valid_element_id_ExpectAndReturn(dev_id, true);
    fwk_id_get_element_idx_ExpectAndReturn(dev_id, FAKE_CCSM_IDX_CONTINUOUS);

    status = ccsm_clock_power_state_pending_change(dev_id, 0, MOD_PD_STATE_OFF);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL(MOD_CLOCK_STATE_STOPPED, ctx->current_state);
}

void test_function_ccsm_init(void)
{
    int status;

    status = ccsm_init(FWK_ID_MODULE(FWK_MODULE_IDX_CCSM), 0, NULL);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

    status = ccsm_init(
        FWK_ID_MODULE(FWK_MODULE_IDX_CCSM), FAKE_CCSM_IDX_COUNT, NULL);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void test_function_ccsm_element_init(void)
{
    int status;
    struct ccsm_dev_ctx *ctx;
    fwk_id_t dev_id =
        FWK_ID_ELEMENT_INIT(FAKE_MODULE_ID, FAKE_CCSM_IDX_INDEXED);

    ctx = &module_ctx.dev_ctx_table[FAKE_CCSM_IDX_INDEXED];

    fwk_id_get_element_idx_ExpectAndReturn(dev_id, FAKE_CCSM_IDX_INDEXED);

    status =
        ccsm_element_init(dev_id, 0, element_table[FAKE_CCSM_IDX_INDEXED].data);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

    TEST_ASSERT_EQUAL(ctx->initialized, false);
    TEST_ASSERT_EQUAL(ctx->current_state, MOD_CLOCK_STATE_STOPPED);

    dev_id = FWK_ID_ELEMENT(FAKE_MODULE_ID, FAKE_CCSM_IDX_COUNT);

    fwk_id_get_element_idx_ExpectAndReturn(dev_id, FAKE_CCSM_IDX_COUNT);

    status =
        ccsm_element_init(dev_id, 0, element_table[FAKE_CCSM_IDX_INDEXED].data);
    TEST_ASSERT_EQUAL(FWK_E_DATA, status);
}

void test_function_ccsm_process_bind_request(void)
{
    int status;
    struct mod_clock_drv_api *clock_api;
    fwk_id_t req_id =
        FWK_ID_ELEMENT_INIT(FAKE_MODULE_ID, FAKE_CCSM_IDX_INDEXED);
    fwk_id_t element_id =
        FWK_ID_ELEMENT_INIT(FAKE_MODULE_ID, FAKE_CCSM_IDX_CONTINUOUS);
    fwk_id_t api_type = FWK_ID_API(FAKE_MODULE_ID, MOD_CCSM_CLOCK_API);

    fwk_id_get_api_idx_ExpectAndReturn(api_type, MOD_CCSM_CLOCK_API);

    status = ccsm_process_bind_request(
        req_id, element_id, api_type, (const void **)&clock_api);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL_PTR(&api_ccsm_clock, clock_api);
}

void test_function_ccsm_bind(void)
{
    int status;
    fwk_id_t req_id =
        FWK_ID_ELEMENT_INIT(FAKE_MODULE_ID, FAKE_CCSM_IDX_CONTINUOUS);

    fwk_id_is_type_ExpectAndReturn(req_id, FWK_ID_TYPE_ELEMENT, true);
    fwk_id_get_element_idx_ExpectAndReturn(req_id, FAKE_CCSM_IDX_INDEXED);
    fwk_module_bind_ExpectAnyArgsAndReturn(FWK_SUCCESS);

    status = ccsm_bind(req_id, 1);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void test_function_get_register_value_rate_fail(void)
{
    int status;
    struct ccsm_dev_ctx *ctx;
    uint32_t pll_settings;

    ctx = &module_ctx.dev_ctx_table[FAKE_CCSM_IDX_CONTINUOUS];

    status = get_register_value_rate(NULL, 0, &pll_settings, &pll_settings);

    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);

    status = get_register_value_rate(ctx, 0, NULL, &pll_settings);

    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);

    status = get_register_value_rate(ctx, 0, &pll_settings, NULL);

    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
}

int ccsm_test_main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_function_ccsm_init);
    RUN_TEST(test_function_ccsm_element_init);
    RUN_TEST(test_function_ccsm_process_bind_request);
    RUN_TEST(test_function_ccsm_bind);

    RUN_TEST(test_function_ccsm_clock_select_rates_pass_continuous_in_range);
    RUN_TEST(test_function_ccsm_clock_select_rates_pass_continuous_max);
    RUN_TEST(test_function_ccsm_clock_select_rates_pass_continuous_min);
    RUN_TEST(test_function_ccsm_clock_select_rates_fail_continuous_low);
    RUN_TEST(test_function_ccsm_clock_select_rates_fail_continuous_high);

    RUN_TEST(test_function_ccsm_clock_select_rates_pass_indexed_in_range);
    RUN_TEST(test_function_ccsm_clock_select_rates_pass_indexed_max);
    RUN_TEST(test_function_ccsm_clock_select_rates_pass_indexed_min);
    RUN_TEST(test_function_ccsm_clock_select_rates_fail_indexed_low);
    RUN_TEST(test_function_ccsm_clock_select_rates_fail_indexed_high);

    RUN_TEST(test_function_ccsm_clock_select_rates_fail_round_none);

    RUN_TEST(test_function_ccsm_clock_set_rate_pass_indexed_uninitialized);
    RUN_TEST(test_function_ccsm_clock_set_rate_pass_indexed_initialized);

    RUN_TEST(test_function_ccsm_clock_set_rate_pass_continuous_uninitialized);
    RUN_TEST(test_function_ccsm_clock_set_rate_pass_continuous_initialized);
    RUN_TEST(test_function_ccsm_clock_set_rate_pass_continuous_initialized_dn);

    RUN_TEST(test_function_ccsm_clock_set_rate_pass_swfb_up);
    RUN_TEST(test_function_ccsm_clock_set_rate_pass_swfb_down);

    RUN_TEST(test_function_ccsm_clock_set_rate_pass_same_rate);
    RUN_TEST(test_function_ccsm_clock_set_rate_fail_stopped);

    RUN_TEST(test_function_ccsm_clock_get_rate_pass);

    RUN_TEST(test_function_ccsm_clock_get_rate_from_index_pass);
    RUN_TEST(test_function_ccsm_clock_get_rate_from_index_fail_count_exceeded);
    RUN_TEST(test_function_ccsm_clock_get_rate_from_index_fail_rate_null);
    RUN_TEST(test_function_ccsm_clock_get_rate_from_index_fail_continuous);

    RUN_TEST(test_function_ccsm_dm_get_configuration_pass);
    RUN_TEST(test_function_ccsm_dm_set_configuration_pass);

    RUN_TEST(test_function_ccsm_mod_set_configuration_pass);
    RUN_TEST(test_function_ccsm_mod_set_configuration_fail);

    RUN_TEST(test_function_ccsm_clock_set_state_pass);
    RUN_TEST(test_function_ccsm_clock_set_state_fail);
    RUN_TEST(test_function_ccsm_clock_get_state_pass);
    RUN_TEST(test_function_ccsm_clock_get_state_fail);

    RUN_TEST(test_function_ccsm_clock_get_range_continuous_pass);
    RUN_TEST(test_function_ccsm_clock_get_range_fail_null);
    RUN_TEST(test_function_ccsm_clock_get_range_indexed_pass);

    RUN_TEST(test_function_ccsm_power_state_change_off_pass);
    RUN_TEST(test_function_ccsm_power_state_change_on_pass);

    RUN_TEST(test_function_ccsm_clock_power_state_pending_change_pass);

    RUN_TEST(test_function_get_register_value_rate_fail);

    return UNITY_END();
}

#if !defined(TEST_ON_TARGET)
int main(void)
{
    return ccsm_test_main();
}
#endif
