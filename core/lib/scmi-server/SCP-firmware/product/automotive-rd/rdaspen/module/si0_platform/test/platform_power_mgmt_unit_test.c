/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "unity.h"

#include <Mockfwk_id.h>
#include <Mockfwk_module.h>

#include <mod_si0_platform.h>

#include <fwk_module_idx.h>

#include UNIT_TEST_SRC

#include "config_si0_platform.h"

void setUp(void)
{
}

void tearDown(void)
{
    /* Do Nothing */
}

/*!
 * \brief platform_power_mgmt unit test: platform_power_mgmt_bind(),
 *
 *  \details Test successful bind
 */
void test_platform_power_mgmt_bind_success(void)
{
    int status;

    fwk_module_bind_IgnoreAndReturn(FWK_SUCCESS);

    status = platform_power_mgmt_bind();

    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
}

/*!
 * \brief platform_power_mgmt unit test: platform_power_mgmt_bind(),
 *
 *  \details Test failure in bind
 */
void test_platform_power_mgmt_bind_fail(void)
{
    int status;

    fwk_module_bind_IgnoreAndReturn(FWK_E_DATA);

    status = platform_power_mgmt_bind();

    TEST_ASSERT_EQUAL(status, FWK_E_DATA);
}

int platform_power_mgmt_test_main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_platform_power_mgmt_bind_success);
    RUN_TEST(test_platform_power_mgmt_bind_fail);

    return UNITY_END();
}

int main(void)
{
    return platform_power_mgmt_test_main();
}
