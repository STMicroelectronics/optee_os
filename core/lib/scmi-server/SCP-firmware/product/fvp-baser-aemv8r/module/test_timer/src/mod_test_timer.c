/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <unity.h>

#include <mod_integration_test.h>
#include <mod_test_timer.h>
#include <mod_timer.h>

#include <fwk_core.h>
#include <fwk_id.h>
#include <fwk_module.h>

enum test_case_idx { TEST_CASE_TIMER_ALARM_TRIGGER, TEST_CASE_COUNT };

static const struct mod_timer_alarm_api *alarm_api;
static fwk_id_t alarm_id;
static bool callback_invoked = false;

struct test_timer_context {
    unsigned int case_idx;
    unsigned int step_idx;
};

static void alarm_callback(uintptr_t param)
{
    const struct test_timer_context *ctx = (const void *)param;
    callback_invoked = true;

    struct fwk_event event = {
        .id = MOD_INTEGRATION_TEST_EVENT_ID_STEP_CONTINUE,
        .source_id = FWK_ID_MODULE(FWK_MODULE_IDX_TEST_TIMER),
        .target_id = FWK_ID_MODULE(FWK_MODULE_IDX_INTEGRATION_TEST),
        .params = { ctx->case_idx, ctx->step_idx }
    };

    int status = fwk_put_event(&event);
    fwk_assert(status == FWK_SUCCESS);
}

static int test_timer_alarm_trigger(
    unsigned int step_idx,
    const struct fwk_event *event)
{
    switch (step_idx) {
    case 0:
        callback_invoked = false;
        static struct test_timer_context ctx = {
            .case_idx = TEST_CASE_TIMER_ALARM_TRIGGER,
            .step_idx = 1,
        };
        alarm_api->start(
            alarm_id,
            100000,
            MOD_TIMER_ALARM_TYPE_ONCE,
            alarm_callback,
            (uintptr_t)&ctx);
        return FWK_PENDING;
    case 1:
        TEST_ASSERT_TRUE(callback_invoked);
        return FWK_SUCCESS;
    default:
        return FWK_E_PARAM;
    }
}

static int run(
    unsigned int case_idx,
    unsigned int step_idx,
    const struct fwk_event *event)
{
    switch (case_idx) {
    case TEST_CASE_TIMER_ALARM_TRIGGER:
        return test_timer_alarm_trigger(step_idx, event);
    default:
        return FWK_E_PARAM;
    }
}

static const char *test_name(unsigned int case_idx)
{
    switch (case_idx) {
    case TEST_CASE_TIMER_ALARM_TRIGGER:
        return "test_timer_alarm_trigger";
    default:
        return NULL;
    }
}

static const struct mod_integration_test_api test_api = {
    .run = run,
    .test_name = test_name,
};

static int test_timer_init(
    fwk_id_t module_id,
    unsigned int unused,
    const void *data)
{
    const struct mod_test_timer_config *config = data;

    alarm_id = config->alarm_id;
    return FWK_SUCCESS;
}

static int test_timer_bind(fwk_id_t id, unsigned int round)
{
    if (round > 0) {
        return FWK_SUCCESS;
    }

    return fwk_module_bind(
        alarm_id, MOD_TIMER_API_ID_ALARM, (void **)&alarm_api);
}

static int test_timer_process_bind_request(
    fwk_id_t requester_id,
    fwk_id_t target_id,
    fwk_id_t api_id,
    const void **api)
{
    if (fwk_id_get_api_idx(api_id) != MOD_INTEGRATION_TEST_API_IDX_TEST) {
        return FWK_E_PARAM;
    }

    *api = &test_api;
    return FWK_SUCCESS;
}

const struct fwk_module module_test_timer = {
    .type = FWK_MODULE_TYPE_SERVICE,
    .init = test_timer_init,
    .process_bind_request = test_timer_process_bind_request,
    .api_count = MOD_INTEGRATION_TEST_API_COUNT,
    .bind = test_timer_bind,
};
