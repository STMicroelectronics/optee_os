/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "internal/fmu_common.h"
#include "scp_unity.h"
#include "unity.h"

#include <Mockfwk_interrupt.h>
#include <Mockfwk_module.h>
#include <Mockfwk_notification.h>

#include <fwk_macros.h>
#include <fwk_mm.h>
#include <fwk_string.h>

#include UNIT_TEST_SRC
#include <config_fmu.h>

#include <mod_fmu.h>

static bool system_fault_peek(
    const struct mod_fmu_dev_config *config,
    unsigned int *node_idx)
{
    static unsigned int counter = 0;
    if (counter == 0) {
        counter = 1;
        *node_idx = 33;
        return true;
    }
    return false;
}

static void system_fault_ack(
    const struct mod_fmu_dev_config *config,
    struct mod_fmu_fault *fault,
    unsigned int node_idx,
    bool *fault_tracked)
{
    fault->node_idx = 33;
    fault->sm_idx = 17;
}

static int system_inject(
    const struct mod_fmu_dev_config *config,
    const struct mod_fmu_fault *fault)
{
    return FWK_SUCCESS;
}

static uint8_t s_threshold;

static int system_set_threshold(
    const struct mod_fmu_dev_config *config,
    uint16_t node_id,
    uint8_t threshold)
{
    s_threshold = threshold;

    return FWK_SUCCESS;
}

static int system_get_threshold(
    const struct mod_fmu_dev_config *config,
    uint16_t node_id,
    uint8_t *threshold)
{
    *threshold = s_threshold;

    return FWK_SUCCESS;
}

struct mod_fmu_impl_api mod_fmu_system_api = {
    .fault_peek = system_fault_peek,
    .fault_ack = system_fault_ack,
    .inject = system_inject,
    .set_threshold = system_set_threshold,
    .get_threshold = system_get_threshold,
};

struct mod_fmu_impl_api mod_fmu_gic_mhu_api = { 0 };

void setUp(void)
{
    /* Do nothing */
}

void tearDown(void)
{
    /* Do nothing */
}

void test_fmu_init(void)
{
    int status;

    status = fmu_init(fwk_module_id_fmu, SCP_FMU_COUNT, config_fmu.data);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

    TEST_ASSERT_EQUAL(SCP_FMU_COUNT, ctx.num_devices);
}

void test_fmu_device_init(void)
{
    int status;
    unsigned int idx;
    fwk_id_t element_id;

    for (idx = 0; idx < SCP_FMU_COUNT; idx++) {
        element_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_FMU, idx);
        status = fmu_device_init(element_id, 0, fmu_devices[idx].data);
        TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

        TEST_ASSERT_EQUAL(fmu_devices[idx].data, ctx.device_config[idx]);
    }
}

void test_fmu_start(void)
{
    int status;

    fwk_module_get_data_ExpectAndReturn(fwk_module_id_fmu, config_fmu.data);

    fwk_interrupt_set_isr_ExpectAndReturn(12, fmu_isr_critical, FWK_SUCCESS);
    fwk_interrupt_set_isr_ExpectAndReturn(
        13, fmu_isr_non_critical, FWK_SUCCESS);
    fwk_interrupt_enable_ExpectAndReturn(12, FWK_SUCCESS);
    fwk_interrupt_enable_ExpectAndReturn(13, FWK_SUCCESS);

    status = fmu_start(fwk_module_id_fmu);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void test_fmu_process_bind_request(void)
{
    int status;
    uintptr_t api;

    status = fmu_process_bind_request(
        fwk_module_id_fake,
        fwk_module_id_fmu,
        FWK_ID_API(FWK_MODULE_IDX_FMU, MOD_FMU_DEVICE_API_IDX),
        (void *)&api);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL(&fmu_api, api);
}

void test_fmu_isr(void)
{
    struct fwk_event event = { 0 };
    struct mod_fmu_fault_notification_params *params;

    event.id = mod_fmu_notification_id_fault;
    event.source_id = fwk_module_id_fmu;
    params = (struct mod_fmu_fault_notification_params *)event.params;
    params->critical = true;
    params->fault.device_idx = 0;
    params->fault.node_idx = 33;
    params->fault.sm_idx = 17;
    fwk_notification_notify_ExpectAndReturn(&event, NULL, FWK_SUCCESS);
    fwk_notification_notify_IgnoreArg_count();
    fmu_isr(true);
}

void test_fmu_set_count_unsupported(void)
{
    int status;
    fwk_id_t id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_FMU, SCP_FMU_ROOT);

    status = fmu_api.set_count(id, 12, 13);
    TEST_ASSERT_EQUAL(FWK_E_SUPPORT, status);
}

void test_fmu_set_threshold(void)
{
    int status;
    uint8_t count;
    fwk_id_t id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_FMU, SCP_FMU_ROOT);

    status = fmu_api.set_threshold(id, 34, 25);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

    status = fmu_api.get_threshold(id, 34, &count);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL(25, count);
}

int fmu_test_main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_fmu_init);
    RUN_TEST(test_fmu_device_init);
    RUN_TEST(test_fmu_start);
    RUN_TEST(test_fmu_process_bind_request);
    RUN_TEST(test_fmu_isr);
    RUN_TEST(test_fmu_set_count_unsupported);
    RUN_TEST(test_fmu_set_threshold);

    return UNITY_END();
}

#if !defined(TEST_ON_TARGET)
int main(void)
{
    return fmu_test_main();
}
#endif
