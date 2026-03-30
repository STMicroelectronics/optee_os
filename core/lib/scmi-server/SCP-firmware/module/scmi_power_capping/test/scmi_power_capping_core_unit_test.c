/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "scp_unity.h"
#include "unity.h"

#include <Mockfwk_mm.h>
#include <Mockfwk_module.h>
#include <Mockfwk_notification.h>
#include <Mockmod_power_capping_extra.h>
#include <internal/Mockfwk_core_internal.h>

#include <mod_scmi_power_capping_unit_test.h>

#include <stdarg.h>
#include <string.h>

#include UNIT_TEST_SRC

const struct mod_scmi_power_capping_domain_config valid_config = {
    .min_power_cap = 10u,
    .max_power_cap = 100u,
    .power_cap_step = 1u,
};

struct mod_scmi_power_capping_domain_context
    test_domain_ctx_table[FAKE_POWER_CAPPING_IDX_COUNT];

const struct mod_power_capping_api power_capping_api = {
    .get_applied_cap = get_applied_cap,
    .request_cap = request_cap,
    .get_average_power = get_average_power,
    .get_averaging_interval = get_averaging_interval,
    .get_averaging_interval_range = get_averaging_interval_range,
    .get_averaging_interval_step = get_averaging_interval_step,
    .set_averaging_interval = set_averaging_interval,
    .set_power_thresholds = set_power_thresholds,

};

struct fwk_event expected_notification_event;

void test_init(void)
{
    pcapping_core_ctx.power_capping_api = &power_capping_api;
}

void setUp(void)
{
    pcapping_core_ctx.power_capping_domain_count = FAKE_POWER_CAPPING_IDX_COUNT;
    pcapping_core_ctx.power_capping_domain_ctx_table = test_domain_ctx_table;

    for (unsigned int i = 0u; i < FAKE_POWER_CAPPING_IDX_COUNT; i++) {
        pcapping_core_ctx.power_capping_domain_ctx_table[i] =
            (struct mod_scmi_power_capping_domain_context){ 0 };
    }
}

void tearDown(void)
{
    Mockmod_power_capping_extra_Verify();
    Mockmod_power_capping_extra_Destroy();
}

void utest_pcapping_core_get_domain_ctx_idx_out_of_range(void)
{
    unsigned int domain_idx = pcapping_core_ctx.power_capping_domain_count;
    int status = pcapping_core_get_domain_ctx(domain_idx, NULL);

    TEST_ASSERT_EQUAL(status, FWK_E_RANGE);
}

void utest_pcapping_core_get_domain_ctx_null_ptr(void)
{
    unsigned int domain_idx = pcapping_core_ctx.power_capping_domain_count - 1u;
    int status = pcapping_core_get_domain_ctx(domain_idx, NULL);

    TEST_ASSERT_EQUAL(status, FWK_E_PARAM);
}

void utest_pcapping_core_get_domain_ctx_success(void)
{
    struct mod_scmi_power_capping_domain_context *ctx;
    unsigned int domain_idx = pcapping_core_ctx.power_capping_domain_count - 1u;

    int status = pcapping_core_get_domain_ctx(domain_idx, &ctx);

    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
    TEST_ASSERT_EQUAL_PTR(
        ctx, &pcapping_core_ctx.power_capping_domain_ctx_table[domain_idx]);
}

void utest_pcapping_core_check_domain_configuration_min_cap_0(void)
{
    int status;

    struct mod_scmi_power_capping_domain_config config = valid_config;
    config.min_power_cap = 0u,

    status = pcapping_core_check_domain_configuration(&config);
    TEST_ASSERT_EQUAL(status, FWK_E_DATA);
}

void utest_pcapping_core_check_domain_configuration_max_cap_0(void)
{
    int status;

    struct mod_scmi_power_capping_domain_config config = valid_config;
    config.max_power_cap = 0u;

    status = pcapping_core_check_domain_configuration(&config);
    TEST_ASSERT_EQUAL(status, FWK_E_DATA);
}

void utest_pcapping_core_check_domain_configuration_invalid_cap_step_zero(void)
{
    int status;

    struct mod_scmi_power_capping_domain_config config = valid_config;
    config.power_cap_step = 0u;

    status = pcapping_core_check_domain_configuration(&config);
    TEST_ASSERT_EQUAL(status, FWK_E_DATA);
}

void utest_pcapping_core_check_domain_configuration_valid_cap_step_zero(void)
{
    int status;

    struct mod_scmi_power_capping_domain_config config = {
        .min_power_cap = 10u,
        .max_power_cap = 10u,
        .power_cap_step = 0u,
    };

    status = pcapping_core_check_domain_configuration(&config);
    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
}

void utest_pcapping_core_check_domain_configuration_success(void)
{
    int status;

    status = pcapping_core_check_domain_configuration(&valid_config);
    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
}

void utest_pcapping_core_bind(void)
{
    int status;

    fwk_module_bind_ExpectAndReturn(
        FWK_ID_MODULE(FWK_MODULE_IDX_POWER_CAPPING),
        FWK_ID_API(FWK_MODULE_IDX_POWER_CAPPING, MOD_POWER_CAPPING_API_IDX_CAP),
        &(pcapping_core_ctx.power_capping_api),
        FWK_SUCCESS);

    status = pcapping_core_bind();

    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
}

void utest_pcapping_core_init(void)
{
    unsigned int element_count = FAKE_POWER_CAPPING_IDX_COUNT;

    struct mod_scmi_power_capping_domain_context dummy_ctx;

    fwk_mm_calloc_ExpectAndReturn(
        element_count,
        sizeof(struct mod_scmi_power_capping_domain_context),
        &dummy_ctx);
    pcapping_core_init(element_count);

    TEST_ASSERT_EQUAL_PTR(
        &dummy_ctx, pcapping_core_ctx.power_capping_domain_ctx_table);
    TEST_ASSERT_EQUAL(
        element_count, pcapping_core_ctx.power_capping_domain_count);
}

void utest_pcapping_core_domain_init(void)
{
    int status;
    uint32_t domain_idx = FAKE_POWER_CAPPING_IDX_COUNT - 1u;
    fwk_id_t none_id = FWK_ID_NONE;
    (void)none_id;

    struct mod_scmi_power_capping_domain_context *ctx =
        &pcapping_core_ctx.power_capping_domain_ctx_table[domain_idx];

    status = pcapping_core_domain_init(domain_idx, &valid_config);

    TEST_ASSERT_EQUAL_PTR(ctx->config, &valid_config);
    TEST_ASSERT(fwk_id_is_equal(ctx->cap_pending_service_id, none_id));
    TEST_ASSERT(fwk_id_is_equal(ctx->cap_notification_service_id, none_id));
    TEST_ASSERT(fwk_id_is_equal(ctx->pai_notification_service_id, none_id));

    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
}

void utest_pcapping_core_domain_init_invalid_config(void)
{
    int status;
    uint32_t domain_idx = FAKE_POWER_CAPPING_IDX_COUNT - 1u;
    fwk_id_t none_id = FWK_ID_NONE;
    struct mod_scmi_power_capping_domain_config invalid_configs = {
        .min_power_cap = 9u,
        .max_power_cap = 100u,
        .power_cap_step = 2u,
    };
    (void)none_id;

    struct mod_scmi_power_capping_domain_context *ctx =
        &pcapping_core_ctx.power_capping_domain_ctx_table[domain_idx];

    status = pcapping_core_domain_init(domain_idx, &invalid_configs);

    TEST_ASSERT_EQUAL(FWK_E_DATA, status);
    TEST_ASSERT(ctx->config != &invalid_configs);
}

void utest_pcapping_core_start(void)
{
    int status;
    uint32_t domain_idx = FAKE_POWER_CAPPING_IDX_COUNT - 1u;
    struct mod_scmi_power_capping_domain_context *ctx =
        &pcapping_core_ctx.power_capping_domain_ctx_table[domain_idx];
    fwk_id_t power_capping_domain_id =
        FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_POWER_CAPPING, 1u);

    struct mod_scmi_power_capping_domain_config config = valid_config;
    config.power_capping_domain_id = power_capping_domain_id;

    ctx->config = &config;

    fwk_notification_subscribe_ExpectAndReturn(
        pcapping_core_cap_notification,
        power_capping_domain_id,
        FWK_ID_ELEMENT(FWK_MODULE_IDX_SCMI_POWER_CAPPING, domain_idx),
        FWK_SUCCESS);

#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
    fwk_notification_subscribe_ExpectAndReturn(
        pcapping_core_pai_notification,
        FWK_ID_MODULE(FWK_MODULE_IDX_POWER_CAPPING),
        FWK_ID_ELEMENT(FWK_MODULE_IDX_SCMI_POWER_CAPPING, domain_idx),
        FWK_SUCCESS);

    fwk_notification_subscribe_ExpectAndReturn(
        pcapping_core_power_measurements_notification,
        FWK_ID_MODULE(FWK_MODULE_IDX_POWER_CAPPING),
        FWK_ID_ELEMENT(FWK_MODULE_IDX_SCMI_POWER_CAPPING, domain_idx),
        FWK_SUCCESS);
#endif

    status = pcapping_core_start(domain_idx);

    TEST_ASSERT_EQUAL(ctx->cap_config_support, true);
    TEST_ASSERT_EQUAL(ctx->cap_config_support, true);

    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
}

void utest_pcapping_core_set_cap_error_no_cap_config_support(void)
{
    int status;

    unsigned int domain_idx = FAKE_POWER_CAPPING_IDX_COUNT - 1u;
    bool is_async = false;

    fwk_id_t service_id =
        FWK_ID_ELEMENT_INIT(FAKE_SCMI_MODULE_ID, FAKE_SERVICE_IDX_1);

    struct mod_scmi_power_capping_domain_context *ctx =
        &pcapping_core_ctx.power_capping_domain_ctx_table[domain_idx];

    ctx->config = &valid_config;

    uint32_t cap = ctx->config->min_power_cap + ctx->config->power_cap_step;

    ctx->cap_config_support = false;

    status = pcapping_core_set_cap(service_id, domain_idx, is_async, cap);

    TEST_ASSERT_EQUAL(status, FWK_E_SUPPORT);
}

void utest_pcapping_core_set_cap_error_cap_lower_than_min(void)
{
    int status;

    unsigned int domain_idx = FAKE_POWER_CAPPING_IDX_COUNT - 1u;
    bool is_async = false;

    fwk_id_t service_id =
        FWK_ID_ELEMENT_INIT(FAKE_SCMI_MODULE_ID, FAKE_SERVICE_IDX_1);

    struct mod_scmi_power_capping_domain_context *ctx =
        &pcapping_core_ctx.power_capping_domain_ctx_table[domain_idx];

    ctx->config = &valid_config;

    uint32_t cap = ctx->config->min_power_cap - (uint32_t)1;

    ctx->cap_config_support = true;

    status = pcapping_core_set_cap(service_id, domain_idx, is_async, cap);

    TEST_ASSERT_EQUAL(status, FWK_E_RANGE);
}

void utest_pcapping_core_set_cap_error_cap_greater_than_max(void)
{
    int status;

    unsigned int domain_idx = FAKE_POWER_CAPPING_IDX_COUNT - 1u;
    bool is_async = false;

    fwk_id_t service_id =
        FWK_ID_ELEMENT_INIT(FAKE_SCMI_MODULE_ID, FAKE_SERVICE_IDX_1);

    struct mod_scmi_power_capping_domain_context *ctx =
        &pcapping_core_ctx.power_capping_domain_ctx_table[domain_idx];

    ctx->config = &valid_config;

    uint32_t cap = ctx->config->max_power_cap + (uint32_t)1;

    ctx->cap_config_support = true;

    status = pcapping_core_set_cap(service_id, domain_idx, is_async, cap);

    TEST_ASSERT_EQUAL(status, FWK_E_RANGE);
}

void utest_pcapping_core_set_cap_error_cap_not_match_step(void)
{
    int status;

    unsigned int domain_idx = FAKE_POWER_CAPPING_IDX_COUNT - 1u;
    bool is_async = false;

    fwk_id_t service_id =
        FWK_ID_ELEMENT_INIT(FAKE_SCMI_MODULE_ID, FAKE_SERVICE_IDX_1);

    struct mod_scmi_power_capping_domain_context *ctx =
        &pcapping_core_ctx.power_capping_domain_ctx_table[domain_idx];

    struct mod_scmi_power_capping_domain_config config = valid_config;

    config.power_cap_step = (uint32_t)5;

    ctx->config = &config;

    uint32_t cap = ctx->config->min_power_cap + (uint32_t)1;

    ctx->cap_config_support = true;

    status = pcapping_core_set_cap(service_id, domain_idx, is_async, cap);

    TEST_ASSERT_EQUAL(status, FWK_E_RANGE);
}

void utest_pcapping_core_set_cap_busy(void)
{
    int status;

    unsigned int domain_idx = FAKE_POWER_CAPPING_IDX_COUNT - 1u;
    bool is_async = false;

    fwk_id_t service_id =
        FWK_ID_ELEMENT_INIT(FAKE_SCMI_MODULE_ID, FAKE_SERVICE_IDX_1);

    struct mod_scmi_power_capping_domain_context *ctx =
        &pcapping_core_ctx.power_capping_domain_ctx_table[domain_idx];

    ctx->config = &valid_config;

    uint32_t cap = ctx->config->min_power_cap + (uint32_t)1;

    ctx->cap_config_support = true;

    ctx->cap_pending_service_id =
        FWK_ID_ELEMENT(FAKE_SCMI_MODULE_ID, FAKE_SERVICE_IDX_2);

    status = pcapping_core_set_cap(service_id, domain_idx, is_async, cap);

    TEST_ASSERT_EQUAL(status, FWK_E_BUSY);
}

#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
void utest_pcapping_core_set_cap_busy_notification(void)
{
    int status;

    unsigned int domain_idx = FAKE_POWER_CAPPING_IDX_COUNT - 1u;
    bool is_async = false;

    fwk_id_t service_id =
        FWK_ID_ELEMENT_INIT(FAKE_SCMI_MODULE_ID, FAKE_SERVICE_IDX_1);

    struct mod_scmi_power_capping_domain_context *ctx =
        &pcapping_core_ctx.power_capping_domain_ctx_table[domain_idx];

    struct mod_scmi_power_capping_domain_config config = valid_config;

    config.cap_pai_change_notification_support = true;

    ctx->config = &config;

    uint32_t cap = ctx->config->min_power_cap + (uint32_t)1;

    ctx->cap_pending_service_id = FWK_ID_NONE;
    ctx->cap_notification_service_id =
        FWK_ID_ELEMENT(FAKE_SCMI_MODULE_ID, FAKE_SERVICE_IDX_2);
    ctx->cap_config_support = true;

    status = pcapping_core_set_cap(service_id, domain_idx, is_async, cap);

    TEST_ASSERT_EQUAL(status, FWK_E_BUSY);
}
#endif

void utest_pcapping_core_set_cap_pending_sync_request(void)
{
    int status;

    unsigned int domain_idx = FAKE_POWER_CAPPING_IDX_COUNT - 1u;
    bool is_async = false;

    fwk_id_t service_id =
        FWK_ID_ELEMENT_INIT(FAKE_SCMI_MODULE_ID, FAKE_SERVICE_IDX_1);

    struct mod_scmi_power_capping_domain_context *ctx =
        &pcapping_core_ctx.power_capping_domain_ctx_table[domain_idx];

    struct mod_scmi_power_capping_domain_config config = valid_config;
    ctx->config = &config;

    config.power_capping_domain_id =
        FWK_ID_ELEMENT(FWK_MODULE_IDX_POWER_CAPPING, 0);

    uint32_t cap = ctx->config->min_power_cap + (uint32_t)1;

    ctx->cap_config_support = true;

    ctx->cap_pending_service_id = FWK_ID_NONE;
    ctx->cap_notification_service_id = FWK_ID_NONE;

    request_cap_ExpectAndReturn(
        config.power_capping_domain_id, cap, FWK_PENDING);

    status = pcapping_core_set_cap(service_id, domain_idx, is_async, cap);

    TEST_ASSERT_EQUAL(ctx->is_cap_request_async, false);
    TEST_ASSERT_EQUAL(status, FWK_PENDING);
}

void utest_pcapping_core_set_cap_pending_async_request(void)
{
    int status;

    unsigned int domain_idx = FAKE_POWER_CAPPING_IDX_COUNT - 1u;
    bool is_async = true;

    fwk_id_t service_id =
        FWK_ID_ELEMENT_INIT(FAKE_SCMI_MODULE_ID, FAKE_SERVICE_IDX_1);

    struct mod_scmi_power_capping_domain_context *ctx =
        &pcapping_core_ctx.power_capping_domain_ctx_table[domain_idx];

    struct mod_scmi_power_capping_domain_config config = valid_config;
    ctx->config = &config;

    config.power_capping_domain_id =
        FWK_ID_ELEMENT(FWK_MODULE_IDX_POWER_CAPPING, 0);

    uint32_t cap = ctx->config->min_power_cap + (uint32_t)1;

    ctx->cap_config_support = true;

    ctx->cap_pending_service_id = FWK_ID_NONE;
    ctx->cap_notification_service_id = FWK_ID_NONE;

    request_cap_ExpectAndReturn(
        config.power_capping_domain_id, cap, FWK_PENDING);

    status = pcapping_core_set_cap(service_id, domain_idx, is_async, cap);

    TEST_ASSERT_EQUAL(ctx->is_cap_request_async, true);
    TEST_ASSERT_EQUAL(status, FWK_PENDING);
}

void utest_pcapping_core_get_cap_out_of_range(void)
{
    int status;
    unsigned int domain_idx = FAKE_POWER_CAPPING_IDX_COUNT;
    uint32_t cap;

    status = pcapping_core_get_cap(domain_idx, &cap);

    TEST_ASSERT_EQUAL(status, FWK_E_RANGE);
}

void utest_pcapping_core_get_cap_success(void)
{
    int status;
    unsigned int domain_idx = FAKE_POWER_CAPPING_IDX_COUNT - 1u;
    uint32_t returned_cap = 50u;
    uint32_t expected_cap;

    struct mod_scmi_power_capping_domain_context *ctx =
        &pcapping_core_ctx.power_capping_domain_ctx_table[domain_idx];

    struct mod_scmi_power_capping_domain_config config = valid_config;
    config.power_capping_domain_id =
        FWK_ID_ELEMENT(FWK_MODULE_IDX_POWER_CAPPING, 0);

    ctx->config = &config;

    get_applied_cap_ExpectAndReturn(
        config.power_capping_domain_id, NULL, FWK_SUCCESS);
    get_applied_cap_IgnoreArg_cap();
    get_applied_cap_ReturnThruPtr_cap(&returned_cap);

    status = pcapping_core_get_cap(domain_idx, &expected_cap);

    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
    TEST_ASSERT_EQUAL(expected_cap, returned_cap);
}

void utest_pcapping_core_set_pai_error_pai_error(void)
{
    int status;

    unsigned int domain_idx = FAKE_POWER_CAPPING_IDX_COUNT - 1u;

    struct mod_scmi_power_capping_domain_context *ctx =
        &pcapping_core_ctx.power_capping_domain_ctx_table[domain_idx];

    struct mod_scmi_power_capping_domain_config config = valid_config;
    config.pai_config_support = true;

    ctx->config = &config;

    fwk_id_t service_id =
        FWK_ID_ELEMENT_INIT(FAKE_SCMI_MODULE_ID, FAKE_SERVICE_IDX_1);

    uint32_t pai = 1u;

    set_averaging_interval_ExpectAndReturn(
        config.power_capping_domain_id, pai, FWK_E_RANGE);

    status = pcapping_core_set_pai(service_id, domain_idx, pai);

    TEST_ASSERT_EQUAL(status, FWK_E_RANGE);
}

#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
void utest_pcapping_core_set_pai_busy(void)
{
    int status;

    unsigned int domain_idx = FAKE_POWER_CAPPING_IDX_COUNT - 1u;

    fwk_id_t service_id =
        FWK_ID_ELEMENT_INIT(FAKE_SCMI_MODULE_ID, FAKE_SERVICE_IDX_1);

    struct mod_scmi_power_capping_domain_context *ctx =
        &pcapping_core_ctx.power_capping_domain_ctx_table[domain_idx];

    struct mod_scmi_power_capping_domain_config config = valid_config;
    config.cap_pai_change_notification_support = true;
    config.pai_config_support = true;

    ctx->config = &config;

    uint32_t pai = (uint32_t)100;

    ctx->pai_notification_service_id =
        FWK_ID_ELEMENT(FAKE_SCMI_MODULE_ID, FAKE_SERVICE_IDX_2);

    status = pcapping_core_set_pai(service_id, domain_idx, pai);
    TEST_ASSERT_EQUAL(status, FWK_E_BUSY);
}
#endif

void utest_pcapping_core_set_pai_success(void)
{
    int status;

    unsigned int domain_idx = FAKE_POWER_CAPPING_IDX_COUNT - 1u;

    fwk_id_t service_id =
        FWK_ID_ELEMENT_INIT(FAKE_SCMI_MODULE_ID, FAKE_SERVICE_IDX_1);

    struct mod_scmi_power_capping_domain_context *ctx =
        &pcapping_core_ctx.power_capping_domain_ctx_table[domain_idx];

    struct mod_scmi_power_capping_domain_config config = valid_config;

    config.power_capping_domain_id =
        FWK_ID_ELEMENT(FWK_MODULE_IDX_POWER_CAPPING, 1);

    ctx->config = &config;

    uint32_t pai = (uint32_t)23;

    config.pai_config_support = true;

    set_averaging_interval_ExpectAndReturn(
        config.power_capping_domain_id, pai, FWK_SUCCESS);

    status = pcapping_core_set_pai(service_id, domain_idx, pai);

    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
}

#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
void utest_pcapping_core_set_pai_success_notification(void)
{
    int status;

    unsigned int domain_idx = FAKE_POWER_CAPPING_IDX_COUNT - 1u;

    fwk_id_t service_id =
        FWK_ID_ELEMENT_INIT(FAKE_SCMI_MODULE_ID, FAKE_SERVICE_IDX_1);

    struct mod_scmi_power_capping_domain_context *ctx =
        &pcapping_core_ctx.power_capping_domain_ctx_table[domain_idx];

    struct mod_scmi_power_capping_domain_config config = valid_config;

    config.power_capping_domain_id =
        FWK_ID_ELEMENT(FWK_MODULE_IDX_POWER_CAPPING, 1);
    config.cap_pai_change_notification_support = true;

    ctx->config = &config;
    config.pai_config_support = true;

    uint32_t pai = (uint32_t)10;

    ctx->pai_notification_service_id = FWK_ID_NONE;

    set_averaging_interval_ExpectAndReturn(
        config.power_capping_domain_id, pai, FWK_SUCCESS);

    status = pcapping_core_set_pai(service_id, domain_idx, pai);

    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
    TEST_ASSERT(fwk_id_is_equal(ctx->pai_notification_service_id, service_id));
}
#endif

void utest_pcapping_core_get_pai_out_of_range(void)
{
    int status;
    unsigned int domain_idx = FAKE_POWER_CAPPING_IDX_COUNT;
    uint32_t pai;

    status = pcapping_core_get_pai(domain_idx, &pai);

    TEST_ASSERT_EQUAL(status, FWK_E_RANGE);
}

void utest_pcapping_core_get_pai_success(void)
{
    int status;
    unsigned int domain_idx = FAKE_POWER_CAPPING_IDX_COUNT - 1u;
    uint32_t returned_pai = 60u;
    uint32_t expected_pai;

    struct mod_scmi_power_capping_domain_context *ctx =
        &pcapping_core_ctx.power_capping_domain_ctx_table[domain_idx];

    struct mod_scmi_power_capping_domain_config config = valid_config;
    config.power_capping_domain_id =
        FWK_ID_ELEMENT(FWK_MODULE_IDX_POWER_CAPPING, 0);

    ctx->config = &config;

    get_averaging_interval_ExpectAndReturn(
        config.power_capping_domain_id, NULL, FWK_SUCCESS);
    get_averaging_interval_IgnoreArg_pai();
    get_averaging_interval_ReturnThruPtr_pai(&returned_pai);

    status = pcapping_core_get_pai(domain_idx, &expected_pai);

    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
    TEST_ASSERT_EQUAL(expected_pai, returned_pai);
}

void utest_pcapping_core_get_power_out_of_range(void)
{
    int status;
    unsigned int domain_idx = FAKE_POWER_CAPPING_IDX_COUNT;
    uint32_t power;

    status = pcapping_core_get_power(domain_idx, &power);

    TEST_ASSERT_EQUAL(status, FWK_E_RANGE);
}

void utest_pcapping_core_get_power_success(void)
{
    int status;
    unsigned int domain_idx = FAKE_POWER_CAPPING_IDX_COUNT - 1u;
    uint32_t returned_power = 70u;
    uint32_t expected_power;

    struct mod_scmi_power_capping_domain_context *ctx =
        &pcapping_core_ctx.power_capping_domain_ctx_table[domain_idx];

    struct mod_scmi_power_capping_domain_config config = valid_config;
    config.power_capping_domain_id =
        FWK_ID_ELEMENT(FWK_MODULE_IDX_POWER_CAPPING, 0);

    ctx->config = &config;

    get_average_power_ExpectAndReturn(
        config.power_capping_domain_id, NULL, FWK_SUCCESS);
    get_average_power_IgnoreArg_power();
    get_average_power_ReturnThruPtr_power(&returned_power);

    status = pcapping_core_get_power(domain_idx, &expected_power);

    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
    TEST_ASSERT_EQUAL(expected_power, returned_power);
}

void utest_pcapping_core_set_power_thresholds_success(void)
{
    int status;
    uint32_t threshold_low = 20;
    uint32_t threshold_high = 50;
    unsigned int domain_idx = FAKE_POWER_CAPPING_IDX_COUNT - 1u;

    struct mod_scmi_power_capping_domain_context *ctx =
        &pcapping_core_ctx.power_capping_domain_ctx_table[domain_idx];

    struct mod_scmi_power_capping_domain_config config = valid_config;
    config.power_capping_domain_id =
        FWK_ID_ELEMENT(FWK_MODULE_IDX_POWER_CAPPING, 0);
    ctx->config = &config;

    set_power_thresholds_ExpectAndReturn(
        config.power_capping_domain_id,
        threshold_low,
        threshold_high,
        FWK_SUCCESS);

    status = pcapping_core_set_power_thresholds(
        domain_idx, threshold_low, threshold_high);

    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
}

void utest_pcapping_core_set_power_thresholds_invalid_index(void)
{
    int status;
    unsigned int invalid_idx = FAKE_POWER_CAPPING_IDX_COUNT;

    status = pcapping_core_set_power_thresholds(invalid_idx, 10, 20);

    TEST_ASSERT_EQUAL(status, FWK_E_RANGE);
}

void utest_pcapping_core_set_power_thresholds_hal_error(void)
{
    int status;
    uint32_t threshold_low = 20;
    uint32_t threshold_high = 10;
    unsigned int domain_idx = FAKE_POWER_CAPPING_IDX_COUNT - 1u;

    struct mod_scmi_power_capping_domain_context *ctx =
        &pcapping_core_ctx.power_capping_domain_ctx_table[domain_idx];

    struct mod_scmi_power_capping_domain_config config = valid_config;
    config.power_capping_domain_id =
        FWK_ID_ELEMENT(FWK_MODULE_IDX_POWER_CAPPING, 0);
    ctx->config = &config;

    set_power_thresholds_ExpectAndReturn(
        config.power_capping_domain_id,
        threshold_low,
        threshold_high,
        FWK_E_PARAM);

    status = pcapping_core_set_power_thresholds(
        domain_idx, threshold_low, threshold_high);

    TEST_ASSERT_EQUAL(status, FWK_E_PARAM);
}

int fwk_put_event_notifications_callback(struct fwk_event *event, int numCalls)
{
    TEST_ASSERT_EQUAL_MEMORY(
        event->params,
        expected_notification_event.params,
        FWK_EVENT_PARAMETERS_SIZE);

    return FWK_SUCCESS;
}

void utest_pcapping_core_process_cap_fwk_notification(void)
{
    int status;

    unsigned int domain_idx = FAKE_POWER_CAPPING_IDX_COUNT - 1u;
    struct mod_scmi_power_capping_domain_context *ctx =
        &pcapping_core_ctx.power_capping_domain_ctx_table[domain_idx];

    ctx->config = &valid_config;

    ctx->cap_pending_service_id =
        FWK_ID_ELEMENT(FAKE_SCMI_MODULE_ID, FAKE_SERVICE_IDX_2);

    struct pcapping_core_cap_pai_event_parameters *event_params =
        (struct pcapping_core_cap_pai_event_parameters *)
            expected_notification_event.params;

    event_params->domain_idx = domain_idx;

    event_params->service_id = ctx->cap_pending_service_id;

    __fwk_put_event_Stub(fwk_put_event_notifications_callback);

    status = pcapping_core_process_cap_fwk_notification(domain_idx, ctx);

    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
    TEST_ASSERT(fwk_id_is_equal(ctx->cap_pending_service_id, FWK_ID_NONE));
}

#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
void utest_pcapping_core_process_cap_fwk_notification_notify_cap_change(void)
{
    int status;

    uint32_t returned_cap = 17u;
    uint32_t returned_pai = 103u;

    unsigned int domain_idx = FAKE_POWER_CAPPING_IDX_COUNT - 1u;
    struct mod_scmi_power_capping_domain_context *ctx =
        &pcapping_core_ctx.power_capping_domain_ctx_table[domain_idx];

    struct mod_scmi_power_capping_domain_config config = valid_config;

    config.cap_pai_change_notification_support = true;

    config.power_capping_domain_id =
        FWK_ID_ELEMENT(FWK_MODULE_IDX_POWER_CAPPING, 2u);

    ctx->config = &config;

    ctx->is_cap_request_async = false;

    ctx->cap_pending_service_id = FWK_ID_NONE;

    ctx->cap_notification_service_id =
        FWK_ID_ELEMENT(FAKE_SCMI_MODULE_ID, FAKE_SERVICE_IDX_2);

    expected_notification_event = (struct fwk_event){ 0 };

    struct pcapping_core_cap_pai_event_parameters *expected_event_params =
        (struct pcapping_core_cap_pai_event_parameters *)
            expected_notification_event.params;

    expected_event_params->domain_idx = domain_idx;

    expected_event_params->service_id =
        FWK_ID_ELEMENT(FAKE_SCMI_MODULE_ID, FAKE_SERVICE_IDX_2);

    expected_event_params->pai = returned_pai;

    expected_event_params->cap = returned_cap;

    __fwk_put_event_Stub(fwk_put_event_notifications_callback);

    get_averaging_interval_ExpectAndReturn(
        config.power_capping_domain_id, NULL, FWK_SUCCESS);
    get_averaging_interval_IgnoreArg_pai();
    get_averaging_interval_ReturnThruPtr_pai(&returned_pai);

    get_applied_cap_ExpectAndReturn(
        config.power_capping_domain_id, NULL, FWK_SUCCESS);
    get_applied_cap_IgnoreArg_cap();
    get_applied_cap_ReturnThruPtr_cap(&returned_cap);

    status = pcapping_core_process_cap_fwk_notification(domain_idx, ctx);

    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
    TEST_ASSERT(fwk_id_is_equal(ctx->cap_notification_service_id, FWK_ID_NONE));
}

void utest_pcapping_core_process_power_measurements_fwk_notification(void)
{
    int status;

    unsigned int domain_idx = FAKE_POWER_CAPPING_IDX_COUNT - 1u;

    struct mod_scmi_power_capping_domain_context *ctx =
        &pcapping_core_ctx.power_capping_domain_ctx_table[domain_idx];

    struct mod_scmi_power_capping_domain_config config = valid_config;

    config.power_capping_domain_id =
        FWK_ID_ELEMENT(FWK_MODULE_IDX_POWER_CAPPING, 0);

    config.power_measurements_change_notification_support = true;

    ctx->config = &config;

    uint32_t returned_power = 70u;

    expected_notification_event = (struct fwk_event){ 0 };

    struct pcapping_core_pwr_meas_event_parameters *scmi_notif_event_params =
        (struct pcapping_core_pwr_meas_event_parameters *)
            expected_notification_event.params;

    expected_notification_event.target_id =
        FWK_ID_MODULE(FWK_MODULE_IDX_SCMI_POWER_CAPPING),
    expected_notification_event.id = FWK_ID_EVENT(
        FWK_MODULE_IDX_SCMI_POWER_CAPPING,
        SCMI_POWER_CAPPING_EVENT_IDX_PROCESS_HAL_MEASUREMENT_NOTIF);
    scmi_notif_event_params->service_id = FWK_ID_NONE;
    scmi_notif_event_params->domain_idx = domain_idx;
    scmi_notif_event_params->power = returned_power;

    get_average_power_ExpectAndReturn(
        config.power_capping_domain_id, NULL, FWK_SUCCESS);
    get_average_power_IgnoreArg_power();
    get_average_power_ReturnThruPtr_power(&returned_power);

    __fwk_put_event_Stub(fwk_put_event_notifications_callback);

    status = pcapping_core_process_power_measurements_fwk_notification(
        domain_idx, ctx);

    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
}
#endif

int scmi_test_main(void)
{
    UNITY_BEGIN();
    test_init();
    RUN_TEST(utest_pcapping_core_get_domain_ctx_idx_out_of_range);
    RUN_TEST(utest_pcapping_core_get_domain_ctx_null_ptr);
    RUN_TEST(utest_pcapping_core_get_domain_ctx_success);
    RUN_TEST(utest_pcapping_core_check_domain_configuration_min_cap_0);
    RUN_TEST(utest_pcapping_core_check_domain_configuration_max_cap_0);
    RUN_TEST(
        utest_pcapping_core_check_domain_configuration_invalid_cap_step_zero);
    RUN_TEST(
        utest_pcapping_core_check_domain_configuration_valid_cap_step_zero);
    RUN_TEST(utest_pcapping_core_check_domain_configuration_success);
    RUN_TEST(utest_pcapping_core_domain_init);
    RUN_TEST(utest_pcapping_core_domain_init_invalid_config);
    RUN_TEST(utest_pcapping_core_bind);
    RUN_TEST(utest_pcapping_core_init);
    RUN_TEST(utest_pcapping_core_start);
    RUN_TEST(utest_pcapping_core_set_cap_error_no_cap_config_support);
    RUN_TEST(utest_pcapping_core_set_cap_error_cap_lower_than_min);
    RUN_TEST(utest_pcapping_core_set_cap_error_cap_greater_than_max);
    RUN_TEST(utest_pcapping_core_set_cap_error_cap_not_match_step);
    RUN_TEST(utest_pcapping_core_set_cap_busy);
#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
    RUN_TEST(utest_pcapping_core_set_cap_busy_notification);
#endif
    RUN_TEST(utest_pcapping_core_set_cap_pending_sync_request);
    RUN_TEST(utest_pcapping_core_set_cap_pending_async_request);
    RUN_TEST(utest_pcapping_core_get_cap_out_of_range);
    RUN_TEST(utest_pcapping_core_get_cap_success);
    RUN_TEST(utest_pcapping_core_set_pai_error_pai_error);
#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
    RUN_TEST(utest_pcapping_core_set_pai_busy);
    RUN_TEST(utest_pcapping_core_set_pai_success_notification);
#endif
    RUN_TEST(utest_pcapping_core_set_pai_success);
    RUN_TEST(utest_pcapping_core_get_pai_out_of_range);
    RUN_TEST(utest_pcapping_core_get_pai_success);
    RUN_TEST(utest_pcapping_core_get_power_out_of_range);
    RUN_TEST(utest_pcapping_core_get_power_success);
    RUN_TEST(utest_pcapping_core_set_power_thresholds_success);
    RUN_TEST(utest_pcapping_core_set_power_thresholds_invalid_index);
    RUN_TEST(utest_pcapping_core_set_power_thresholds_hal_error);
    RUN_TEST(utest_pcapping_core_process_cap_fwk_notification);
#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
    RUN_TEST(
        utest_pcapping_core_process_cap_fwk_notification_notify_cap_change);
    RUN_TEST(utest_pcapping_core_process_power_measurements_fwk_notification);
#endif

    return UNITY_END();
}

#if !defined(TEST_ON_TARGET)
int main(void)
{
    return scmi_test_main();
}
#endif
