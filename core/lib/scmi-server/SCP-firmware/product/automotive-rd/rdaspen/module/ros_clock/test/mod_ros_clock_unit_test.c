/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "unity.h"

#include <Mockfwk_id.h>
#include <Mockfwk_module.h>

#include <mod_power_domain.h>
#include <mod_ros_clock.h>

#include <fwk_module_idx.h>

#include UNIT_TEST_SRC
#include "config_ros_clock.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Offsets for clock control register.
 */
#define CLK_CONTROL_CLKSELECT_POS 0U
#define CLK_CONTROL_CLKDIV0_POS   4U
#define CLK_CONTROL_CLKDIV1_POS   12U

/*
 * Masks for register.
 */
#define CLK_CONTROL_CLKSELECT_MSK (UINT32_C(0x3) << CLK_CONTROL_CLKSELECT_POS)
#define CLK_CONTROL_CLKDIV0_MSK   (UINT32_C(0xFF) << CLK_CONTROL_CLKDIV0_POS)
#define CLK_CONTROL_CLKDIV1_MSK   (UINT32_C(0xFF) << CLK_CONTROL_CLKDIV1_POS)

const struct mod_clock_drv_api *api;

void setUp(void)
{
    /* Do Nothing */
}

void tearDown(void)
{
    /* Do Nothing */
}

void test_ros_clock(void)
{
    int status;

    /* Initialize the RoS clock module */
    status = ros_clock_init(
        fwk_module_id_ros_clock, CFGD_MOD_ROS_CLOCK_EIDX_COUNT, NULL);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

    /* Get Clock module APIs */
    fwk_id_t api_id = FWK_ID_API(FWK_MODULE_IDX_ROS_CLOCK, 0U);
    fwk_id_t element_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_ROS_CLOCK, 0U);
    fwk_id_is_type_ExpectAndReturn(element_id, FWK_ID_TYPE_ELEMENT, true);
    status = ros_clock_process_bind_request(
        fwk_module_id_fake, element_id, api_id, (const void **)&api);
    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
    TEST_ASSERT_NOT_NULL(api);

    /* Initialize each clock device */
    for (uint32_t idx = 0; idx < CFGD_MOD_ROS_CLOCK_EIDX_COUNT; idx++) {
        uint32_t divider_msk, divider_pos;
        uint32_t expected_register_value = 0U;
        const struct mod_ros_clock_dev_config *dev_config =
            ros_clock_table[idx].data;
        uint32_t source = dev_config->rate_table->source;
        uint32_t clkdiv = dev_config->rate_table->divider - 1U;

        if (source == MOD_ROS_CLOCK_CLOCK_DIVIDER_0) {
            divider_msk = CLK_CONTROL_CLKDIV0_MSK;
            divider_pos = CLK_CONTROL_CLKDIV0_POS;
        } else {
            divider_msk = CLK_CONTROL_CLKDIV1_MSK;
            divider_pos = CLK_CONTROL_CLKDIV1_POS;
        }

        expected_register_value =
            (expected_register_value & ~divider_msk) | (clkdiv << divider_pos);

        expected_register_value =
            (expected_register_value & ~CLK_CONTROL_CLKSELECT_MSK) |
            (source << CLK_CONTROL_CLKSELECT_POS);

        fwk_id_t element_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_ROS_CLOCK, idx);
        fwk_id_get_element_idx_IgnoreAndReturn(idx);
        status = ros_clock_element_init(
            element_id, 1, (const void *)ros_clock_table[idx].data);

        TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

        /* Verify correct value was written to register */
        TEST_ASSERT_EQUAL(
            expected_register_value, *(((uint32_t *)ROS_CLOCK_PTR) + idx + 1U));
    }
}

void test_ros_clock_api(void)
{
    int status;

    for (uint32_t device_idx = 0; device_idx < CFGD_MOD_ROS_CLOCK_EIDX_COUNT;
         device_idx++) {
        struct mod_clock_range range;
        const struct mod_ros_clock_dev_config *dev_config =
            ros_clock_table[device_idx].data;
        fwk_id_t element_id =
            FWK_ID_ELEMENT(FWK_MODULE_IDX_ROS_CLOCK, device_idx);
        fwk_id_get_element_idx_IgnoreAndReturn(device_idx);

        /* Get the range */
        status = api->get_range(element_id, &range);
        TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
        TEST_ASSERT_EQUAL(MOD_CLOCK_RATE_TYPE_DISCRETE, range.rate_type);
        TEST_ASSERT_EQUAL(dev_config->rate_table[0].rate, range.min);
        TEST_ASSERT_EQUAL(
            dev_config->rate_table[dev_config->rate_count - 1U].rate,
            range.max);
        TEST_ASSERT_EQUAL(dev_config->rate_count, range.rate_count);

        for (uint32_t rate_idx = 0; rate_idx < dev_config->rate_count;
             rate_idx++) {
            uint64_t rate;
            enum mod_clock_state state;

            /* Get the rate with index */
            status = api->get_rate_from_index(element_id, rate_idx, &rate);
            TEST_ASSERT_EQUAL(dev_config->rate_table[rate_idx].rate, rate);

            /* Set the rate */
            status = api->set_rate(
                element_id,
                dev_config->rate_table[rate_idx].rate,
                MOD_CLOCK_ROUND_MODE_NONE);
            TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

            /* Get the rate */
            status = api->get_rate(element_id, &rate);
            TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
            TEST_ASSERT_EQUAL(dev_config->rate_table[rate_idx].rate, rate);

            /* Set state */
            status = api->set_state(element_id, MOD_CLOCK_STATE_RUNNING);
            TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

            /* Get state */
            status = api->get_state(element_id, &state);
            TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
            TEST_ASSERT_EQUAL(MOD_CLOCK_STATE_RUNNING, state);
        }
    }
}

int ros_clock_test_main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_ros_clock);
    RUN_TEST(test_ros_clock_api);

    return UNITY_END();
}

#if !defined(TEST_ON_TARGET)
int main(void)
{
    return ros_clock_test_main();
}
#endif
