/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <mod_integration_test.h>

#include <fwk_core.h>
#include <fwk_event.h>
#include <fwk_log.h>
#include <fwk_mm.h>
#include <fwk_module.h>

#include <stddef.h>

#ifdef BUILD_HAS_DEBUGGER
#    include <cli.h>
#endif

#include "unity.h"

#define MOD_NAME "[INTEGRATION_TEST] "

static const fwk_id_t mod_integration_test =
    FWK_ID_MODULE_INIT(FWK_MODULE_IDX_INTEGRATION_TEST);

/*!
 * \brief Identifier for the
 * ::MOD_INTEGRATION_TEST_EVENT_IDX_RUN_TEST_SUITE event.
 */
static const fwk_id_t mod_integration_test_event_id_run_suite =
    FWK_ID_EVENT_INIT(
        FWK_MODULE_IDX_INTEGRATION_TEST,
        MOD_INTEGRATION_TEST_EVENT_IDX_RUN_TEST_SUITE);
/*!
 * \brief Identifier for the
 * ::MOD_INTEGRATION_TEST_EVENT_IDX_CONTINUE_STEP event.
 */
static const fwk_id_t mod_integration_test_event_id_step_continue =
    FWK_ID_EVENT_INIT(
        FWK_MODULE_IDX_INTEGRATION_TEST,
        MOD_INTEGRATION_TEST_EVENT_IDX_CONTINUE_STEP);

#define MAX_TEST_NAME_LENGTH 256

static struct {
    unsigned int suite_idx;
    unsigned int case_idx;
    const struct mod_integration_test_config *config;
    const struct mod_integration_test_api **apis;
} ctx;

#ifdef BUILD_HAS_DEBUGGER
static int32_t schedule_integration_test(int32_t argc, char **argv)
{
    int status;
    const char *suite_name;
    size_t test_suite_count, idx = 0;

    if (argc != 2) {
        return FWK_E_PARAM;
    }

    suite_name = argv[1];

    status =
        fwk_module_get_element_count(mod_integration_test, &test_suite_count);
    if (status != FWK_SUCCESS) {
        FWK_LOG_ERR("Failed to get test suite count (status = %d)", status);
        return status;
    }

    for (idx = 0; idx < test_suite_count; idx++) {
        fwk_id_t element_id =
            FWK_ID_ELEMENT(FWK_MODULE_IDX_INTEGRATION_TEST, idx);
        const char *name = fwk_module_get_element_name(element_id);
        if (cli_strncmp(name, suite_name, MAX_TEST_NAME_LENGTH) == 0) {
            struct fwk_event event = {
                .id = mod_integration_test_event_id_run_suite,
                .source_id = mod_integration_test,
                .target_id = mod_integration_test,
                .params = { idx }
            };
            return fwk_put_event(&event);
        }
    }

    return FWK_E_PARAM;
}
#endif

void setUp(void)
{
    if (ctx.apis[ctx.suite_idx]->setup != NULL) {
        ctx.apis[ctx.suite_idx]->setup();
    }
}

void tearDown(void)
{
    if (ctx.apis[ctx.suite_idx]->teardown != NULL) {
        ctx.apis[ctx.suite_idx]->teardown();
    }
}

static int run_test_case(void)
{
    const char *test_name = ctx.apis[ctx.suite_idx]->test_name(ctx.case_idx);
    UNITY_NEW_TEST(test_name);
    /* Call the setUp function, ensuring that any assertions return
       here */
    if (TEST_PROTECT()) {
        setUp();
    }

    struct fwk_event event = { .id =
                                   mod_integration_test_event_id_step_continue,
                               .target_id = mod_integration_test,
                               .params = { ctx.suite_idx, 0 } };

    return fwk_put_event(&event);
}

static int continue_test_case(const struct fwk_event *event)
{
    int status;
    unsigned int step_idx = event->params[1];
    volatile fwk_id_t element_id =
        FWK_ID_ELEMENT(FWK_MODULE_IDX_INTEGRATION_TEST, ctx.suite_idx);
    const struct mod_integration_test_config *config =
        fwk_module_get_data(element_id);

    /* Run the test case, ensuring that any assertions return here */
    if (TEST_PROTECT()) {
        status = ctx.apis[ctx.suite_idx]->run(ctx.case_idx, step_idx, event);
        /* Tests return FWK_PENDING to indicate they are awaiting an event */
        if (status == FWK_PENDING)
            return FWK_SUCCESS;
    }

    /* The test finished (success or failure) so call tearDown */
    if (TEST_PROTECT()) {
        if (!TEST_IS_IGNORED) {
            tearDown();
        }
        UnityConcludeTest();
    }

    /* Advance to the next test case in the suite */
    ctx.case_idx++;

    if (ctx.case_idx >= config->num_test_cases) {
        /* The test suite has finished */
        status = UNITY_END();
        FWK_LOG_INFO(
            MOD_NAME "End: %s", fwk_module_get_element_name(element_id));
        return status;
    }

    return run_test_case();
}

static int start_test(unsigned int idx)
{
    /* Set idx in the context for setup/teardown functions to use */
    ctx.suite_idx = idx;
    ctx.case_idx = 0;

    FWK_LOG_INFO(
        MOD_NAME "Start: %s",
        fwk_module_get_element_name(
            FWK_ID_ELEMENT(FWK_MODULE_IDX_INTEGRATION_TEST, idx)));

    UNITY_BEGIN();
    return run_test_case();
}

static int integration_test_init(
    fwk_id_t module_id,
    unsigned int element_count,
    const void *data)
{
    ctx.config = data;

#ifdef BUILD_HAS_DEBUGGER
    int status;
    status = cli_command_register((cli_command_st){
        "test", "Schedule test suite(s)", schedule_integration_test, false });
    if (status != FWK_SUCCESS) {
        return status;
    }
#endif

    fwk_assert(element_count < UINT8_MAX);

    /* Allocate context */
    ctx.apis = fwk_mm_calloc(element_count, sizeof(*ctx.apis));

    return FWK_SUCCESS;
}

static int integration_test_element_init(
    fwk_id_t element_id,
    unsigned int unused,
    const void *data)
{
    return FWK_SUCCESS;
}

static int integration_test_bind(fwk_id_t id, unsigned int round)
{
    unsigned int idx;

    if (round > 0) {
        return FWK_SUCCESS;
    }

    if (fwk_id_is_type(id, FWK_ID_TYPE_MODULE))
        return FWK_SUCCESS;

    const struct mod_integration_test_config *config = fwk_module_get_data(id);
    idx = fwk_id_get_element_idx(id);

    return fwk_module_bind(
        config->test_id,
        FWK_ID_API(
            fwk_id_get_module_idx(config->test_id),
            MOD_INTEGRATION_TEST_API_IDX_TEST),
        &ctx.apis[idx]);
}

static int integration_test_process_event(
    const struct fwk_event *event,
    struct fwk_event *resp_event)
{
    /* Process event to start a specific suite (triggered from CLI or boot) */
    if (fwk_id_is_equal(event->id, mod_integration_test_event_id_run_suite)) {
        return start_test(event->params[0]);
    }

    /* Handle continuation step for an in-progress test case */
    if (fwk_id_is_equal(
            event->id, mod_integration_test_event_id_step_continue)) {
        return continue_test_case(event);
    }
    return FWK_E_PARAM;
}

static int integration_test_start(fwk_id_t id)
{
    int status = FWK_SUCCESS;

    if (fwk_id_is_type(id, FWK_ID_TYPE_MODULE))
        return FWK_SUCCESS;

    const struct mod_integration_test_config *config = fwk_module_get_data(id);

    if (config->run_at_start) {
        struct fwk_event event = { .id =
                                       mod_integration_test_event_id_run_suite,
                                   .source_id = mod_integration_test,
                                   .target_id = mod_integration_test,
                                   .params = { fwk_id_get_element_idx(id) } };

        status = fwk_put_event(&event);
        if (status != FWK_SUCCESS) {
            fwk_unexpected();
        }
    }

    return status;
}

const struct fwk_module module_integration_test = {
    .type = FWK_MODULE_TYPE_SERVICE,
    .init = integration_test_init,
    .element_init = integration_test_element_init,
    .bind = integration_test_bind,
    .process_event = integration_test_process_event,
    .event_count = MOD_INTEGRATION_TEST_EVENT_IDX_COUNT,
    .start = integration_test_start,
};
