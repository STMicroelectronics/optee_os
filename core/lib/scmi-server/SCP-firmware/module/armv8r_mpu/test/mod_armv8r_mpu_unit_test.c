/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "scp_unity.h"
#include "unity.h"

#include <Mockarmv8r_mpu_reg.h>
#include <Mockfwk_id.h>
#include <config_armv8r_mpu.h>

#include UNIT_TEST_SRC

void setUp(void)
{
}

void tearDown(void)
{
}

void test_mod_armv8r_mpu_insufficient_attributes(void)
{
    fwk_id_t id;
    int status;

    static uint8_t mem_attributes[] = {
        0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF,
    };

    const struct fwk_module_config config_armv8r_mpu_max_regions = {
        .data = &((struct mod_armv8r_mpu_config){
            .attributes_count = FWK_ARRAY_SIZE(mem_attributes),
            .attributes = mem_attributes,
            .region_count = 0,
            .regions = NULL,
        }),
    };

    status = armv8r_mpu_init(id, 0, config_armv8r_mpu_max_regions.data);
    TEST_ASSERT_EQUAL(status, FWK_E_RANGE);
}

void test_mod_armv8r_mpu_insufficient_regions(void)
{
    fwk_id_t id;
    int status;

    read_mpuir_el2_ExpectAndReturn(1);

    status = armv8r_mpu_init(id, 0, config_armv8r_mpu_ut.data);
    TEST_ASSERT_EQUAL(status, FWK_E_RANGE);
}

void test_mod_armv8r_mpu_init(void)
{
    fwk_id_t id;
    int status;
    uint8_t index;

    barrier_dsync_fence_full_Ignore();
    barrier_isync_fence_full_Ignore();

    /* MPU region count check */
    read_mpuir_el2_ExpectAndReturn(32);

    /* Disable MPU */
    read_sctlr_el2_ExpectAndReturn(0x0);
    write_sctlr_el2_Expect(0x0);

    /* Set up memory attributes */
    read_mair_el2_ExpectAndReturn(0x0);
    write_mair_el2_Expect(0xff);
    read_mair_el2_ExpectAndReturn(0xff);
    write_mair_el2_Expect(0xff);

    /* Set up region 0 */
    write_prselr_el2_Expect(0x0);
    write_prbar_el2_Expect(0x0);
    write_prlar_el2_Expect(0x7fffffC1);

    /* Set up region 1 */
    write_prselr_el2_Expect(0x1);
    write_prbar_el2_Expect(0x80000000);
    write_prlar_el2_Expect(0xffffffC3);

    /* Clear remaining regions */
    for (index = 2; index < 32; index++) {
        write_prselr_el2_Expect(index);
        write_prbar_el2_Expect(0);
        write_prlar_el2_Expect(0);
    }

    /* Enable MPU */
    read_sctlr_el2_ExpectAndReturn(0x0);
    write_sctlr_el2_Expect(0x1);

    status = armv8r_mpu_init(id, 0, config_armv8r_mpu_ut.data);
    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);

    barrier_dsync_fence_full_StopIgnore();
    barrier_isync_fence_full_StopIgnore();
}

int template_test_main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_mod_armv8r_mpu_insufficient_attributes);
    RUN_TEST(test_mod_armv8r_mpu_insufficient_regions);
    RUN_TEST(test_mod_armv8r_mpu_init);
    return UNITY_END();
}

#if !defined(TEST_ON_TARGET)
int main(void)
{
    return template_test_main();
}
#endif
