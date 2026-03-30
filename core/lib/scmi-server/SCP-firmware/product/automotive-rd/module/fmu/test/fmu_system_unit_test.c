/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "scp_unity.h"
#include "unity.h"

#include <Mockfwk_interrupt.h>
#include <Mockfwk_module.h>

#include <fwk_macros.h>
#include <fwk_mm.h>
#include <fwk_string.h>

#include UNIT_TEST_SRC

#include <mod_fmu.h>

static uint8_t fmu_reg[64 * FWK_KIB] = { 0 };

struct mod_fmu_dev_config fmu_config = {
    .base = (uintptr_t)fmu_reg,
    .parent_cr_index = 0,
    .parent_ncr_index = 1,
};

void setUp(void)
{
    /* Clear the cluster control registers between tests */
    fwk_str_memset(fmu_reg, 0, sizeof(fmu_reg));
}

void tearDown(void)
{
    /* Do nothing */
}

void test_fmu_system_inject(void)
{
    int status;
    uint32_t val;
    struct mod_fmu_fault fault = {
        .device_idx = 0,
        .node_idx = 1,
        .sm_idx = MOD_FMU_SM_SYSTEM_INPUT_ERROR,
    };

    status = inject(&fmu_config, &fault);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

    /* Assert fault has been injected */
    val = fwk_mmio_read_32((uintptr_t)fmu_reg + FMU_FIELD_ERRIMPDEF(1));
    TEST_ASSERT_BITS(
        FMU_ERRIMPDEF_IE_MASK,
        MOD_FMU_SM_SYSTEM_INPUT_ERROR << FMU_ERRIMPDEF_IE_SHIFT,
        val);
}

void test_fmu_system_next_fault_simple(void)
{
    bool exists;
    struct mod_fmu_fault fault = {
        .device_idx = 0,
        .node_idx = 1,
        .sm_idx = MOD_FMU_SM_SYSTEM_INPUT_ERROR,
    };
    bool fault_tracked = false;
    unsigned int node;

    /* Set up FMU fault response */
    fwk_mmio_write_32((uintptr_t)fmu_reg + FMU_FIELD_ERRGSR_L(0), FWK_BIT(1));
    fwk_mmio_write_32(
        (uintptr_t)fmu_reg + FMU_FIELD_ERR_STATUS(1),
        MOD_FMU_SM_SYSTEM_INPUT_ERROR << FMU_ERR_STATUS_IERR_SHIFT);

    exists = fault_peek(&fmu_config, &node);
    fault_ack(&fmu_config, &fault, node, &fault_tracked);
    TEST_ASSERT_TRUE(exists);
    TEST_ASSERT_EQUAL(1, fault.node_idx);
    TEST_ASSERT_EQUAL(MOD_FMU_SM_SYSTEM_INPUT_ERROR, fault.sm_idx);
    TEST_ASSERT_TRUE(fault_tracked);
}

void test_fmu_system_next_fault_upstream(void)
{
    bool exists;
    unsigned int node;

    exists = fault_peek(&fmu_config, &node);
    TEST_ASSERT_FALSE(exists);
}

void test_fmu_system_set_enabled(void)
{
    int status;
    bool enabled;
    struct mod_fmu_fault fault = {
        .device_idx = 0,
        .node_idx = 2,
        .sm_idx = MOD_FMU_SM_ALL,
    };

    /* Set value to true and read back */
    status = set_enabled(&fmu_config, &fault, true);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

    status = get_enabled(&fmu_config, &fault, &enabled);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL(true, enabled);

    /* Set value to false and read back */
    status = set_enabled(&fmu_config, &fault, false);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

    status = get_enabled(&fmu_config, &fault, &enabled);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL(false, enabled);
}

void test_fmu_system_set_count(void)
{
    int status;
    uint8_t count;

    status = set_count(&fmu_config, 12, 13);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

    status = get_count(&fmu_config, 12, &count);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL(13, count);
}

void test_fmu_system_set_threshold(void)
{
    int status;
    uint8_t count;

    status = set_threshold(&fmu_config, 34, 25);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

    status = get_threshold(&fmu_config, 34, &count);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL(25, count);
}

void test_fmu_system_set_upgrade_enabled(void)
{
    int status;
    bool enabled;

    /* Set value to true and read back */
    status = set_upgrade_enabled(&fmu_config, 101, true);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

    status = get_upgrade_enabled(&fmu_config, 101, &enabled);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL(true, enabled);

    /* Set value to false and read back */
    status = set_upgrade_enabled(&fmu_config, 101, false);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

    status = get_upgrade_enabled(&fmu_config, 101, &enabled);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL(false, enabled);
}

int fmu_test_main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_fmu_system_inject);
    RUN_TEST(test_fmu_system_next_fault_simple);
    RUN_TEST(test_fmu_system_next_fault_upstream);
    RUN_TEST(test_fmu_system_set_enabled);
    RUN_TEST(test_fmu_system_set_count);
    RUN_TEST(test_fmu_system_set_threshold);
    RUN_TEST(test_fmu_system_set_upgrade_enabled);

    return UNITY_END();
}

#if !defined(TEST_ON_TARGET)
int main(void)
{
    return fmu_test_main();
}
#endif
