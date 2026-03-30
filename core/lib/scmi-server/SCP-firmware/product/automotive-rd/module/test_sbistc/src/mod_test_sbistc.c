/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <unity.h>

#include <mod_fmu.h>
#include <mod_integration_test.h>
#include <mod_sbistc.h>
#include <mod_ssu.h>

#include <fwk_assert.h>
#include <fwk_core.h>
#include <fwk_id.h>
#include <fwk_log.h>
#include <fwk_module.h>
#include <fwk_notification.h>

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define MOD_NAME "[TEST_SBISTC] "

enum { TEST_STEP_INJECT, TEST_STEP_CHECK } test_step;

static bool sbistc_inject_flag = false;
uint8_t test_sbistc_step_idx;

struct sbistc_event_packet {
    uint8_t suite_idx;
    uint8_t step_idx;
    struct mod_fmu_fault_notification_params *fmu_params;
};

static const struct mod_fmu_api *fmu_api = NULL;
static const struct mod_sbistc_api *sbistc_api = NULL;
static const struct mod_ssu_sys_register_api *ssu_sys_api = NULL;
static const struct mod_ssu_error_control_status_api *ssu_err_api = NULL;

static volatile bool handler_called = false;

static void test_fault_handler(void)
{
    FWK_LOG_INFO(MOD_NAME " [%s] test_fault_handler called!", __func__);
    handler_called = true;
}

static int test_sbistc_inject(
    unsigned int fault_id,
    unsigned int step_idx,
    const struct fwk_event *event)
{
    int status;
    uint8_t count_before = 0;
    uint8_t count_after = 0;
    struct mod_fmu_fault fault = { 0 };
    uint32_t ssu_state = 0;

    const struct mod_sbistc_config *sbistc_mod_config =
        (const struct mod_sbistc_config *)fwk_module_get_data(
            FWK_ID_MODULE(FWK_MODULE_IDX_SBISTC));
    TEST_ASSERT_NOT_NULL(sbistc_mod_config);

    /* All faults tested? */
    if (fault_id >= sbistc_mod_config->count)
        return FWK_SUCCESS;

    const struct sbistc_fault_config *flt =
        &sbistc_mod_config->flt_cfgs[fault_id];

    switch (step_idx) {
    /* Step 0: Setup and inject */
    case TEST_STEP_INJECT:
        status = sbistc_api->set_enabled(fault_id, true);
        TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

        status = sbistc_api->set_handler(fault_id, test_fault_handler);
        TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

        status = sbistc_api->get_count(fault_id, &count_before);
        TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

        fault.device_idx = flt->fmu_device_id;
        fault.node_idx = flt->fmu_node_id;
        fault.sm_idx = MOD_FMU_SM_SYSTEM_INPUT_ERROR;
        status = fmu_api->inject(&fault);
        TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
        sbistc_inject_flag = true;
        FWK_LOG_INFO(
            MOD_NAME "Injected %s (FMU dev=%u, node=%u)",
            flt->flt_name,
            flt->fmu_device_id,
            flt->fmu_node_id);

        /* Wait for SBISTC to process */
        return FWK_PENDING;

    /* Step 1: Check results */
    case TEST_STEP_CHECK:
        status = sbistc_api->get_count(fault_id, &count_after);
        TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
        TEST_ASSERT_TRUE(count_after > count_before);
        TEST_ASSERT_TRUE(handler_called);

        FWK_LOG_INFO(
            MOD_NAME "fault:%u: count:%u, flag:%s, ssu_state:0x%X",
            fault_id,
            count_after,
            handler_called ? "true" : "false",
            ssu_state);
        /* Check if the fault was reported to SSU */
        fwk_id_t ssu_dev_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_SSU, 0);
        TEST_ASSERT_NOT_NULL(ssu_sys_api);
        status = ssu_sys_api->get_sys_status(ssu_dev_id, &ssu_state);
        TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
        TEST_ASSERT_EQUAL(MOD_SSU_SAFETY_STATUS_ERRN, ssu_state);

        FWK_LOG_INFO(
            MOD_NAME "fault:%u: count:%u, flag:%s, ssu_state:0x%X",
            fault_id,
            count_after,
            handler_called ? "true" : "false",
            ssu_state);

        /* Reset the handler_called flag for the next iteration */
        handler_called = false;
        /* Disable the fault handler for the next iteration */
        status = sbistc_api->set_handler(fault_id, NULL);
        TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

        return FWK_SUCCESS;
    }
    return FWK_E_PARAM;
}

static const char *test_name(unsigned int case_idx)
{
    FWK_LOG_INFO(MOD_NAME "test_name called with case_idx=%u", case_idx);
    const struct mod_sbistc_config *sbistc_mod_config =
        (const struct mod_sbistc_config *)fwk_module_get_data(
            FWK_ID_MODULE(FWK_MODULE_IDX_SBISTC));
    /* Inject only valid SBISTC Faults */
    if (sbistc_mod_config && (case_idx < sbistc_mod_config->count)) {
        return "test_sbistc_inject";
    }
    return NULL;
}

static int run(
    unsigned int case_idx,
    unsigned int step_idx,
    const struct fwk_event *event)
{
    return test_sbistc_inject(case_idx, step_idx, event);
}

static void test_sbistc_setup(void)
{
}

static void test_sbistc_teardown(void)
{
    test_sbistc_step_idx = 0;
}

static const struct mod_integration_test_api test_api = {
    .run = run,
    .test_name = test_name,
    .setup = test_sbistc_setup,
    .teardown = test_sbistc_teardown,
};

static int test_sbistc_init(
    fwk_id_t module_id,
    unsigned int unused,
    const void *data)
{
    return FWK_SUCCESS;
}

static int test_sbistc_process_bind_request(
    fwk_id_t requester_id,
    fwk_id_t target_id,
    fwk_id_t api_id,
    const void **api)
{
    if (fwk_id_get_api_idx(api_id) != MOD_INTEGRATION_TEST_API_IDX_TEST)
        return FWK_E_PARAM;

    *api = &test_api;
    return FWK_SUCCESS;
}

static int test_sbistc_bind(fwk_id_t id, unsigned int round)
{
    if (round > 0)
        return FWK_SUCCESS;

    int status = fwk_module_bind(
        FWK_ID_MODULE(FWK_MODULE_IDX_FMU),
        FWK_ID_API(FWK_MODULE_IDX_FMU, MOD_FMU_DEVICE_API_IDX),
        (const void **)&fmu_api);

    status = fwk_module_bind(
        FWK_ID_MODULE(FWK_MODULE_IDX_SBISTC),
        FWK_ID_API(FWK_MODULE_IDX_SBISTC, MOD_SBISTC_API_IDX_DEFAULT),
        (const void **)&sbistc_api);

    /* Bind to SSU sys API for state checking */
    int ssu_status = fwk_module_bind(
        FWK_ID_MODULE(FWK_MODULE_IDX_SSU),
        FWK_ID_API(FWK_MODULE_IDX_SSU, MOD_SSU_SYS_API_IDX),
        (const void **)&ssu_sys_api);
    if (ssu_status != FWK_SUCCESS)
        return ssu_status;

    /* Bind to SSU error control/status API for test setup */
    ssu_status = fwk_module_bind(
        FWK_ID_MODULE(FWK_MODULE_IDX_SSU),
        FWK_ID_API(FWK_MODULE_IDX_SSU, MOD_SSU_ERR_API_IDX),
        (const void **)&ssu_err_api);
    if (ssu_status != FWK_SUCCESS)
        return ssu_status;

    return status;
}

static int test_sbistc_start(fwk_id_t id)
{
    int status = FWK_SUCCESS;
    /* Enable error detection for non-critical errors in SSU */
    if (ssu_err_api) {
        fwk_id_t ssu_dev_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_SSU, 0);
        status =
            ssu_err_api->err_detect_control(ssu_dev_id, MOD_SSU_ERR_NCR_EN, 1);
        TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    }

#ifdef BUILD_HAS_NOTIFICATION
    if (!fwk_id_is_type(id, FWK_ID_TYPE_MODULE)) {
        return FWK_SUCCESS;
    }
    /* Subscribe to FMU fault notifications */
    status = fwk_notification_subscribe(
        mod_fmu_notification_id_fault, FWK_ID_MODULE(FWK_MODULE_IDX_FMU), id);
    if (status != FWK_SUCCESS) {
        FWK_LOG_ERR(MOD_NAME "Failed to subscribe to FMU fault notification");
    } else {
        FWK_LOG_INFO(MOD_NAME "Subscribed to FMU fault notifications");
    }
#endif /* BUILD_HAS_NOTIFICATION */

    return status;
}

#ifdef BUILD_HAS_NOTIFICATION
static int test_sbistc_process_notification(
    const struct fwk_event *event,
    struct fwk_event *resp_event)
{
    int status = FWK_SUCCESS;
    struct mod_fmu_fault_notification_params *fmu_notify_params;

    struct fwk_event step_event = {
        .id = MOD_INTEGRATION_TEST_EVENT_ID_STEP_CONTINUE,
        .source_id = FWK_ID_MODULE(FWK_MODULE_IDX_TEST_FMU),
        .target_id = FWK_ID_MODULE(FWK_MODULE_IDX_INTEGRATION_TEST),
    };

    if (sbistc_inject_flag == true) {
        if (event == NULL)
            return FWK_E_PARAM;

        fwk_assert(fwk_id_is_type(event->target_id, FWK_ID_TYPE_MODULE));

        if (!fwk_id_is_equal(event->id, mod_fmu_notification_id_fault))
            return FWK_E_PARAM;

        fmu_notify_params =
            (struct mod_fmu_fault_notification_params *)event->params;
        test_sbistc_step_idx++;

        struct sbistc_event_packet payload = {
            .suite_idx = 0,
            .step_idx = test_sbistc_step_idx,
            .fmu_params = fmu_notify_params,
        };

        memcpy((void *)step_event.params, (void *)&payload, sizeof(payload));

        status = fwk_put_event(&step_event);
        fwk_assert(status == FWK_SUCCESS);
    }

    return status;
}
#endif

struct fwk_module_config config_test_sbistc = { 0 };

const struct fwk_module module_test_sbistc = {
    .type = FWK_MODULE_TYPE_SERVICE,
    .init = test_sbistc_init,
    .start = test_sbistc_start,
    .process_bind_request = test_sbistc_process_bind_request,
    .api_count = MOD_INTEGRATION_TEST_API_COUNT,
    .bind = test_sbistc_bind,
#ifdef BUILD_HAS_NOTIFICATION
    .process_notification = test_sbistc_process_notification,
#endif /* BUILD_HAS_NOTIFICATION */
};
