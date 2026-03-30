/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "unity.h"

#include <Mockfwk_id.h>
#include <Mockfwk_module.h>

#include <mod_power_domain.h>
#include <mod_safety_island_platform.h>
#include <mod_transport.h>

#include <fwk_module_idx.h>

#include UNIT_TEST_SRC
#include "config_safety_island_platform.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static int release_transport_channel_lock_success(fwk_id_t channel_id)
{
    return FWK_SUCCESS;
}

static int pd_set_state_success(
    fwk_id_t pd_id,
    bool response_requested,
    uint32_t state)
{
    return FWK_SUCCESS;
}

static struct mod_transport_firmware_api transport_api = {
    .release_transport_channel_lock = release_transport_channel_lock_success,
};
static struct mod_pd_restricted_api pd_restricted_api = {
    .set_state = pd_set_state_success,
};

void setUp(void)
{
    /* Do Nothing */
}

void tearDown(void)
{
    /* Do Nothing */
}

/*!
 * \brief Safety Island Platform unit test: safety_platform_mod_init(),
          invalid number of safety island clusters.
 *
 *  \details Handle case in safety_platform_mod_init() where number of
 *           safety island clusters is passed.
 */
void test_safety_island_platform_mod_init_fail(void)
{
    int status = 0;

    status = safety_island_platform_mod_init(
        fwk_module_id_safety_island_platform, 0, NULL);

    TEST_ASSERT_EQUAL(status, FWK_E_PARAM);
}

/*!
 * \brief Safety Island Platform unit test: safety_platform_mod_init(),
          valid number of safety island clusters.
 *
 *  \details Handle case in safety_platform_mod_init() where valid number of
 *           safety island clusters is passed.
 */
void test_safety_island_platform_mod_init_success(void)
{
    int status;

    status = safety_island_platform_mod_init(
        fwk_module_id_safety_island_platform, SI_CL_COUNT, NULL);

    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
}

/*!
 * \brief Safety Island Platform unit test:
 *        safety_island_platform_cluster_init(), invalid config data.
 *
 *  \details Handle case in safety_island_platform_cluster_init() where invalid
 *           config data is passed.
 */
void test_safety_island_platform_cluster_init_fail(void)
{
    int status = 0;

    status = safety_island_platform_mod_init(
        fwk_module_id_safety_island_platform, SI_CL_COUNT, NULL);

    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);

    status = safety_island_platform_cluster_init(
        FWK_ID_ELEMENT(FWK_MODULE_IDX_SAFETY_ISLAND_PLATFORM, 0), 1, NULL);

    TEST_ASSERT_EQUAL(status, FWK_E_DATA);
}

/*!
 * \brief Safety Island Platform unit test:
 *        safety_island_platform_cluster_init(), valid config data.
 *
 *  \details Handle case in safety_island_platform_cluster_init() where valid
 *           config data is passed.
 */
void test_safety_island_platform_cluster_init_success(void)
{
    int status = 0;
    uint32_t idx;

    status = safety_island_platform_mod_init(
        fwk_module_id_safety_island_platform, SI_CL_COUNT, NULL);

    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);

    for (idx = 0; idx < SI_CL_COUNT; idx++) {
        fwk_id_t element_id =
            FWK_ID_ELEMENT(FWK_MODULE_IDX_SAFETY_ISLAND_PLATFORM, idx);
        fwk_id_get_element_idx_ExpectAndReturn(element_id, idx);
        status = safety_island_platform_cluster_init(
            element_id,
            1,
            (const void *)safety_island_platform_element_table[idx].data);

        TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
    }
}

/*!
 * \brief Safety Island Platform unit test: safety_island_platform_bind(),
          module binding failure.
 *
 *  \details Handle case in safety_island_platform_bind() when module binding
 *           fails
 */
void test_safety_island_platform_bind_fail(void)
{
    int status;
    fwk_id_t element_id =
        FWK_ID_ELEMENT(FWK_MODULE_IDX_SAFETY_ISLAND_PLATFORM, 0U);

    fwk_id_is_type_ExpectAndReturn(element_id, FWK_ID_TYPE_MODULE, false);
    fwk_id_get_element_idx_ExpectAndReturn(element_id, 0U);
    fwk_module_bind_ExpectAnyArgsAndReturn(1);

    status = safety_island_platform_bind(element_id, 0U);

    TEST_ASSERT_NOT_EQUAL(status, FWK_SUCCESS);
}

/*!
 * \brief Safety Island Platform unit test: safety_island_platform_bind(),
          valid config data and arguments.
 *
 *  \details Handle case in safety_island_platform_bind() where valid config
 *           data is passed and arguments are passed.
 */
void test_safety_island_platform_bind_success(void)
{
    int status;
    fwk_id_t element_id =
        FWK_ID_ELEMENT(FWK_MODULE_IDX_SAFETY_ISLAND_PLATFORM, 0U);

    /* Attempt binding with a non zero round */
    status = safety_island_platform_bind(element_id, 1U);
    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);

    /* Attempt binding for the module and not a cluster */
    fwk_id_is_type_ExpectAndReturn(
        fwk_module_id_safety_island_platform, FWK_ID_TYPE_MODULE, true);
    status =
        safety_island_platform_bind(fwk_module_id_safety_island_platform, 0U);
    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);

    /* Bind the safety island cluster correctly */
    fwk_id_is_type_ExpectAndReturn(element_id, FWK_ID_TYPE_MODULE, false);
    fwk_id_get_element_idx_ExpectAndReturn(element_id, 0U);
    fwk_module_bind_IgnoreAndReturn(FWK_SUCCESS);
    status = safety_island_platform_bind(element_id, 0U);
    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
}

/*!
 * \brief Safety Island Platform unit test:
          safety_island_platform_process_bind_request(), invalid arguments.
 *
 *  \details Handle case in safety_island_platform_process_bind_request() where
 *           invalid arguments are passed.
 */
void test_safety_island_platform_process_bind_request_fail(void)
{
    int status;
    const void *api = NULL;
    fwk_id_t api_id = FWK_ID_API(
        FWK_MODULE_IDX_SAFETY_ISLAND_PLATFORM,
        MOD_SAFETY_ISLAND_PLATFORM_API_COUNT);
    fwk_id_is_type_ExpectAndReturn(
        fwk_module_id_safety_island_platform, FWK_ID_TYPE_ELEMENT, false);
    status = safety_island_platform_process_bind_request(
        fwk_module_id_fake, fwk_module_id_safety_island_platform, api_id, &api);
    TEST_ASSERT_EQUAL(status, FWK_E_ACCESS);
    TEST_ASSERT_NULL(api);
}

/*!
 * \brief Safety Island Platform unit test:
          safety_island_platform_process_bind_request(), valid config data
          and arguments.
 *
 *  \details Handle case in safety_island_platform_process_bind_request() where
 *           valid config and arguments are passed.
 */
void test_safety_island_platform_process_bind_request_success(void)
{
    int status;
    uint32_t idx;

    /* Initialize the module */
    status = safety_island_platform_mod_init(
        fwk_module_id_safety_island_platform, SI_CL_COUNT, NULL);

    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);

    for (idx = 0; idx < SI_CL_COUNT; idx++) {
        const struct safety_island_cluster_config *config =
            safety_island_platform_element_table[idx].data;
        fwk_id_t api_id = FWK_ID_API(
            FWK_MODULE_IDX_SAFETY_ISLAND_PLATFORM,
            MOD_SAFETY_ISLAND_PLATFORM_API_COUNT);
        const struct mod_transport_firmware_signal_api *api;
        fwk_id_t element_id =
            FWK_ID_ELEMENT(FWK_MODULE_IDX_SAFETY_ISLAND_PLATFORM, idx);

        /* Intiailize the Safety Island cluster */
        fwk_id_get_element_idx_ExpectAndReturn(element_id, idx);
        status = safety_island_platform_cluster_init(
            element_id,
            1,
            (const void *)safety_island_platform_element_table[idx].data);
        TEST_ASSERT_EQUAL(status, FWK_SUCCESS);

        /* Bind the safety island cluster */
        fwk_id_is_type_ExpectAndReturn(element_id, FWK_ID_TYPE_MODULE, false);
        fwk_id_get_element_idx_ExpectAndReturn(element_id, idx);
        fwk_module_bind_IgnoreAndReturn(FWK_SUCCESS);
        status = safety_island_platform_bind(element_id, 0U);
        TEST_ASSERT_EQUAL(status, FWK_SUCCESS);

        /* Call Process Bind Request on the Safety Island Cluster */
        fwk_id_is_type_ExpectAndReturn(element_id, FWK_ID_TYPE_ELEMENT, true);
        fwk_id_get_element_idx_ExpectAndReturn(element_id, idx);
        fwk_id_get_api_idx_ExpectAndReturn(
            api_id, MOD_SAFETY_ISLAND_PLATFORM_API_IDX_BOOT_CLUSTER);
        fwk_id_is_equal_ExpectAndReturn(
            fwk_module_id_fake, config->transport_id, true);

        /* Set Transport and Power Domain APIs */
        ctx.safety_island_ctx_table[idx].transport_api = &transport_api;
        ctx.safety_island_ctx_table[idx].pd_restricted_api = &pd_restricted_api;

        status = safety_island_platform_process_bind_request(
            fwk_module_id_fake, element_id, api_id, (const void **)&api);
        TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
        TEST_ASSERT_NOT_NULL(api);

        /* Test signal_message */
        fwk_id_get_element_idx_IgnoreAndReturn(idx);
        fwk_module_get_element_name_IgnoreAndReturn("Test");
        status = api->signal_message(api_id);
        TEST_ASSERT_EQUAL(status, FWK_SUCCESS);

        /* Test signal_error */
        fwk_id_get_element_idx_IgnoreAndReturn(idx);
        fwk_module_get_element_name_IgnoreAndReturn("Test");
        status = api->signal_error(api_id);
        TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
    }
}

int safety_island_platform_test_main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_safety_island_platform_mod_init_fail);
    RUN_TEST(test_safety_island_platform_mod_init_success);
    RUN_TEST(test_safety_island_platform_cluster_init_fail);
    RUN_TEST(test_safety_island_platform_cluster_init_success);
    RUN_TEST(test_safety_island_platform_bind_fail);
    RUN_TEST(test_safety_island_platform_bind_success);
    RUN_TEST(test_safety_island_platform_process_bind_request_fail);
    RUN_TEST(test_safety_island_platform_process_bind_request_success);

    return UNITY_END();
}

int main(void)
{
    return safety_island_platform_test_main();
}
