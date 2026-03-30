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
#include <mod_timer.h>

static uint8_t fmu_reg[64 * FWK_KIB] = { 0 };

struct mod_fmu_dev_config fmu_config = {
    .base = (uintptr_t)&fmu_reg[0],
    .parent_cr_index = 0,
    .parent_ncr_index = 1,
};

int wait(
    fwk_id_t dev_id,
    uint32_t microseconds,
    bool (*cond)(void *),
    void *data)
{
    return cond(data) ? FWK_SUCCESS : FWK_E_DATA;
}

struct mod_timer_api timer_api_impl = {
    .wait = wait,
};

void setUp(void)
{
    /* Clear the cluster control registers between tests */
    fwk_str_memset(fmu_reg, 0, sizeof(fmu_reg));
    timer_api = &timer_api_impl;
}

void tearDown(void)
{
    /* Do nothing */
}

void test_fmu_gic_inject(void)
{
    int status;
    uint32_t val;
    struct mod_fmu_fault fault = {
        .device_idx = 0,
        .node_idx = 1,
        .sm_idx = 2,
    };

    status = inject(&fmu_config, &fault);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

    /* Assert fault has been injected */
    val = fwk_mmio_read_32((uintptr_t)fmu_reg + FMU_FIELD_SMERR);
    TEST_ASSERT_EQUAL_HEX(0x10000200, val);
}

void test_fmu_gic_next_fault_simple(void)
{
    bool exists;
    struct mod_fmu_fault fault = {
        .device_idx = 0,
        .node_idx = 1,
        .sm_idx = 2,
    };
    bool fault_tracked = false;
    unsigned int node;

    /* Set up FMU fault response */
    fwk_mmio_write_64(fmu_config.base + FMU_FIELD_ERRGSR, FWK_BIT(3));
    fwk_mmio_write_64(
        fmu_config.base + FMU_FIELD_ERRSTATUS(3),
        (2UL << FMU_ERRSTATUS_SMID_SHIFT) | FMU_ERRSTATUS_V_MASK);

    exists = fault_peek(&fmu_config, &node);
    fault_ack(&fmu_config, &fault, node, &fault_tracked);
    TEST_ASSERT_TRUE(exists);
    TEST_ASSERT_EQUAL(1, fault.node_idx);
    TEST_ASSERT_EQUAL(2, fault.sm_idx);
    TEST_ASSERT_TRUE(fault_tracked);
}

void test_fmu_gic_set_enabled(void)
{
    int status;
    struct mod_fmu_fault fault = {
        .device_idx = 0,
        .node_idx = 2,
        .sm_idx = 1,
    };
    uint32_t val;

    /* Set value to true and read back */
    status = set_enabled(&fmu_config, &fault, true);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

    val = fwk_mmio_read_32(fmu_config.base + FMU_FIELD_SMEN);
    TEST_ASSERT_EQUAL_HEX(0x20000101, val);

    /* Set value to false and read back */
    status = set_enabled(&fmu_config, &fault, false);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

    val = fwk_mmio_read_32(fmu_config.base + FMU_FIELD_SMEN);
    TEST_ASSERT_EQUAL_HEX(0x20000100, val);
}

void test_fmu_gic_set_critical(void)
{
    int status;
    struct mod_fmu_fault fault = {
        .device_idx = 0,
        .node_idx = 2,
        .sm_idx = 1,
    };
    uint32_t val;

    /* Set value to true and read back */
    status = set_critical(&fmu_config, &fault, true);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

    val = fwk_mmio_read_32(fmu_config.base + FMU_FIELD_SMCR);
    TEST_ASSERT_EQUAL_HEX(0x20000101, val);

    /* Set value to false and read back */
    status = set_critical(&fmu_config, &fault, false);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

    val = fwk_mmio_read_32(fmu_config.base + FMU_FIELD_SMCR);
    TEST_ASSERT_EQUAL_HEX(0x20000100, val);
}

int fmu_test_main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_fmu_gic_inject);
    RUN_TEST(test_fmu_gic_next_fault_simple);
    RUN_TEST(test_fmu_gic_set_enabled);
    RUN_TEST(test_fmu_gic_set_critical);

    return UNITY_END();
}

#if !defined(TEST_ON_TARGET)
int main(void)
{
    return fmu_test_main();
}
#endif
