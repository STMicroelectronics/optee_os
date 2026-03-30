/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "mod_ssu.h"
#include "si0_cfgd_ssu.h"

#include <unity.h>

#include <mod_integration_test.h>

#include <fwk_id.h>
#include <fwk_log.h>
#include <fwk_module.h>

static struct mod_ssu_sys_register_api *ssu_api;
static struct mod_ssu_error_control_status_api *error_ctl_api;

static fwk_id_t element_id =
    FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_SSU, CONFIG_SSU_ELEMENT_IDX);

static int test_ssu_sys_api(void)
{
    uint32_t state;

    fwk_assert(NULL != ssu_api);

    /* Change SSU state to MOD_SSU_FSM_SAFE_STATE */
    ssu_api->set_sys_ctrl(element_id, MOD_SSU_FSM_SAFE_STATE);

    ssu_api->get_sys_status(element_id, &state);
    TEST_ASSERT_EQUAL(state, MOD_SSU_SAFETY_STATUS_SAFE);

    /* Change SSU state to MOD_SSU_FSM_NCE_STATE */
    ssu_api->set_sys_ctrl(element_id, MOD_SSU_FSM_NCE_STATE);

    ssu_api->get_sys_status(element_id, &state);
    TEST_ASSERT_EQUAL(state, MOD_SSU_SAFETY_STATUS_ERRN);

    /* Change SSU state to MOD_SSU_FSM_SAFE_STATE */
    ssu_api->set_sys_ctrl(element_id, MOD_SSU_FSM_SAFE_STATE);

    ssu_api->get_sys_status(element_id, &state);
    TEST_ASSERT_EQUAL(state, MOD_SSU_SAFETY_STATUS_SAFE);

    /* Change SSU state to MOD_SSU_FSM_NCE_STATE */
    ssu_api->set_sys_ctrl(element_id, MOD_SSU_FSM_CE_STATE);

    ssu_api->get_sys_status(element_id, &state);
    TEST_ASSERT_EQUAL(state, MOD_SSU_SAFETY_STATUS_ERRC);

    /* Change SSU state to MOD_SSU_FSM_SAFE_STATE */
    ssu_api->set_sys_ctrl(element_id, MOD_SSU_FSM_SAFE_STATE);

    ssu_api->get_sys_status(element_id, &state);
    TEST_ASSERT_NOT_EQUAL(state, MOD_SSU_SAFETY_STATUS_SAFE);
    return FWK_SUCCESS;
}

static int test_error_control_api(void)
{
    uint32_t value;

    fwk_assert(NULL != error_ctl_api);

    /* Get MOD_SSU_ERR_IN_FMU */
    error_ctl_api->get_err_status(element_id, MOD_SSU_ERR_IN_FMU, &value);
    TEST_ASSERT_EQUAL(0x0, value);
    return FWK_SUCCESS;
}

enum test_case {
    TEST_CASE_SYS_API,
    TEST_CASE_ERR_CTRL,
    TEST_CASE_COUNT,
};

static const char *test_name(unsigned int case_idx)
{
    enum test_case current_test = case_idx;

    switch (current_test) {
    case TEST_CASE_SYS_API:
        return "test_ssu_sys_api";
    case TEST_CASE_ERR_CTRL:
        return "test_error_control_api";
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
    case TEST_CASE_SYS_API:
        return test_ssu_sys_api();
    case TEST_CASE_ERR_CTRL:
        return test_error_control_api();
    default:
        return FWK_E_PARAM;
    }
}

static const struct mod_integration_test_api test_api = {
    .run = run,
    .test_name = test_name,
};

static int test_ssu_init(
    fwk_id_t module_id,
    unsigned int unused,
    const void *data)
{
    return FWK_SUCCESS;
}

static int test_ssu_process_bind_request(
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

static int test_ssu_bind(fwk_id_t id, unsigned int round)
{
    int status;

    if (round > 0)
        return FWK_SUCCESS;

    status = fwk_module_bind(
        FWK_ID_MODULE(FWK_MODULE_IDX_SSU),
        FWK_ID_API(FWK_MODULE_IDX_SSU, MOD_SSU_SYS_API_IDX),
        &ssu_api);
    if (status != FWK_SUCCESS) {
        fwk_unexpected();
        return status;
    }

    status = fwk_module_bind(
        FWK_ID_MODULE(FWK_MODULE_IDX_SSU),
        FWK_ID_API(FWK_MODULE_IDX_SSU, MOD_SSU_ERR_API_IDX),
        &error_ctl_api);
    if (status != FWK_SUCCESS) {
        fwk_unexpected();
        return status;
    }

    return FWK_SUCCESS;
}

struct fwk_module_config config_test_ssu = { 0 };
const struct fwk_module module_test_ssu = {
    .type = FWK_MODULE_TYPE_SERVICE,
    .init = test_ssu_init,
    .process_bind_request = test_ssu_process_bind_request,
    .api_count = MOD_INTEGRATION_TEST_API_COUNT,
    .bind = test_ssu_bind,
};
