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

#include <mod_ssu.h>

#include <fwk_mmio.h>
#include <fwk_module_idx.h>
#include <fwk_string.h>

#include UNIT_TEST_SRC
#include "config_ssu.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void setUp(void)
{
    /* Do Nothing */
}

void tearDown(void)
{
    /* Do Nothing */
}

void test_ssu(void)
{
    int status;

    /* Initialize the SSU module */
    status = ssu_init(fwk_module_id_ssu, CONFIG_SSU_ELEMENT_IDX_COUNT, NULL);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

    fwk_id_t element_id =
        FWK_ID_ELEMENT(FWK_MODULE_IDX_SSU, CONFIG_SSU_ELEMENT_IDX);

    fwk_module_is_valid_element_id_IgnoreAndReturn(true);
    fwk_id_get_element_idx_ExpectAndReturn(element_id, 0);

    status = ssu_element_init(
        element_id,
        1,
        (const void *)ssu_element_desc_table[CONFIG_SSU_ELEMENT_IDX].data);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void test_ssu_sys_api(void)
{
    static const struct mod_ssu_sys_register_api *api;
    int status;
    uint32_t state;

    /* Get SSU module Sys APIs */
    fwk_id_t api_id = FWK_ID_API(FWK_MODULE_IDX_SSU, MOD_SSU_SYS_API_IDX);
    fwk_id_t element_id =
        FWK_ID_ELEMENT(FWK_MODULE_IDX_SSU, CONFIG_SSU_ELEMENT_IDX);

    fwk_id_is_type_ExpectAndReturn(element_id, FWK_ID_TYPE_ELEMENT, true);
    fwk_id_get_api_idx_ExpectAndReturn(api_id, MOD_SSU_SYS_API_IDX);
    status = ssu_process_bind_request(
        fwk_module_id_fake, element_id, api_id, (const void **)&api);
    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
    TEST_ASSERT_NOT_NULL(api);

    fwk_id_get_element_idx_ExpectAndReturn(element_id, 0);

    /* Change SSU state to MOD_SSU_FSM_SAFE_STATE */
    api->set_sys_ctrl(element_id, MOD_SSU_FSM_SAFE_STATE);
    TEST_ASSERT_EQUAL(
        MOD_SSU_FSM_SAFE_STATE, ((struct ssu_reg *)SSU_REG_PTR)->SYS_CTRL);

    fwk_id_get_element_idx_ExpectAndReturn(element_id, 0);

    /* Change SSU state to MOD_SSU_FSM_NCE_STATE */
    api->set_sys_ctrl(element_id, MOD_SSU_FSM_NCE_STATE);
    TEST_ASSERT_EQUAL(
        MOD_SSU_FSM_NCE_STATE, ((struct ssu_reg *)SSU_REG_PTR)->SYS_CTRL);

    fwk_id_get_element_idx_ExpectAndReturn(element_id, 0);

    /* Change SSU state to MOD_SSU_FSM_SAFE_STATE */
    api->set_sys_ctrl(element_id, MOD_SSU_FSM_CE_STATE);
    TEST_ASSERT_EQUAL(
        MOD_SSU_FSM_CE_STATE, ((struct ssu_reg *)SSU_REG_PTR)->SYS_CTRL);

    fwk_id_get_element_idx_ExpectAndReturn(element_id, 0);

    /* Change SSU state to Invalid state, Negative test */
    status = api->set_sys_ctrl(element_id, INVALID_FSM_STATE);
    TEST_ASSERT_EQUAL(status, FWK_E_PARAM);

    fwk_id_get_element_idx_ExpectAndReturn(element_id, 0);

    api->get_sys_status(element_id, &state);
    TEST_ASSERT_EQUAL(state, ((struct ssu_reg *)SSU_REG_PTR)->SYS_STATUS);
}

void test_ssu_error_control_api(void)
{
    static const struct mod_ssu_error_control_status_api *api;
    int status;

    /* Get SSU module Error control APIs */
    fwk_id_t api_id = FWK_ID_API(FWK_MODULE_IDX_SSU, MOD_SSU_ERR_API_IDX);
    fwk_id_t element_id =
        FWK_ID_ELEMENT(FWK_MODULE_IDX_SSU, CONFIG_SSU_ELEMENT_IDX);
    fwk_id_is_type_ExpectAndReturn(element_id, FWK_ID_TYPE_ELEMENT, true);
    fwk_id_get_api_idx_ExpectAndReturn(api_id, MOD_SSU_ERR_API_IDX);
    status = ssu_process_bind_request(
        fwk_module_id_fake, element_id, api_id, (const void **)&api);
    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
    TEST_ASSERT_NOT_NULL(api);

    fwk_id_get_element_idx_ExpectAndReturn(element_id, 0);

    api->set_err_ctrl_enable(element_id, true);
    TEST_ASSERT_EQUAL(ERROR_ENABLE, ((struct ssu_reg *)SSU_REG_PTR)->ERR_CTRL);

    fwk_id_get_element_idx_ExpectAndReturn(element_id, 0);

    api->set_err_ctrl_enable(element_id, false);
    TEST_ASSERT_EQUAL(ERROR_DISABLE, ((struct ssu_reg *)SSU_REG_PTR)->ERR_CTRL);

    fwk_id_get_element_idx_ExpectAndReturn(element_id, 0);

    /* Enable MOD_SSU_ERR_CR_EN */
    api->err_detect_control(element_id, MOD_SSU_ERR_CR_EN, 0x1);
    TEST_ASSERT_EQUAL(0x1, ((struct ssu_reg *)SSU_REG_PTR)->ERR_IMPDEF);

    fwk_id_get_element_idx_ExpectAndReturn(element_id, 0);

    /* Disable MOD_SSU_ERR_CR_EN */
    api->err_detect_control(element_id, MOD_SSU_ERR_CR_EN, 0x0);
    TEST_ASSERT_EQUAL(0x0, ((struct ssu_reg *)SSU_REG_PTR)->ERR_IMPDEF);

    fwk_id_get_element_idx_ExpectAndReturn(element_id, 0);

    /* Enable MOD_SSU_ERR_NCR_EN */
    api->err_detect_control(element_id, MOD_SSU_ERR_NCR_EN, 0x1);
    TEST_ASSERT_EQUAL(0x2, ((struct ssu_reg *)SSU_REG_PTR)->ERR_IMPDEF);

    fwk_id_get_element_idx_ExpectAndReturn(element_id, 0);

    /* Disable MOD_SSU_ERR_NCR_EN */
    api->err_detect_control(element_id, MOD_SSU_ERR_NCR_EN, 0x0);
    TEST_ASSERT_EQUAL(0x0, ((struct ssu_reg *)SSU_REG_PTR)->ERR_IMPDEF);

    fwk_id_get_element_idx_ExpectAndReturn(element_id, 0);

    /* Enable MOD_SSU_ERR_APB_SW */
    api->err_detect_control(element_id, MOD_SSU_ERR_APB_SW, 0x1);
    TEST_ASSERT_EQUAL(0x10, ((struct ssu_reg *)SSU_REG_PTR)->ERR_IMPDEF);

    fwk_id_get_element_idx_ExpectAndReturn(element_id, 0);

    /* Disable MOD_SSU_ERR_APB_SW */
    api->err_detect_control(element_id, MOD_SSU_ERR_APB_SW, 0x0);
    TEST_ASSERT_EQUAL(0x0, ((struct ssu_reg *)SSU_REG_PTR)->ERR_IMPDEF);

    fwk_id_get_element_idx_ExpectAndReturn(element_id, 0);

    /* Enable MOD_SSU_ERR_APB_INC_SEQ */
    api->err_detect_control(element_id, MOD_SSU_ERR_APB_INC_SEQ, 0x1);
    TEST_ASSERT_EQUAL(0x20, ((struct ssu_reg *)SSU_REG_PTR)->ERR_IMPDEF);

    fwk_id_get_element_idx_ExpectAndReturn(element_id, 0);

    /* Disable MOD_SSU_ERR_APB_INC_SEQ */
    api->err_detect_control(element_id, MOD_SSU_ERR_APB_INC_SEQ, 0x0);
    TEST_ASSERT_EQUAL(0x0, ((struct ssu_reg *)SSU_REG_PTR)->ERR_IMPDEF);

    fwk_id_get_element_idx_ExpectAndReturn(element_id, 0);

    /* Enable MOD_SSU_ERR_APB_PARITY */
    api->err_detect_control(element_id, MOD_SSU_ERR_APB_PARITY, 0x1);
    TEST_ASSERT_EQUAL(0x40, ((struct ssu_reg *)SSU_REG_PTR)->ERR_IMPDEF);

    fwk_id_get_element_idx_ExpectAndReturn(element_id, 0);

    /* Disable MOD_SSU_ERR_APB_PARITY */
    api->err_detect_control(element_id, MOD_SSU_ERR_APB_PARITY, 0x0);
    TEST_ASSERT_EQUAL(0x0, ((struct ssu_reg *)SSU_REG_PTR)->ERR_IMPDEF);

    fwk_id_get_element_idx_ExpectAndReturn(element_id, 0);

    /* Enable all bits in Error control register */
    api->err_detect_control(element_id, MOD_SSU_ERR_CR_EN, 0x1);
    fwk_id_get_element_idx_ExpectAndReturn(element_id, 0);
    api->err_detect_control(element_id, MOD_SSU_ERR_NCR_EN, 0x1);
    fwk_id_get_element_idx_ExpectAndReturn(element_id, 0);
    api->err_detect_control(element_id, MOD_SSU_ERR_APB_SW, 0x1);
    fwk_id_get_element_idx_ExpectAndReturn(element_id, 0);
    api->err_detect_control(element_id, MOD_SSU_ERR_APB_INC_SEQ, 0x1);
    fwk_id_get_element_idx_ExpectAndReturn(element_id, 0);
    api->err_detect_control(element_id, MOD_SSU_ERR_APB_PARITY, 0x1);

    TEST_ASSERT_EQUAL(0x73, ((struct ssu_reg *)SSU_REG_PTR)->ERR_IMPDEF);
}

void test_ssu_error_status_api(void)
{
    static const struct mod_ssu_error_control_status_api *api;
    int status;
    uint32_t value;
    uint8_t err_fr;

    /* Get SSU module Error control APIs */
    fwk_id_t api_id = FWK_ID_API(FWK_MODULE_IDX_SSU, MOD_SSU_ERR_API_IDX);
    fwk_id_t element_id =
        FWK_ID_ELEMENT(FWK_MODULE_IDX_SSU, CONFIG_SSU_ELEMENT_IDX);
    fwk_id_is_type_ExpectAndReturn(element_id, FWK_ID_TYPE_ELEMENT, true);
    fwk_id_get_api_idx_ExpectAndReturn(api_id, MOD_SSU_ERR_API_IDX);
    status = ssu_process_bind_request(
        fwk_module_id_fake, element_id, api_id, (const void **)&api);
    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
    TEST_ASSERT_NOT_NULL(api);

    fwk_id_get_element_idx_ExpectAndReturn(element_id, 0);

    api->get_err_feature(element_id, &err_fr);
    TEST_ASSERT_EQUAL(err_fr, ((struct ssu_reg *)SSU_REG_PTR)->ERR_FR);

    ((struct ssu_reg *)SSU_REG_PTR)->ERR_STATUS = 0xDEADBEEF;

    /* Get MOD_SSU_ERR_SERR */
    api->get_err_status(element_id, MOD_SSU_ERR_SERR, &value);
    TEST_ASSERT_EQUAL(0xEF, value);

    fwk_id_get_element_idx_ExpectAndReturn(element_id, 0);

    /* Get MOD_SSU_ERR_IERR */
    api->get_err_status(element_id, MOD_SSU_ERR_IERR, &value);
    TEST_ASSERT_EQUAL(0x1E, value);

    fwk_id_get_element_idx_ExpectAndReturn(element_id, 0);

    /* Get MOD_SSU_ERR_APB_SW */
    api->get_err_status(element_id, MOD_SSU_ERR_APB_SW, &value);
    TEST_ASSERT_EQUAL(0x0, value);

    fwk_id_get_element_idx_ExpectAndReturn(element_id, 0);

    /* Get MOD_SSU_ERR_APB_INC_SEQ */
    api->get_err_status(element_id, MOD_SSU_ERR_APB_INC_SEQ, &value);
    TEST_ASSERT_EQUAL(0x1, value);

    fwk_id_get_element_idx_ExpectAndReturn(element_id, 0);

    /* Get MOD_SSU_ERR_APB_PARITY */
    api->get_err_status(element_id, MOD_SSU_ERR_APB_PARITY, &value);
    TEST_ASSERT_EQUAL(0x1, value);

    fwk_id_get_element_idx_ExpectAndReturn(element_id, 0);

    /* Get MOD_SSU_ERR_IN_FMU */
    api->get_err_status(element_id, MOD_SSU_ERR_IN_FMU, &value);
    TEST_ASSERT_EQUAL(0x1, value);

    fwk_id_get_element_idx_ExpectAndReturn(element_id, 0);

    /* Get MOD_SSU_ERR_OF */
    api->get_err_status(element_id, MOD_SSU_ERR_OF, &value);
    TEST_ASSERT_EQUAL(0x1, value);

    fwk_id_get_element_idx_ExpectAndReturn(element_id, 0);

    /* Get MOD_SSU_ERR_VALID */
    api->get_err_status(element_id, MOD_SSU_ERR_VALID, &value);
    TEST_ASSERT_EQUAL(0x1, value);
}

void test_ssu_start(void)
{
    int status;

    fwk_id_is_type_ExpectAndReturn(
        fwk_element_id_ssu, FWK_ID_TYPE_ELEMENT, true);

    fwk_notification_subscribe_ExpectAndReturn(
        mod_fmu_notification_id_fault,
        FWK_ID_MODULE(FWK_MODULE_IDX_FMU),
        fwk_element_id_ssu,
        FWK_SUCCESS);

    status = ssu_start(fwk_element_id_ssu);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

#ifdef BUILD_HAS_NOTIFICATION
void test_ssu_subscribe_notification_false(void)
{
    int status;
    struct fwk_event event = {
        .target_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_SSU, CONFIG_SSU_ELEMENT_IDX)
    };
    struct fwk_event resp_event = { 0 };

    fwk_id_is_equal_ExpectAndReturn(
        event.id, mod_fmu_notification_id_fault, false);

    status = ssu_process_notification(&event, &resp_event);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void test_ssu_subscribe_notification_true(void)
{
    int status;

    fwk_id_t element_id =
        FWK_ID_ELEMENT(FWK_MODULE_IDX_SSU, CONFIG_SSU_ELEMENT_IDX);
    struct fwk_event event = {
        .target_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_SSU, CONFIG_SSU_ELEMENT_IDX)
    };
    struct fwk_event resp_event = { 0 };

    struct mod_fmu_fault_notification_params *req_params =
        (struct mod_fmu_fault_notification_params *)&event.params;

    req_params->critical = true;

    fwk_id_is_equal_ExpectAndReturn(
        event.id, mod_fmu_notification_id_fault, true);

    fwk_id_get_element_idx_ExpectAndReturn(element_id, 0);

    status = ssu_process_notification(&event, &resp_event);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

#endif

int ssu_test_main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_ssu);
    RUN_TEST(test_ssu_sys_api);
    RUN_TEST(test_ssu_error_control_api);
    RUN_TEST(test_ssu_error_status_api);
    RUN_TEST(test_ssu_start);
#ifdef BUILD_HAS_NOTIFICATION
    RUN_TEST(test_ssu_subscribe_notification_false);
    RUN_TEST(test_ssu_subscribe_notification_true);
#endif

    return UNITY_END();
}

#if !defined(TEST_ON_TARGET)
int main(void)
{
    return ssu_test_main();
}
#endif
