/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <unity.h>

#include <mod_fmu.h>
#include <mod_integration_test.h>
#include <mod_timer.h>

#include <fwk_core.h>
#include <fwk_id.h>
#include <fwk_log.h>
#include <fwk_module.h>
#include <fwk_notification.h>
#include <fwk_time.h>

#include <string.h>

#define MOD_NAME "[TEST_FMU] "

static struct mod_fmu_api *fmu_api;

static fwk_id_t root_fmu = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_FMU, 0);
static fwk_id_t fmu1 = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_FMU, 1);

static fwk_id_t gic_fmu = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_FMU, 5);
static fwk_id_t rse_cl0_mhu_fmu = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_FMU, 6);
static fwk_id_t cl0_rse_mhu_fmu = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_FMU, 7);
static fwk_id_t pc0_cl0_mhu_fmu = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_FMU, 8);
static fwk_id_t cl0_pc0_mhu_fmu = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_FMU, 9);
static fwk_id_t pc1_cl0_mhu_fmu = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_FMU, 10);
static fwk_id_t cl0_pc1_mhu_fmu = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_FMU, 11);
static fwk_id_t pc2_cl0_mhu_fmu = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_FMU, 12);
static fwk_id_t cl0_pc2_mhu_fmu = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_FMU, 13);
static fwk_id_t pc3_cl0_mhu_fmu = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_FMU, 14);
static fwk_id_t cl0_pc3_mhu_fmu = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_FMU, 15);

#define GIC_FMU_BLOCK_TYPE_WAKE 1
#define MHU_FMU_BLOCK_TYPE_FMU  2

enum test_inject_state {
    STEP_START,
    STEP_INJECT_0,
    STEP_INJECT_1,
    STEP_INJECT_2,
    STEP_COUNT
};

uint8_t test_fmu_step_idx;
static bool fmu_inject_flag = false;
struct fmu_event_packet {
    uint8_t suite_idx;
    uint8_t step_idx;
    struct mod_fmu_fault_notification_params *fmu_params;
};

static int test_inject(unsigned int step_idx, const struct fwk_event *event)
{
    int status;
    struct mod_fmu_fault fault = { 0 };
    const struct fmu_event_packet *event_params;
    const struct mod_fmu_fault_notification_params *params;

    fwk_assert(step_idx < STEP_COUNT);

    switch (step_idx) {
    case STEP_START:
        fault.device_idx = fwk_id_get_element_idx(root_fmu);
        fault.node_idx = 0;
        fault.sm_idx = MOD_FMU_SM_ALL;
        status = fmu_api->set_enabled(&fault, true);
        TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

        /* Inject a critical fault */
        fault.sm_idx = MOD_FMU_SM_SYSTEM_INPUT_ERROR;
        status = fmu_api->inject(&fault);
        fmu_inject_flag = true;
        TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
        return FWK_PENDING;
    case STEP_INJECT_0:
        /* Validate the received fault */
        event_params = (const struct fmu_event_packet *)event->params;
        params = (const struct mod_fmu_fault_notification_params *)
                     event_params->fmu_params;
        TEST_ASSERT_EQUAL(
            fwk_id_get_element_idx(root_fmu), params->fault.device_idx);
        TEST_ASSERT_EQUAL(0, params->fault.node_idx);
        TEST_ASSERT_EQUAL(MOD_FMU_SM_SYSTEM_INPUT_ERROR, params->fault.sm_idx);
        TEST_ASSERT_TRUE(params->critical);

        /* Inject a non-critical fault */
        fault.device_idx = 0;
        fault.node_idx = 1;
        fault.sm_idx = MOD_FMU_SM_ALL;
        status = fmu_api->set_enabled(&fault, true);
        TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

        fault.sm_idx = MOD_FMU_SM_SYSTEM_INPUT_ERROR;
        status = fmu_api->inject(&fault);
        TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
        return FWK_PENDING;
    case STEP_INJECT_1:
        /* Validate the received fault */
        event_params = (const struct fmu_event_packet *)event->params;
        params = (const struct mod_fmu_fault_notification_params *)
                     event_params->fmu_params;
        TEST_ASSERT_EQUAL(
            fwk_id_get_element_idx(root_fmu), params->fault.device_idx);
        TEST_ASSERT_EQUAL(1, params->fault.node_idx);
        TEST_ASSERT_EQUAL(MOD_FMU_SM_SYSTEM_INPUT_ERROR, params->fault.sm_idx);
        TEST_ASSERT_FALSE(params->critical);

        /* Inject a non-critical fault to an upstream FMU */
        fault.device_idx = fwk_id_get_element_idx(fmu1);
        fault.node_idx = 0;
        fault.sm_idx = MOD_FMU_SM_ALL;
        status = fmu_api->set_enabled(&fault, true);
        TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

        fault.sm_idx = MOD_FMU_SM_SYSTEM_INPUT_ERROR;
        status = fmu_api->inject(&fault);
        TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
        return FWK_PENDING;
    case STEP_INJECT_2:
        /* Validate the received fault */
        event_params = (const struct fmu_event_packet *)event->params;
        params = (const struct mod_fmu_fault_notification_params *)
                     event_params->fmu_params;
        TEST_ASSERT_EQUAL(
            fwk_id_get_element_idx(fmu1), params->fault.device_idx);
        TEST_ASSERT_EQUAL(0, params->fault.node_idx);
        TEST_ASSERT_EQUAL(MOD_FMU_SM_SYSTEM_INPUT_ERROR, params->fault.sm_idx);
        TEST_ASSERT_FALSE(params->critical);
        return FWK_SUCCESS;

    default:
        fwk_unreachable();
    }
}

static void validate_fault(
    fwk_id_t device_id,
    int node_idx,
    int sm_idx,
    bool expected_critical,
    const struct fwk_event *event)
{
    const struct fmu_event_packet *event_params;
    const struct mod_fmu_fault_notification_params *params;

    event_params = (const struct fmu_event_packet *)event->params;
    params = (const struct mod_fmu_fault_notification_params *)
                 event_params->fmu_params;

    TEST_ASSERT_EQUAL(
        fwk_id_get_element_idx(device_id), params->fault.device_idx);
    TEST_ASSERT_EQUAL(node_idx, params->fault.node_idx);
    TEST_ASSERT_EQUAL(sm_idx, params->fault.sm_idx);
    TEST_ASSERT_EQUAL(expected_critical, params->critical);
}

static void enable_and_inject(struct mod_fmu_fault *fault, bool critical)
{
    int status = fmu_api->set_enabled(fault, true);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

    if (critical) {
        status = fmu_api->set_critical(fault, true);
        TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    }

    status = fmu_api->inject(fault);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

static int inject_fault(
    fwk_id_t fmu_id,
    int node_idx,
    unsigned int step_idx,
    const struct fwk_event *event)
{
    fwk_assert(step_idx < STEP_COUNT);
    struct mod_fmu_fault fault = { .device_idx = fwk_id_get_element_idx(fmu_id),
                                   .node_idx = node_idx,
                                   .sm_idx = 2 };
    switch (step_idx) {
    case STEP_START:
        enable_and_inject(&fault, true);
        return FWK_PENDING;
    case STEP_INJECT_0:
        validate_fault(fmu_id, node_idx, 2, true, event);
        fmu_api->set_critical(&fault, false);
        enable_and_inject(&fault, false);
        return FWK_PENDING;
    case STEP_INJECT_1:
        validate_fault(fmu_id, node_idx, 2, false, event);
        return FWK_SUCCESS;
    default:
        fwk_unreachable();
    }
}

static int test_inject_gic_fmu(
    unsigned int step_idx,
    const struct fwk_event *event)
{
    return inject_fault(gic_fmu, GIC_FMU_BLOCK_TYPE_WAKE, step_idx, event);
}

static int test_inject_rse_cl0_mhu_fmu(
    unsigned int step_idx,
    const struct fwk_event *event)
{
    return inject_fault(
        rse_cl0_mhu_fmu, MHU_FMU_BLOCK_TYPE_FMU, step_idx, event);
}

static int test_inject_cl0_rse_mhu_fmu(
    unsigned int step_idx,
    const struct fwk_event *event)
{
    return inject_fault(
        cl0_rse_mhu_fmu, MHU_FMU_BLOCK_TYPE_FMU, step_idx, event);
}

static int test_inject_pc0_cl0_mhu_fmu(
    unsigned int step_idx,
    const struct fwk_event *event)
{
    return inject_fault(
        pc0_cl0_mhu_fmu, MHU_FMU_BLOCK_TYPE_FMU, step_idx, event);
}

static int test_inject_cl0_pc0_mhu_fmu(
    unsigned int step_idx,
    const struct fwk_event *event)
{
    return inject_fault(
        cl0_pc0_mhu_fmu, MHU_FMU_BLOCK_TYPE_FMU, step_idx, event);
}

static int test_inject_pc1_cl0_mhu_fmu(
    unsigned int step_idx,
    const struct fwk_event *event)
{
    return inject_fault(
        pc1_cl0_mhu_fmu, MHU_FMU_BLOCK_TYPE_FMU, step_idx, event);
}

static int test_inject_cl0_pc1_mhu_fmu(
    unsigned int step_idx,
    const struct fwk_event *event)
{
    return inject_fault(
        cl0_pc1_mhu_fmu, MHU_FMU_BLOCK_TYPE_FMU, step_idx, event);
}

static int test_inject_pc2_cl0_mhu_fmu(
    unsigned int step_idx,
    const struct fwk_event *event)
{
    return inject_fault(
        pc2_cl0_mhu_fmu, MHU_FMU_BLOCK_TYPE_FMU, step_idx, event);
}

static int test_inject_cl0_pc2_mhu_fmu(
    unsigned int step_idx,
    const struct fwk_event *event)
{
    return inject_fault(
        cl0_pc2_mhu_fmu, MHU_FMU_BLOCK_TYPE_FMU, step_idx, event);
}

static int test_inject_pc3_cl0_mhu_fmu(
    unsigned int step_idx,
    const struct fwk_event *event)
{
    return inject_fault(
        pc3_cl0_mhu_fmu, MHU_FMU_BLOCK_TYPE_FMU, step_idx, event);
}

static int test_inject_cl0_pc3_mhu_fmu(
    unsigned int step_idx,
    const struct fwk_event *event)
{
    return inject_fault(
        cl0_pc3_mhu_fmu, MHU_FMU_BLOCK_TYPE_FMU, step_idx, event);
}

static int test_set_enabled(
    unsigned int step_idx,
    const struct fwk_event *event)
{
    int status;
    bool enabled;
    struct mod_fmu_fault fault = { 0 };
    const struct fmu_event_packet *event_params;
    const struct mod_fmu_fault_notification_params *params;
    enum test_inject_state {
        STEP_START,
        STEP_ENABLED,
        STEP_COUNT,
    } state = step_idx;
    fwk_assert(step_idx < STEP_COUNT);

    switch (state) {
    case STEP_START:
        /* Disable the fault and check its status */
        fault.device_idx = 0;
        fault.node_idx = 0;
        fault.sm_idx = MOD_FMU_SM_ALL;
        status = fmu_api->set_enabled(&fault, false);
        TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
        status = fmu_api->get_enabled(&fault, &enabled);
        TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
        TEST_ASSERT_FALSE(enabled);

        /* Inject a fault */
        fault.sm_idx = MOD_FMU_SM_SYSTEM_INPUT_ERROR;
        status = fmu_api->inject(&fault);
        TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

        /* Re-enable the fault and check its status */
        fault.sm_idx = MOD_FMU_SM_ALL;
        status = fmu_api->set_enabled(&fault, true);
        TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
        status = fmu_api->get_enabled(&fault, &enabled);
        TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
        TEST_ASSERT_TRUE(enabled);

        /* Inject a fault */
        fault.sm_idx = MOD_FMU_SM_SYSTEM_INPUT_ERROR;
        status = fmu_api->inject(&fault);
        TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

        return FWK_PENDING;
    case STEP_ENABLED:
        event_params = (const struct fmu_event_packet *)event->params;
        params = (const struct mod_fmu_fault_notification_params *)
                     event_params->fmu_params;
        TEST_ASSERT_EQUAL(0, params->fault.device_idx);
        TEST_ASSERT_EQUAL(0, params->fault.node_idx);
        TEST_ASSERT_EQUAL(MOD_FMU_SM_SYSTEM_INPUT_ERROR, params->fault.sm_idx);
        return FWK_SUCCESS;

    default:
        fwk_unreachable();
    }
}

static int test_upgrade(unsigned int step_idx, const struct fwk_event *event)
{
    int status;
    struct mod_fmu_fault fault = { 0 };
    const struct fmu_event_packet *event_params;
    const struct mod_fmu_fault_notification_params *params;
    bool enabled;
    uint8_t val;
    enum test_inject_state {
        STEP_START,
        STEP_INJECT_0,
        STEP_INJECT_1,
        STEP_INJECT_2,
        STEP_COUNT,
    } state = step_idx;
    fwk_assert(step_idx < STEP_COUNT);

    switch (state) {
    case STEP_START:
        fault.device_idx = 0;
        fault.node_idx = 1;
        fault.sm_idx = MOD_FMU_SM_SYSTEM_INPUT_ERROR;
        /* Configure fault upgrade after 2 occurrences */
        status = fmu_api->set_upgrade_enabled(root_fmu, fault.node_idx, true);
        TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
        status = fmu_api->set_threshold(root_fmu, fault.node_idx, 2);
        TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
        status = fmu_api->set_count(root_fmu, fault.node_idx, 0);
        TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

        /* Read back configuration */
        status =
            fmu_api->get_upgrade_enabled(root_fmu, fault.node_idx, &enabled);
        TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
        TEST_ASSERT_TRUE(enabled);
        status = fmu_api->get_threshold(root_fmu, fault.node_idx, &val);
        TEST_ASSERT_EQUAL(val, 2);
        TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
        status = fmu_api->get_count(root_fmu, fault.node_idx, &val);
        TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
        TEST_ASSERT_EQUAL(val, 0);

        /* Inject fault for the first time */
        status = fmu_api->inject(&fault);
        TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
        return FWK_PENDING;

    case STEP_INJECT_0:
        /* Validate the received fault */
        event_params = (const struct fmu_event_packet *)event->params;
        params = (const struct mod_fmu_fault_notification_params *)
                     event_params->fmu_params;
        TEST_ASSERT_EQUAL(1, params->fault.node_idx);
        TEST_ASSERT_FALSE(params->critical);

        /* Check that the count has incremented */
        status = fmu_api->get_count(root_fmu, 1, &val);
        TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
        TEST_ASSERT_EQUAL(1, val);

        /* Inject fault for the second time */
        fault.device_idx = 0;
        fault.node_idx = 1;
        fault.sm_idx = MOD_FMU_SM_SYSTEM_INPUT_ERROR;
        status = fmu_api->inject(&fault);
        TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
        return FWK_PENDING;

    case STEP_INJECT_1:
        /* Validate the received fault */
        event_params = (const struct fmu_event_packet *)event->params;
        params = (const struct mod_fmu_fault_notification_params *)
                     event_params->fmu_params;
        TEST_ASSERT_EQUAL(1, params->fault.node_idx);
        TEST_ASSERT_FALSE(params->critical);

        /* Check that the count has incremented */
        status = fmu_api->get_count(root_fmu, 1, &val);
        TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
        TEST_ASSERT_EQUAL(val, 2);

        /* Inject fault for the third time */
        fault.device_idx = 0;
        fault.node_idx = 1;
        fault.sm_idx = MOD_FMU_SM_SYSTEM_INPUT_ERROR;
        status = fmu_api->inject(&fault);
        TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
        return FWK_PENDING;

    case STEP_INJECT_2:
        event_params = (const struct fmu_event_packet *)event->params;
        params = (const struct mod_fmu_fault_notification_params *)
                     event_params->fmu_params;
        TEST_ASSERT_EQUAL(1, params->fault.node_idx);
        TEST_ASSERT_TRUE(params->critical);

        /* Check that the count has incremented */
        status = fmu_api->get_count(root_fmu, 1, &val);
        TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
        TEST_ASSERT_EQUAL(val, 3);

        return FWK_SUCCESS;

    default:
        fwk_unreachable();
    }
}

enum test_case {
    TEST_CASE_INJECT,
    TEST_CASE_INJECT_GIC_FMU,
    TEST_CASE_INJECT_RSE_CL0_MHU_FMU,
    TEST_CASE_INJECT_CL0_RSE_MHU_FMU,
    TEST_CASE_INJECT_PC0_CL0_MHU_FMU,
    TEST_CASE_INJECT_CL0_PC0_MHU_FMU,
    TEST_CASE_INJECT_PC1_CL0_MHU_FMU,
    TEST_CASE_INJECT_CL0_PC1_MHU_FMU,
    TEST_CASE_INJECT_PC2_CL0_MHU_FMU,
    TEST_CASE_INJECT_CL0_PC2_MHU_FMU,
    TEST_CASE_INJECT_PC3_CL0_MHU_FMU,
    TEST_CASE_INJECT_CL0_PC3_MHU_FMU,
    TEST_CASE_SET_ENABLED,
    TEST_CASE_UPGRADE,
    TEST_CASE_COUNT,
};

static const char *test_name(unsigned int case_idx)
{
    enum test_case current_test = case_idx;

    switch (current_test) {
    case TEST_CASE_INJECT:
        return "test_inject";
    case TEST_CASE_INJECT_GIC_FMU:
        return "test_inject_gic_fmu";
    case TEST_CASE_INJECT_RSE_CL0_MHU_FMU:
        return "test_inject_rse_cl0_mhu_fmu";
    case TEST_CASE_INJECT_CL0_RSE_MHU_FMU:
        return "test_inject_cl0_rse_mhu_fmu";
    case TEST_CASE_INJECT_PC0_CL0_MHU_FMU:
        return "test_inject_pc0_cl0_mhu_fmu";
    case TEST_CASE_INJECT_CL0_PC0_MHU_FMU:
        return "test_inject_cl0_pc0_mhu_fmu";
    case TEST_CASE_INJECT_PC1_CL0_MHU_FMU:
        return "test_inject_pc1_cl0_mhu_fmu";
    case TEST_CASE_INJECT_CL0_PC1_MHU_FMU:
        return "test_inject_cl0_pc1_mhu_fmu";
    case TEST_CASE_INJECT_PC2_CL0_MHU_FMU:
        return "test_inject_pc2_cl0_mhu_fmu";
    case TEST_CASE_INJECT_CL0_PC2_MHU_FMU:
        return "test_inject_cl0_pc2_mhu_fmu";
    case TEST_CASE_INJECT_PC3_CL0_MHU_FMU:
        return "test_inject_pc3_cl0_mhu_fmu";
    case TEST_CASE_INJECT_CL0_PC3_MHU_FMU:
        return "test_inject_cl0_pc3_mhu_fmu";
    case TEST_CASE_SET_ENABLED:
        return "test_set_enabled";
    case TEST_CASE_UPGRADE:
        return "test_upgrade";
    default:
        return NULL;
    }
}

static int run(
    unsigned int case_idx,
    unsigned int step_idx,
    const struct fwk_event *event)
{
    enum test_case current_test = case_idx;

    switch (current_test) {
    case TEST_CASE_INJECT:
        return test_inject(step_idx, event);
    case TEST_CASE_INJECT_GIC_FMU:
        return test_inject_gic_fmu(step_idx, event);
    case TEST_CASE_INJECT_RSE_CL0_MHU_FMU:
        return test_inject_rse_cl0_mhu_fmu(step_idx, event);
    case TEST_CASE_INJECT_CL0_RSE_MHU_FMU:
        return test_inject_cl0_rse_mhu_fmu(step_idx, event);
    case TEST_CASE_INJECT_PC0_CL0_MHU_FMU:
        return test_inject_pc0_cl0_mhu_fmu(step_idx, event);
    case TEST_CASE_INJECT_CL0_PC0_MHU_FMU:
        return test_inject_cl0_pc0_mhu_fmu(step_idx, event);
    case TEST_CASE_INJECT_PC1_CL0_MHU_FMU:
        return test_inject_pc1_cl0_mhu_fmu(step_idx, event);
    case TEST_CASE_INJECT_CL0_PC1_MHU_FMU:
        return test_inject_cl0_pc1_mhu_fmu(step_idx, event);
    case TEST_CASE_INJECT_PC2_CL0_MHU_FMU:
        return test_inject_pc2_cl0_mhu_fmu(step_idx, event);
    case TEST_CASE_INJECT_CL0_PC2_MHU_FMU:
        return test_inject_cl0_pc2_mhu_fmu(step_idx, event);
    case TEST_CASE_INJECT_PC3_CL0_MHU_FMU:
        return test_inject_pc3_cl0_mhu_fmu(step_idx, event);
    case TEST_CASE_INJECT_CL0_PC3_MHU_FMU:
        return test_inject_cl0_pc3_mhu_fmu(step_idx, event);
    case TEST_CASE_SET_ENABLED:
        return test_set_enabled(step_idx, event);
    case TEST_CASE_UPGRADE:
        return test_upgrade(step_idx, event);
    default:
        return FWK_E_PARAM;
    }
}

void test_fmu_setup(void)
{
}
void test_fmu_teardown(void)
{
    test_fmu_step_idx = 0;
}

static const struct mod_integration_test_api test_api = {
    .run = run,
    .test_name = test_name,
    .setup = test_fmu_setup,
    .teardown = test_fmu_teardown
};

static int test_fmu_init(
    fwk_id_t module_id,
    unsigned int unused,
    const void *data)
{
    return FWK_SUCCESS;
}

static int test_fmu_process_bind_request(
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

static int test_fmu_bind(fwk_id_t id, unsigned int round)
{
    int status;

    if (round > 0)
        return FWK_SUCCESS;

    status = fwk_module_bind(
        FWK_ID_MODULE(FWK_MODULE_IDX_FMU),
        FWK_ID_API(FWK_MODULE_IDX_FMU, MOD_FMU_DEVICE_API_IDX),
        &fmu_api);
    if (status != FWK_SUCCESS) {
        fwk_unexpected();
        return status;
    }

    return FWK_SUCCESS;
}

static int test_fmu_start(fwk_id_t id)
{
    int status = FWK_SUCCESS;

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
static int test_fmu_process_notification(
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

    if (fmu_inject_flag == true) {
        if (event == NULL)
            return FWK_E_PARAM;

        fwk_assert(fwk_id_is_type(event->target_id, FWK_ID_TYPE_MODULE));

        if (!fwk_id_is_equal(event->id, mod_fmu_notification_id_fault))
            return FWK_E_PARAM;

        fmu_notify_params =
            (struct mod_fmu_fault_notification_params *)event->params;

        test_fmu_step_idx++;

        struct fmu_event_packet payload = {
            .suite_idx = 0,
            .step_idx = test_fmu_step_idx,
            .fmu_params = fmu_notify_params,
        };

        memcpy((void *)step_event.params, (void *)&payload, sizeof(payload));

        status = fwk_put_event(&step_event);
        fwk_assert(status == FWK_SUCCESS);
    }

    return status;
}
#endif

struct fwk_module_config config_test_fmu = { 0 };
const struct fwk_module module_test_fmu = {
    .type = FWK_MODULE_TYPE_SERVICE,
    .init = test_fmu_init,
    .start = test_fmu_start,
    .process_bind_request = test_fmu_process_bind_request,
    .api_count = MOD_INTEGRATION_TEST_API_COUNT,
    .bind = test_fmu_bind,
#ifdef BUILD_HAS_NOTIFICATION
    .process_notification = test_fmu_process_notification,
#endif /* BUILD_HAS_NOTIFICATION */
};
