/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "unity.h"

#include <Mockfwk_id.h>
#include <Mockfwk_module.h>
#include <Mockfwk_notification.h>

#include <mod_cluster_control.h>

#include <fwk_mmio.h>
#include <fwk_module_idx.h>
#include <fwk_string.h>

#include UNIT_TEST_SRC
#include "config_cluster_control.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void setUp(void)
{
    /* Clear the cluster control registers between tests */
    fwk_str_memset(cluster_control_reg, 0, sizeof(cluster_control_reg));
}

void tearDown(void)
{
    /* Do Nothing */
}

static void validate_rvbars(void)
{
    unsigned int cluster_idx;
    uint32_t rvbar;

    for (cluster_idx = 0; cluster_idx < FWK_ARRAY_SIZE(cluster_control_reg);
         cluster_idx++) {
        rvbar = fwk_mmio_read_32(
            (uintptr_t)(cluster_control_reg[cluster_idx] + 0x100));
        TEST_ASSERT_EQUAL(rvbar, 0xABABABAB);
        rvbar = fwk_mmio_read_32(
            (uintptr_t)(cluster_control_reg[cluster_idx] + 0x104));
        TEST_ASSERT_EQUAL(rvbar, 0xCDCDCDCD);
        rvbar = fwk_mmio_read_32(
            (uintptr_t)(cluster_control_reg[cluster_idx] + 0x108));
        TEST_ASSERT_EQUAL(rvbar, 0xABABABAB);
        rvbar = fwk_mmio_read_32(
            (uintptr_t)(cluster_control_reg[cluster_idx] + 0x10C));
        TEST_ASSERT_EQUAL(rvbar, 0xCDCDCDCD);
        rvbar = fwk_mmio_read_32(
            (uintptr_t)(cluster_control_reg[cluster_idx] + 0x110));
        TEST_ASSERT_EQUAL(rvbar, 0xABABABAB);
        rvbar = fwk_mmio_read_32(
            (uintptr_t)(cluster_control_reg[cluster_idx] + 0x114));
        TEST_ASSERT_EQUAL(rvbar, 0xCDCDCDCD);
        rvbar = fwk_mmio_read_32(
            (uintptr_t)(cluster_control_reg[cluster_idx] + 0x118));
        TEST_ASSERT_EQUAL(rvbar, 0xABABABAB);
        rvbar = fwk_mmio_read_32(
            (uintptr_t)(cluster_control_reg[cluster_idx] + 0x11C));
        TEST_ASSERT_EQUAL(rvbar, 0xCDCDCDCD);
    }
}

void test_cluster_control_direct(void)
{
    int status;

    fwk_module_get_data_ExpectAndReturn(
        fwk_module_id_cluster_control, &cluster_control_config_direct);
    fwk_id_type_is_valid_IgnoreAndReturn(false);

    status = cluster_control_start(fwk_module_id_cluster_control);
    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);

    validate_rvbars();
}

void test_cluster_control_notification(void)
{
    int status;
    struct fwk_event event;

    fwk_module_get_data_ExpectAndReturn(
        fwk_module_id_cluster_control, &cluster_control_config_notification);
    fwk_id_type_is_valid_ExpectAndReturn(fwk_module_id_test_module, true);
    fwk_id_is_equal_ExpectAndReturn(
        fwk_module_id_test_module, FWK_ID_NONE, false);

    fwk_notification_subscribe_ExpectAndReturn(
        test_module_notification_test,
        fwk_module_id_test_module,
        fwk_module_id_cluster_control,
        FWK_SUCCESS);

    status = cluster_control_start(fwk_module_id_cluster_control);
    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);

    event = (struct fwk_event){
        .target_id = fwk_module_id_cluster_control,
        .source_id = fwk_module_id_test_module,
        .id = test_module_notification_test,
    };
    fwk_module_get_data_ExpectAndReturn(
        fwk_module_id_cluster_control, &cluster_control_config_notification);
    fwk_id_is_type_ExpectAndReturn(
        fwk_module_id_cluster_control, FWK_ID_TYPE_MODULE, true);
    fwk_id_is_equal_ExpectAndReturn(
        test_module_notification_test, test_module_notification_test, true);
    fwk_notification_unsubscribe_ExpectAndReturn(
        test_module_notification_test,
        fwk_module_id_test_module,
        fwk_module_id_cluster_control,
        FWK_SUCCESS);
    status = cluster_control_process_notification(&event, NULL);
    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);

    validate_rvbars();
}

int cluster_control_test_main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_cluster_control_direct);
    RUN_TEST(test_cluster_control_notification);

    return UNITY_END();
}

#if !defined(TEST_ON_TARGET)
int main(void)
{
    return cluster_control_test_main();
}
#endif
