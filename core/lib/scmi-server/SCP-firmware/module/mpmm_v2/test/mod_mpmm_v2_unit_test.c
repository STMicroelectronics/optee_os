/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "scp_unity.h"
#include "unity.h"

#include <Mockfwk_core.h>
#include <Mockfwk_id.h>
#include <Mockfwk_mm.h>
#include <Mockfwk_module.h>
#include <Mockfwk_notification.h>
#include <Mockmod_mpmm_v2_extra.h>
#include <config_mpmm_v2.h>
#include <internal/Mockfwk_core_internal.h>

#include <mod_power_domain.h>

#include <fwk_element.h>
#include <fwk_macros.h>
#include <fwk_module_idx.h>
#include <fwk_notification.h>

#include UNIT_TEST_SRC

#define CLAMP_VAL(val, lo, hi) FWK_MIN((typeof(val))FWK_MAX(val, lo), hi)
static fwk_id_t perf_controller_notification_perf_set =
    FWK_ID_NOTIFICATION_INIT(
        FWK_MODULE_IDX_PERF_CONTROLLER,
        MOD_PERF_CONTROLLER_NOTIFICATION_IDX_PERF_SET);

uint64_t test_amu_counter[AMU_COUNT];
struct mod_mpmm_v2_domain_ctx domain_table[1];
struct amu_api test_amu_api;

void setUp(void)
{
    struct mod_mpmm_v2_domain_ctx *domain_ctx;

    mpmm_v2_ctx.mpmm_v2_domain_count = 1;
    mpmm_v2_ctx.domain_ctx = domain_ctx = &domain_table[0];
    mpmm_v2_ctx.amu_driver_api = &test_amu_api;
    mpmm_v2_ctx.amu_driver_api_id =
        FWK_ID_API(FWK_MODULE_IDX_AMU_MMAP, MOD_AMU_MMAP_API_IDX_AMU);

    domain_ctx->domain_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_MPMM_V2, 0);
    domain_ctx->num_cores = 1;
    domain_ctx->num_cores_online = 0;
    domain_ctx->wait_for_perf_transition = true;
    domain_ctx->domain_config = &fake_dom_conf[MPMM_V2_DOM_DEFAULT];
}

void tearDown(void)
{
    mpmm_v2_ctx.mpmm_v2_domain_count = 0;
    mpmm_v2_ctx.domain_ctx = NULL;
    mpmm_v2_ctx.amu_driver_api = NULL;
    mpmm_v2_ctx.amu_driver_api_id = FWK_ID_NONE;
}

void utest_mpmm_v2_start_mod_id_success(void)
{
    int status;

    fwk_module_is_valid_module_id_ExpectAndReturn(fwk_module_id_mpmm_v2, true);

    status = mpmm_v2_start(fwk_module_id_mpmm_v2);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void utest_mpmm_v2_start_notif_subscribe_success(void)
{
    int status;
    fwk_id_t elem_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_MPMM_V2, 0);

    fwk_module_is_valid_module_id_ExpectAndReturn(elem_id, false);
    fwk_id_get_element_idx_ExpectAndReturn(elem_id, 0);
    fwk_notification_subscribe_ExpectAndReturn(
        mod_pd_notification_id_power_state_pre_transition,
        mpmm_v2_ctx.domain_ctx->domain_config->core_config[0].pd_id,
        mpmm_v2_ctx.domain_ctx->domain_id,
        FWK_SUCCESS);
    fwk_notification_subscribe_ExpectAndReturn(
        mod_pd_notification_id_power_state_transition,
        mpmm_v2_ctx.domain_ctx->domain_config->core_config[0].pd_id,
        mpmm_v2_ctx.domain_ctx->domain_id,
        FWK_SUCCESS);

#ifdef BUILD_HAS_MOD_PERF_CONTROLLER
    fwk_notification_subscribe_ExpectAndReturn(
        perf_controller_notification_perf_set,
        mpmm_v2_ctx.domain_ctx->domain_config->perf_id,
        mpmm_v2_ctx.domain_ctx->domain_id,
        FWK_SUCCESS);
#endif
    status = mpmm_v2_start(elem_id);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void utest_mpmm_v2_start_notif_subscribe_pd_pre_transition_fail(void)
{
    int status;
    fwk_id_t elem_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_MPMM_V2, 0);

    fwk_module_is_valid_module_id_ExpectAndReturn(elem_id, false);
    fwk_id_get_element_idx_ExpectAndReturn(elem_id, 0);

    fwk_notification_subscribe_ExpectAndReturn(
        mod_pd_notification_id_power_state_pre_transition,
        mpmm_v2_ctx.domain_ctx->domain_config->core_config[0].pd_id,
        mpmm_v2_ctx.domain_ctx->domain_id,
        FWK_E_STATE);

    status = mpmm_v2_start(elem_id);
    TEST_ASSERT_EQUAL(FWK_E_STATE, status);
}

void utest_mpmm_v2_start_notif_subscribe_pd_post_transition_fail(void)
{
    int status;
    fwk_id_t elem_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_MPMM_V2, 0);

    fwk_module_is_valid_module_id_ExpectAndReturn(elem_id, false);
    fwk_id_get_element_idx_ExpectAndReturn(elem_id, 0);
    fwk_notification_subscribe_ExpectAndReturn(
        mod_pd_notification_id_power_state_pre_transition,
        mpmm_v2_ctx.domain_ctx->domain_config->core_config[0].pd_id,
        mpmm_v2_ctx.domain_ctx->domain_id,
        FWK_SUCCESS);

    fwk_notification_subscribe_ExpectAndReturn(
        mod_pd_notification_id_power_state_transition,
        mpmm_v2_ctx.domain_ctx->domain_config->core_config[0].pd_id,
        mpmm_v2_ctx.domain_ctx->domain_id,
        FWK_E_STATE);

    status = mpmm_v2_start(elem_id);
    TEST_ASSERT_EQUAL(FWK_E_STATE, status);
}

#ifdef BUILD_HAS_MOD_PERF_CONTROLLER
void utest_mpmm_v2_start_notif_subscribe_perf_controller_perf_set_fail(void)
{
    int status;
    fwk_id_t elem_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_MPMM_V2, 0);

    fwk_module_is_valid_module_id_ExpectAndReturn(elem_id, false);
    fwk_id_get_element_idx_ExpectAndReturn(elem_id, 0);
    fwk_notification_subscribe_ExpectAndReturn(
        mod_pd_notification_id_power_state_pre_transition,
        mpmm_v2_ctx.domain_ctx->domain_config->core_config[0].pd_id,
        mpmm_v2_ctx.domain_ctx->domain_id,
        FWK_SUCCESS);
    fwk_notification_subscribe_ExpectAndReturn(
        mod_pd_notification_id_power_state_transition,
        mpmm_v2_ctx.domain_ctx->domain_config->core_config[0].pd_id,
        mpmm_v2_ctx.domain_ctx->domain_id,
        FWK_SUCCESS);

    fwk_notification_subscribe_ExpectAndReturn(
        perf_controller_notification_perf_set,
        mpmm_v2_ctx.domain_ctx->domain_config->perf_id,
        mpmm_v2_ctx.domain_ctx->domain_id,
        FWK_E_STATE);

    status = mpmm_v2_start(elem_id);
    TEST_ASSERT_EQUAL(FWK_E_STATE, status);
}
#endif

void utest_mpmm_v2_init_success(void)
{
    int status;
    unsigned int count = 1;
    struct mod_mpmm_v2_domain_ctx domain_ctx;
    fwk_id_t api_id =
        FWK_ID_API_INIT(FWK_MODULE_IDX_AMU_MMAP, MOD_AMU_MMAP_API_IDX_AMU);

    fwk_mm_calloc_ExpectAndReturn(
        count, sizeof(struct mod_mpmm_v2_domain_ctx), &domain_ctx);

    status = mpmm_v2_init(fwk_module_id_mpmm_v2, count, &api_id);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL(count, mpmm_v2_ctx.mpmm_v2_domain_count);
    TEST_ASSERT_EQUAL_PTR(&domain_ctx, mpmm_v2_ctx.domain_ctx);
    TEST_ASSERT_EQUAL(api_id.value, mpmm_v2_ctx.amu_driver_api_id.value);
}

void utest_mpmm_v2_init_element_count_fail(void)
{
    int status;

    status = mpmm_v2_init(fwk_module_id_mpmm_v2, 0, NULL);
    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
}

/* Element init. */
void utest_mpmm_v2_element_init_two_core_success(void)
{
    int status;
    uint64_t cached_counters[2];
    uint64_t delta[2];
    unsigned int count = 2;
    fwk_id_t elem_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_MPMM_V2, 0);

    fwk_id_get_element_idx_ExpectAndReturn(elem_id, 0);
    fwk_id_build_sub_element_id_IgnoreAndReturn(elem_id);
    fwk_mm_calloc_ExpectAndReturn(
        domain_table[0].domain_config->num_of_gears,
        sizeof(uint64_t),
        &cached_counters[CORE0_IDX]);
    fwk_mm_calloc_ExpectAndReturn(
        domain_table[0].domain_config->num_of_gears,
        sizeof(uint64_t),
        &delta[CORE0_IDX]);
    fwk_mm_calloc_ExpectAndReturn(
        domain_table[0].domain_config->num_of_gears,
        sizeof(uint64_t),
        &cached_counters[CORE1_IDX]);
    fwk_mm_calloc_ExpectAndReturn(
        domain_table[0].domain_config->num_of_gears,
        sizeof(uint64_t),
        &delta[CORE1_IDX]);

    status =
        mpmm_v2_element_init(elem_id, 2, &fake_dom_conf[MPMM_V2_DOM_DEFAULT]);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL(count, domain_table[0].num_cores);
    TEST_ASSERT_EQUAL(false, domain_table[0].wait_for_perf_transition);
    TEST_ASSERT_EQUAL_PTR(
        &fake_dom_conf[MPMM_V2_DOM_DEFAULT], domain_table[0].domain_config);

    /* CORE0_IDX */
    TEST_ASSERT_EQUAL(true, domain_table[0].core_ctx[CORE0_IDX].initialized);
    TEST_ASSERT_EQUAL_PTR(
        fake_core_config[0].mpmm_reg_base,
        domain_table[0].core_ctx[CORE0_IDX].mpmm_v2);
    TEST_ASSERT_EQUAL(
        fake_core_config[CORE0_IDX].base_aux_counter_id.value,
        domain_table[0].core_ctx[CORE0_IDX].base_aux_counter_id.value);
    TEST_ASSERT_EQUAL_PTR(
        &cached_counters[CORE0_IDX],
        domain_table[0].core_ctx[CORE0_IDX].cached_counters);
    TEST_ASSERT_EQUAL_PTR(
        &delta[CORE0_IDX], domain_table[0].core_ctx[CORE0_IDX].delta);

    /* CORE1_IDX */
    TEST_ASSERT_EQUAL(true, domain_table[0].core_ctx[CORE1_IDX].initialized);
    TEST_ASSERT_EQUAL_PTR(
        fake_core_config[CORE1_IDX].mpmm_reg_base,
        domain_table[0].core_ctx[CORE1_IDX].mpmm_v2);
    TEST_ASSERT_EQUAL(
        fake_core_config[CORE1_IDX].base_aux_counter_id.value,
        domain_table[0].core_ctx[CORE1_IDX].base_aux_counter_id.value);
    TEST_ASSERT_EQUAL_PTR(
        &cached_counters[CORE1_IDX],
        domain_table[0].core_ctx[CORE1_IDX].cached_counters);
    TEST_ASSERT_EQUAL_PTR(
        &delta[CORE1_IDX], domain_table[0].core_ctx[CORE1_IDX].delta);
}

void utest_mpmm_v2_element_init_element_count_0_fail(void)
{
    int status;
    fwk_id_t elem_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_MPMM_V2, 0);

    status = mpmm_v2_element_init(elem_id, 0, NULL);
    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
}

void utest_mpmm_v2_element_init_element_count_max_fail(void)
{
    int status;
    fwk_id_t elem_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_MPMM_V2, 0);

    status =
        mpmm_v2_element_init(elem_id, (MPMM_MAX_NUM_CORES_IN_DOMAIN + 1), NULL);
    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
}

void utest_mpmm_v2_element_init_max_gear_count_fail(void)
{
    int status;
    fwk_id_t elem_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_MPMM_V2, 0);

    fwk_id_get_element_idx_ExpectAndReturn(elem_id, 0);

    status = mpmm_v2_element_init(
        elem_id, 1, &fake_dom_conf[MPMM_V2_DOM_MAX_GEAR_COUNT]);
    TEST_ASSERT_EQUAL(FWK_E_SUPPORT, status);
}

void utest_mpmm_v2_element_init_num_gear_mismatch(void)
{
    int status;
    fwk_id_t elem_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_MPMM_V2, 0);

    fwk_id_get_element_idx_ExpectAndReturn(elem_id, 0);
    fwk_id_build_sub_element_id_IgnoreAndReturn(elem_id);

    status = mpmm_v2_element_init(
        elem_id, 1, &fake_dom_conf[MPMM_V2_DOM_NUM_GEAR_NOT_EQUAL]);
    TEST_ASSERT_EQUAL(FWK_E_DEVICE, status);
}

void utest_mpmm_v2_bind_first_round_success(void)
{
    int status;
    unsigned int round = 0;

    status = mpmm_v2_bind(fwk_module_id_mpmm_v2, round);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void utest_mpmm_v2_bind_invalid_module_success(void)
{
    int status;
    unsigned int round = 1;

    fwk_module_is_valid_module_id_ExpectAndReturn(fwk_module_id_mpmm_v2, false);

    status = mpmm_v2_bind(fwk_module_id_mpmm_v2, round);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void utest_mpmm_v2_bind_module_bind_success(void)
{
    int status;
    unsigned int round = 1;

    fwk_module_is_valid_module_id_ExpectAndReturn(fwk_module_id_mpmm_v2, true);
    fwk_module_bind_ExpectAndReturn(
        FWK_ID_MODULE(mpmm_v2_ctx.amu_driver_api_id.common.module_idx),
        mpmm_v2_ctx.amu_driver_api_id,
        &mpmm_v2_ctx.amu_driver_api,
        FWK_SUCCESS);

    status = mpmm_v2_bind(fwk_module_id_mpmm_v2, round);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void utest_mpmm_v2_bind_amu_bind_fail(void)
{
    int status;
    unsigned int round = 1;

    fwk_module_is_valid_module_id_ExpectAndReturn(fwk_module_id_mpmm_v2, true);
    fwk_module_bind_ExpectAndReturn(
        FWK_ID_MODULE(mpmm_v2_ctx.amu_driver_api_id.common.module_idx),
        mpmm_v2_ctx.amu_driver_api_id,
        &mpmm_v2_ctx.amu_driver_api,
        FWK_E_PARAM);

    status = mpmm_v2_bind(fwk_module_id_mpmm_v2, round);
    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
}

void utest_mpmm_v2_process_bind_request_success(void)
{
    int status;
    const struct interface_power_management_api *api = NULL;
    const fwk_id_t expected_api_id =
        FWK_ID_API_INIT(FWK_MODULE_IDX_MPMM_V2, MOD_MPMM_V2_GET_LIMIT_API_IDX);

    fwk_id_is_equal_ExpectAndReturn(
        expected_api_id,
        FWK_ID_API(FWK_MODULE_IDX_MPMM_V2, MOD_MPMM_V2_GET_LIMIT_API_IDX),
        true);

    status = mpmm_v2_process_bind_request(
        fwk_module_id_metrics_analyzer,
        fwk_module_id_mpmm_v2,
        FWK_ID_API(FWK_MODULE_IDX_MPMM_V2, MOD_MPMM_V2_GET_LIMIT_API_IDX),
        (const void **)&api);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL(&mpmm_v2_pm_api, api);
}

void utest_mpmm_v2_process_bind_request_id_not_equal(void)
{
    int status;

    fwk_id_is_equal_ExpectAndReturn(
        fwk_module_id_mpmm_v2,
        FWK_ID_API(FWK_MODULE_IDX_MPMM_V2, MOD_MPMM_V2_GET_LIMIT_API_IDX),
        false);

    status = mpmm_v2_process_bind_request(
        fwk_module_id_mpmm_v2,
        fwk_module_id_mpmm_v2,
        fwk_module_id_mpmm_v2,
        NULL);
    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
}

void utest_mpmm_v2_process_notification_pre_state_to_on(void)
{
    int status;
    struct mod_pd_power_state_pre_transition_notification_params *params;
    struct fwk_event event = { .target_id =
                                   FWK_ID_EVENT(FWK_MODULE_IDX_MPMM_V2, 0) };
    struct fwk_event resp_event = { 0 };
    const struct mod_mpmm_v2_domain_config *config =
        mpmm_v2_ctx.domain_ctx->domain_config;

    domain_table[0].num_cores_online = 0;
    domain_table[0].core_ctx[0].online = true;
    /* This will trigger a dvfs change after turning it ON. */
    domain_table[0].current_power_level = 2 * config->max_power;
    domain_table[0].core_ctx[0].pd_blocked = false;

    /* PD notification OFF --> ON */
    params = (struct mod_pd_power_state_pre_transition_notification_params *)
                 event.params;
    params->current_state = MOD_PD_STATE_OFF;
    params->target_state = MOD_PD_STATE_ON;

    fwk_module_is_valid_element_id_ExpectAndReturn(event.target_id, true);
    fwk_id_get_element_idx_ExpectAndReturn(event.target_id, 0);
    fwk_id_get_module_idx_ExpectAndReturn(
        event.id, FWK_MODULE_IDX_POWER_DOMAIN);
    fwk_id_is_equal_ExpectAndReturn(
        config->core_config[0].pd_id, event.source_id, true);
    fwk_id_is_equal_ExpectAndReturn(
        event.id, mod_pd_notification_id_power_state_pre_transition, true);

    status = mpmm_v2_process_notification(&event, &resp_event);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL(1, mpmm_v2_ctx.domain_ctx->num_cores_online);
    TEST_ASSERT_TRUE(mpmm_v2_ctx.domain_ctx->core_ctx[0].online);
    TEST_ASSERT_EQUAL(0, mpmm_v2_ctx.domain_ctx->core_ctx[0].selected_gear);
    TEST_ASSERT_EQUAL(config->max_power, mpmm_v2_ctx.domain_ctx->power_limit);
    TEST_ASSERT_TRUE(mpmm_v2_ctx.domain_ctx->core_ctx[0].pd_blocked);
    TEST_ASSERT_TRUE(mpmm_v2_ctx.domain_ctx->wait_for_perf_transition);
    TEST_ASSERT_TRUE(resp_event.is_delayed_response);
    TEST_ASSERT_EQUAL(event.cookie, mpmm_v2_ctx.domain_ctx->core_ctx[0].cookie);
}

void utest_mpmm_v2_process_notification_post_state_to_off(void)
{
    int status;
    struct mod_pd_power_state_transition_notification_params *params;
    struct fwk_event event = { .target_id =
                                   FWK_ID_EVENT(FWK_MODULE_IDX_MPMM_V2, 0) };
    struct fwk_event resp_event = { 0 };

    mpmm_v2_ctx.domain_ctx->num_cores_online = 1;
    params = (struct mod_pd_power_state_transition_notification_params *)
                 event.params;
    params->state = MOD_PD_STATE_OFF;

    fwk_module_is_valid_element_id_ExpectAndReturn(event.target_id, true);
    fwk_id_get_element_idx_ExpectAndReturn(event.target_id, 0);
    fwk_id_get_module_idx_ExpectAndReturn(
        event.id, FWK_MODULE_IDX_POWER_DOMAIN);
    fwk_id_is_equal_ExpectAndReturn(
        mpmm_v2_ctx.domain_ctx->domain_config->core_config[0].pd_id,
        event.source_id,
        true);
    fwk_id_is_equal_ExpectAndReturn(
        event.id, mod_pd_notification_id_power_state_pre_transition, false);
    fwk_id_is_equal_ExpectAndReturn(
        event.id, mod_pd_notification_id_power_state_transition, true);

    status = mpmm_v2_process_notification(&event, &resp_event);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL(0, mpmm_v2_ctx.domain_ctx->num_cores_online);
    TEST_ASSERT_EQUAL(false, mpmm_v2_ctx.domain_ctx->core_ctx[0].online);
}

void utest_mpmm_v2_process_notification_post_state_to_on_core_init_fail(void)
{
    int status;
    struct mod_mpmm_v2_domain_ctx *domain_ctx;
    struct mod_pd_power_state_transition_notification_params *params;
    struct fwk_event event = { .target_id = FWK_ID_EVENT(
                                   FWK_MODULE_IDX_MPMM_V2,
                                   MPMM_V2_DOM_NUM_GEAR_NOT_EQUAL) };
    struct fwk_event resp_event = { 0 };

    mpmm_v2_ctx.mpmm_v2_domain_count = MPMM_V2_DOM_NUM_GEAR_NOT_EQUAL + 1;
    domain_ctx = &domain_table[MPMM_V2_DOM_NUM_GEAR_NOT_EQUAL];
    domain_ctx->domain_id =
        FWK_ID_ELEMENT(FWK_MODULE_IDX_MPMM_V2, MPMM_V2_DOM_NUM_GEAR_NOT_EQUAL);
    domain_ctx->domain_config = &fake_dom_conf[MPMM_V2_DOM_NUM_GEAR_NOT_EQUAL];
    domain_ctx->num_cores = 1;
    domain_ctx->num_cores_online = 1;
    domain_ctx->core_ctx[0].mpmm_v2 = &fake_mpmm_reg[0];
    params = (struct mod_pd_power_state_transition_notification_params *)
                 event.params;
    params->state = MOD_PD_STATE_ON;

    fwk_module_is_valid_element_id_ExpectAndReturn(event.target_id, true);
    fwk_id_get_element_idx_ExpectAndReturn(
        event.target_id, MPMM_V2_DOM_NUM_GEAR_NOT_EQUAL);
    fwk_id_get_module_idx_ExpectAndReturn(
        event.id, FWK_MODULE_IDX_POWER_DOMAIN);
    fwk_id_is_equal_ExpectAndReturn(
        mpmm_v2_ctx.domain_ctx->domain_config->core_config[0].pd_id,
        event.source_id,
        true);
    fwk_id_is_equal_ExpectAndReturn(
        event.id, mod_pd_notification_id_power_state_pre_transition, false);
    fwk_id_is_equal_ExpectAndReturn(
        event.id, mod_pd_notification_id_power_state_transition, true);

    status = mpmm_v2_process_notification(&event, &resp_event);
    TEST_ASSERT_EQUAL(FWK_E_DEVICE, status);
}

void utest_mpmm_v2_process_notification_invalid_domain_id(void)
{
    int status;
    struct fwk_event event = { .target_id =
                                   FWK_ID_EVENT(FWK_MODULE_IDX_MPMM_V2, 0) };
    struct fwk_event resp_event = { 0 };

    fwk_module_is_valid_element_id_ExpectAndReturn(event.target_id, false);
    fwk_id_get_element_idx_ExpectAndReturn(event.target_id, 0);
    fwk_id_get_module_idx_ExpectAndReturn(
        event.id, FWK_MODULE_IDX_POWER_DOMAIN);
    fwk_id_is_equal_ExpectAndReturn(
        mpmm_v2_ctx.domain_ctx->domain_config->core_config[0].pd_id,
        event.source_id,
        false);

    status = mpmm_v2_process_notification(&event, &resp_event);
    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
}

void utest_mpmm_v2_process_notification_core_idx_larger(void)
{
    int status;
    struct fwk_event event = { .target_id =
                                   FWK_ID_EVENT(FWK_MODULE_IDX_MPMM_V2, 0) };
    struct fwk_event resp_event = { 0 };

    fwk_module_is_valid_element_id_ExpectAndReturn(event.target_id, true);
    fwk_id_get_element_idx_ExpectAndReturn(event.target_id, 0);
    fwk_id_get_module_idx_ExpectAndReturn(
        event.id, FWK_MODULE_IDX_POWER_DOMAIN);
    fwk_id_is_equal_ExpectAndReturn(
        mpmm_v2_ctx.domain_ctx->domain_config->core_config[0].pd_id,
        event.source_id,
        false);

    status = mpmm_v2_process_notification(&event, &resp_event);
    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
}

void utest_mpmm_v2_process_notification_no_perf_change(void)
{
    int status;
    struct mod_pd_power_state_pre_transition_notification_params *params;
    struct fwk_event event = { .target_id =
                                   FWK_ID_EVENT(FWK_MODULE_IDX_MPMM_V2, 0) };
    struct fwk_event resp_event = { 0 };
    const struct mod_mpmm_v2_domain_config *config =
        mpmm_v2_ctx.domain_ctx->domain_config;

    domain_table[0].num_cores_online = 0;
    domain_table[0].core_ctx[0].online = false;
    domain_table[0].current_power_level = config->max_power;
    domain_table[0].core_ctx[0].pd_blocked = false;

    params = (struct mod_pd_power_state_pre_transition_notification_params *)
                 event.params;
    params->target_state = MOD_PD_STATE_ON;

    fwk_module_is_valid_element_id_ExpectAndReturn(event.target_id, true);
    fwk_id_get_element_idx_ExpectAndReturn(event.target_id, 0);
    fwk_id_get_module_idx_ExpectAndReturn(
        event.id, FWK_MODULE_IDX_POWER_DOMAIN);
    fwk_id_is_equal_ExpectAndReturn(
        mpmm_v2_ctx.domain_ctx->domain_config->core_config[0].pd_id,
        event.source_id,
        true);
    fwk_id_is_equal_ExpectAndReturn(
        event.id, mod_pd_notification_id_power_state_pre_transition, true);

    status = mpmm_v2_process_notification(&event, &resp_event);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL(1, mpmm_v2_ctx.domain_ctx->num_cores_online);
    TEST_ASSERT_TRUE(mpmm_v2_ctx.domain_ctx->core_ctx[0].online);
    TEST_ASSERT_EQUAL(0, mpmm_v2_ctx.domain_ctx->core_ctx[0].selected_gear);
    TEST_ASSERT_EQUAL(config->max_power, mpmm_v2_ctx.domain_ctx->power_limit);
    TEST_ASSERT_FALSE(mpmm_v2_ctx.domain_ctx->core_ctx[0].pd_blocked);
}

void utest_mpmm_v2_evaluate_power_limit_most_aggressive_gear(void)
{
    int expected_power_limit;
    int eval_power_limit;
    struct mod_mpmm_v2_domain_ctx *domain_ctx;

    domain_ctx = &domain_table[0];
    domain_ctx->domain_id =
        FWK_ID_ELEMENT(FWK_MODULE_IDX_MPMM_V2, MPMM_V2_DOM_TWO_GEAR_COUNTER);
    domain_ctx->num_cores = 1;
    domain_ctx->num_cores_online = 1;
    domain_ctx->power_limit = __LINE__;
    domain_ctx->domain_config = &fake_dom_conf[MPMM_V2_DOM_TWO_GEAR_COUNTER];
    domain_ctx->core_ctx[0].selected_gear = 0;
    expected_power_limit = domain_ctx->domain_config->max_power;

    eval_power_limit = mpmm_v2_evaluate_power_limit(domain_ctx);
    TEST_ASSERT_EQUAL(expected_power_limit, eval_power_limit);
}

void utest_mpmm_v2_evaluate_power_limit_least_aggressive_gear(void)
{
    uint32_t expected_power_limit;
    int eval_power_limit;
    struct mod_mpmm_v2_domain_ctx *domain_ctx;
    uint32_t *gear_weights =
        fake_dom_conf[MPMM_V2_DOM_TWO_GEAR_COUNTER].gear_weights;
    uint32_t selected_gear = 1;
    int32_t gear_weight_delta;

    domain_ctx = &domain_table[0];
    domain_ctx->domain_id =
        FWK_ID_ELEMENT(FWK_MODULE_IDX_MPMM_V2, MPMM_V2_DOM_TWO_GEAR_COUNTER);
    domain_ctx->domain_config = &fake_dom_conf[MPMM_V2_DOM_TWO_GEAR_COUNTER];
    domain_ctx->num_cores = 1;
    domain_ctx->num_cores_online = 1;
    domain_ctx->power_limit = __LINE__;
    domain_ctx->core_ctx[0].selected_gear = selected_gear;
    /* Calculating the expected power limit using power model. */
    gear_weight_delta =
        (gear_weights[selected_gear] - gear_weights[MPMM_DEFAULT_GEAR]) + 100;
    gear_weight_delta = FWK_MAX(0, gear_weight_delta);
    expected_power_limit =
        (1.0 * domain_ctx->domain_config->max_power * gear_weight_delta) / 100;
    expected_power_limit = CLAMP_VAL(
        expected_power_limit,
        domain_ctx->domain_config->min_power,
        domain_ctx->domain_config->max_power);

    eval_power_limit = mpmm_v2_evaluate_power_limit(domain_ctx);
    TEST_ASSERT_EQUAL(expected_power_limit, eval_power_limit);
}

void utest_mpmm_v2_evaluate_power_limit_2_cores_online_most_aggressive_gear(
    void)
{
    int expected_power_limit;
    int eval_power_limit;
    struct mod_mpmm_v2_domain_ctx *domain_ctx;
    uint32_t *gear_weights =
        fake_dom_conf[MPMM_V2_DOM_TWO_GEAR_COUNTER].gear_weights;
    uint32_t selected_gear = 0;
    int32_t gear_weight_delta;

    domain_ctx = &domain_table[0];
    domain_ctx->domain_id =
        FWK_ID_ELEMENT(FWK_MODULE_IDX_MPMM_V2, MPMM_V2_DOM_TWO_GEAR_COUNTER);
    domain_ctx->domain_config = &fake_dom_conf[MPMM_V2_DOM_TWO_GEAR_COUNTER];
    domain_ctx->num_cores = 2;
    domain_ctx->num_cores_online = 2;
    domain_ctx->power_limit = __LINE__;
    domain_ctx->core_ctx[0].selected_gear = selected_gear;
    domain_ctx->core_ctx[1].selected_gear = selected_gear;
    /* Calculating the expected power limit using power model. */
    gear_weight_delta =
        2 * (gear_weights[selected_gear] - gear_weights[MPMM_DEFAULT_GEAR]);
    gear_weight_delta = FWK_MAX(0, gear_weight_delta + 100);
    expected_power_limit = domain_ctx->domain_config->max_power;

    eval_power_limit = mpmm_v2_evaluate_power_limit(domain_ctx);
    TEST_ASSERT_EQUAL(expected_power_limit, eval_power_limit);
}

void utest_mpmm_v2_evaluate_power_limit_2_cores_online_mix_of_gears(void)
{
    uint32_t expected_power_limit;
    int eval_power_limit;
    struct mod_mpmm_v2_domain_ctx *domain_ctx;
    uint32_t *gear_weights =
        fake_dom_conf[MPMM_V2_DOM_TWO_GEAR_COUNTER].gear_weights;
    uint32_t core_0_selected_gear = 0;
    uint32_t core_1_selected_gear = 1;
    int32_t gear_weight_delta;

    domain_ctx = &domain_table[0];
    domain_ctx->domain_id =
        FWK_ID_ELEMENT(FWK_MODULE_IDX_MPMM_V2, MPMM_V2_DOM_TWO_GEAR_COUNTER);
    domain_ctx->domain_config = &fake_dom_conf[MPMM_V2_DOM_TWO_GEAR_COUNTER];
    domain_ctx->num_cores = 2;
    domain_ctx->num_cores_online = 2;
    domain_ctx->power_limit = __LINE__;
    domain_ctx->core_ctx[0].selected_gear = core_0_selected_gear;
    domain_ctx->core_ctx[1].selected_gear = core_1_selected_gear;
    /* Calculating the expected power limit using power model. */
    gear_weight_delta =
        (gear_weights[core_0_selected_gear] - gear_weights[MPMM_DEFAULT_GEAR]);
    gear_weight_delta =
        (gear_weights[core_1_selected_gear] - gear_weights[MPMM_DEFAULT_GEAR]);
    gear_weight_delta = FWK_MAX(0, gear_weight_delta + 100);
    expected_power_limit =
        (1.0 * domain_ctx->domain_config->max_power * gear_weight_delta) / 100;
    expected_power_limit = CLAMP_VAL(
        expected_power_limit,
        domain_ctx->domain_config->min_power,
        domain_ctx->domain_config->max_power);

    eval_power_limit = mpmm_v2_evaluate_power_limit(domain_ctx);
    TEST_ASSERT_EQUAL(expected_power_limit, eval_power_limit);
}

void utest_mpmm_v2_evaluate_power_limit_2_cores_online_least_aggressive_gear(
    void)
{
    int expected_power_limit;
    int eval_power_limit;
    struct mod_mpmm_v2_domain_ctx *domain_ctx;
    uint32_t *gear_weights =
        fake_dom_conf[MPMM_V2_DOM_TWO_GEAR_COUNTER].gear_weights;
    uint32_t selected_gear = 1;
    int32_t gear_weight_delta;

    domain_ctx = &domain_table[0];
    domain_ctx->domain_id =
        FWK_ID_ELEMENT(FWK_MODULE_IDX_MPMM_V2, MPMM_V2_DOM_TWO_GEAR_COUNTER);
    domain_ctx->domain_config = &fake_dom_conf[MPMM_V2_DOM_TWO_GEAR_COUNTER];
    domain_ctx->num_cores = 2;
    domain_ctx->num_cores_online = 2;
    domain_ctx->power_limit = __LINE__;
    domain_ctx->core_ctx[0].selected_gear = selected_gear;
    domain_ctx->core_ctx[1].selected_gear = selected_gear;
    /* Calculating the expected power limit using power model. */
    gear_weight_delta =
        2 * (gear_weights[selected_gear] - gear_weights[MPMM_DEFAULT_GEAR]);
    gear_weight_delta = FWK_MAX(0, gear_weight_delta + 100);
    expected_power_limit = domain_ctx->domain_config->min_power;

    eval_power_limit = mpmm_v2_evaluate_power_limit(domain_ctx);
    TEST_ASSERT_EQUAL(expected_power_limit, eval_power_limit);
}

void utest_mpmm_v2_get_power_limit_success(void)
{
    int status;
    struct mod_mpmm_v2_domain_ctx *domain_ctx = mpmm_v2_ctx.domain_ctx;
    fwk_id_t perf_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_PERF_CONTROLLER, 0);
    uint32_t power_limit = __LINE__;

    domain_ctx->num_cores_online = 1;

    fwk_id_get_element_idx_ExpectAndReturn(domain_ctx->domain_id, 0);
    fwk_id_get_element_idx_ExpectAndReturn(domain_ctx->domain_id, 0);
    fwk_id_is_equal_ExpectAndReturn(
        domain_ctx->domain_config->perf_id, perf_id, true);

    status = mpmm_v2_get_power_limit(domain_ctx->domain_id, &power_limit);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL(domain_ctx->power_limit, power_limit);
}

void utest_mpmm_v2_get_power_limit_domain_idx_not_found(void)
{
    int status;
    uint32_t power_limit = UINT32_MAX;
    fwk_id_t perf_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_PERF_CONTROLLER, 0);
    fwk_id_t mpmm_v2_domain_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_MPMM_V2, 0);

    fwk_id_get_element_idx_ExpectAndReturn(mpmm_v2_domain_id, 0);
    fwk_id_is_equal_ExpectAndReturn(
        mpmm_v2_ctx.domain_ctx->domain_config->perf_id, perf_id, false);

    status = mpmm_v2_get_power_limit(mpmm_v2_domain_id, &power_limit);
    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
    TEST_ASSERT_EQUAL(UINT32_MAX, power_limit);
}

void utest_mpmm_v2_get_power_limit_no_cores_online(void)
{
    int status;
    struct mod_mpmm_v2_domain_ctx *domain_ctx = mpmm_v2_ctx.domain_ctx;
    uint32_t power_limit;
    fwk_id_t perf_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_PERF_CONTROLLER, 0);
    fwk_id_t mpmm_v2_domain_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_MPMM_V2, 0);
    domain_ctx->num_cores_online = 0;

    fwk_id_get_element_idx_ExpectAndReturn(mpmm_v2_domain_id, 0);
    fwk_id_get_element_idx_ExpectAndReturn(mpmm_v2_domain_id, 0);
    fwk_id_is_equal_ExpectAndReturn(
        mpmm_v2_ctx.domain_ctx->domain_config->perf_id, perf_id, true);

    status = mpmm_v2_get_power_limit(mpmm_v2_domain_id, &power_limit);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL(domain_ctx->domain_config->max_power, power_limit);
}

void utest_mpmm_v2_get_power_limit_wait_for_perf_trasition(void)
{
    int status;
    struct mod_mpmm_v2_domain_ctx *domain_ctx = mpmm_v2_ctx.domain_ctx;
    domain_ctx->domain_config = &fake_dom_conf[MPMM_V2_DOM_MPMM_ENABLED];
    const struct mod_mpmm_v2_domain_config *config = domain_ctx->domain_config;
    fwk_id_t mpmm_v2_domain_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_MPMM_V2, 0);
    fwk_id_t perf_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_PERF_CONTROLLER, 0);
    uint32_t power_limit;
    uint64_t cached_counters;

    domain_ctx->core_ctx[0].cached_counters = &cached_counters;
    domain_ctx->domain_id = mpmm_v2_domain_id;
    domain_ctx->num_cores = 1;
    domain_ctx->num_cores_online = 1;
    domain_ctx->core_ctx[0].online = true;
    /* This will require a dvfs change, if we switch to most aggressive gear. */
    domain_ctx->current_power_level = config->min_power;
    /* Configure counters expectation to cross current gear. */
    domain_ctx->core_ctx[0].mpmm_v2 =
        ((struct mpmm_reg *)fake_core_config_mpmm_enabled[0].mpmm_reg_base);
    cached_counters = 1;
    test_amu_counter[AMU_AUX0] = 1 + config->base_throtl_count;
    fwk_id_get_element_idx_ExpectAndReturn(mpmm_v2_domain_id, 0);
    fwk_id_get_element_idx_ExpectAndReturn(mpmm_v2_domain_id, 0);
    fwk_id_is_equal_ExpectAndReturn(
        domain_ctx->domain_config->perf_id, perf_id, true);
    /* Here, expectation for get_counter can be stated */
    test_amu_api.get_counters = amu_mmap_get_counters;
    amu_mmap_get_counters_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    amu_mmap_get_counters_ReturnMemThruPtr_counter_buff(
        &(test_amu_counter[AMU_AUX0]), sizeof(uint64_t) * config->num_of_gears);

    status = mpmm_v2_get_power_limit(mpmm_v2_domain_id, &power_limit);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL(power_limit, config->max_power);
    TEST_ASSERT_TRUE(mpmm_v2_ctx.domain_ctx->wait_for_perf_transition);
}

void utest_mpmm_v2_controller_noti_not_waiting_perf_transition_success(void)
{
    int status;
    struct mod_perf_controller_notification_params *params;
    struct fwk_event event = { .target_id =
                                   FWK_ID_EVENT(FWK_MODULE_IDX_MPMM_V2, 0) };
    struct fwk_event resp_event = { 0 };
    fwk_id_t perf_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_PERF_CONTROLLER, 0);
    struct mod_mpmm_v2_domain_ctx *domain_ctx = mpmm_v2_ctx.domain_ctx;

    domain_ctx->num_cores_online = 1;
    domain_ctx->wait_for_perf_transition = false;
    domain_ctx->core_ctx[0].pd_blocked = false;
    domain_ctx->current_power_level = __LINE__;

    fwk_module_is_valid_element_id_ExpectAndReturn(event.target_id, true);
    fwk_id_get_element_idx_ExpectAndReturn(event.target_id, 0);
    fwk_id_get_module_idx_ExpectAndReturn(
        event.id, FWK_MODULE_IDX_PERF_CONTROLLER);
    fwk_id_get_element_idx_ExpectAndReturn(event.target_id, 0);
    fwk_id_is_equal_ExpectAndReturn(
        mpmm_v2_ctx.domain_ctx->domain_config->perf_id, perf_id, true);

    params = (struct mod_perf_controller_notification_params *)event.params;
    params->power_limit = domain_ctx->domain_config->min_power + 100;

    status = mpmm_v2_process_notification(&event, &resp_event);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL(1, mpmm_v2_ctx.domain_ctx->num_cores_online);
    TEST_ASSERT_TRUE(mpmm_v2_ctx.domain_ctx->core_ctx[0].online);
    TEST_ASSERT_EQUAL(0, mpmm_v2_ctx.domain_ctx->core_ctx[0].selected_gear);
    TEST_ASSERT_EQUAL(
        params->power_limit, mpmm_v2_ctx.domain_ctx->current_power_level);
    TEST_ASSERT_FALSE(mpmm_v2_ctx.domain_ctx->core_ctx[0].pd_blocked);
    TEST_ASSERT_FALSE(mpmm_v2_ctx.domain_ctx->wait_for_perf_transition);
    /*TEST_ASSERT_TRUE(resp_event.is_delayed_response);*/
    TEST_ASSERT_EQUAL(event.cookie, mpmm_v2_ctx.domain_ctx->core_ctx[0].cookie);
}

void utest_mpmm_v2_controller_noti_waiting_perf_transition_pd_unblocked_success(
    void)
{
    int status;
    struct mod_perf_controller_notification_params *params;
    struct fwk_event event = { .target_id =
                                   FWK_ID_EVENT(FWK_MODULE_IDX_MPMM_V2, 0) };
    struct fwk_event resp_event = { 0 };
    fwk_id_t perf_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_PERF_CONTROLLER, 0);
    struct mod_mpmm_v2_domain_ctx *domain_ctx = mpmm_v2_ctx.domain_ctx;

    domain_ctx->num_cores_online = 1;
    domain_ctx->wait_for_perf_transition = true;
    domain_ctx->core_ctx[0].pd_blocked = false;
    domain_ctx->core_ctx[0].selected_gear = 1;
    domain_ctx->current_power_level = __LINE__;

    fwk_module_is_valid_element_id_ExpectAndReturn(event.target_id, true);
    fwk_id_get_element_idx_ExpectAndReturn(event.target_id, 0);
    fwk_id_get_module_idx_ExpectAndReturn(
        event.id, FWK_MODULE_IDX_PERF_CONTROLLER);
    fwk_id_get_element_idx_ExpectAndReturn(event.target_id, 0);
    fwk_id_is_equal_ExpectAndReturn(
        mpmm_v2_ctx.domain_ctx->domain_config->perf_id, perf_id, true);

    params = (struct mod_perf_controller_notification_params *)event.params;
    params->power_limit = domain_ctx->domain_config->min_power + 100;

    status = mpmm_v2_process_notification(&event, &resp_event);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL(1, mpmm_v2_ctx.domain_ctx->num_cores_online);
    TEST_ASSERT_TRUE(mpmm_v2_ctx.domain_ctx->core_ctx[0].online);
    TEST_ASSERT_EQUAL(1, mpmm_v2_ctx.domain_ctx->core_ctx[0].selected_gear);
    TEST_ASSERT_EQUAL(
        params->power_limit, mpmm_v2_ctx.domain_ctx->current_power_level);
    TEST_ASSERT_FALSE(mpmm_v2_ctx.domain_ctx->core_ctx[0].pd_blocked);
    TEST_ASSERT_FALSE(mpmm_v2_ctx.domain_ctx->wait_for_perf_transition);
    TEST_ASSERT_FALSE(resp_event.is_delayed_response);
    TEST_ASSERT_EQUAL(event.cookie, mpmm_v2_ctx.domain_ctx->core_ctx[0].cookie);
}

void utest_mpmm_v2_controller_noti_waiting_perf_transition_pd_blocked_success(
    void)
{
    int status;
    struct mod_perf_controller_notification_params *params;
    struct fwk_event event = { .target_id =
                                   FWK_ID_EVENT(FWK_MODULE_IDX_MPMM_V2, 0) };
    struct fwk_event resp_event = { 0 };
    fwk_id_t perf_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_PERF_CONTROLLER, 0);
    struct mod_mpmm_v2_domain_ctx *domain_ctx = mpmm_v2_ctx.domain_ctx;

    domain_ctx->num_cores_online = 1;
    domain_ctx->wait_for_perf_transition = true;
    domain_ctx->core_ctx[0].pd_blocked = true;
    domain_ctx->core_ctx[0].selected_gear = 1;
    domain_ctx->current_power_level = __LINE__;

    fwk_module_is_valid_element_id_ExpectAndReturn(event.target_id, true);
    fwk_id_get_element_idx_ExpectAndReturn(event.target_id, 0);
    fwk_id_get_module_idx_ExpectAndReturn(
        event.id, FWK_MODULE_IDX_PERF_CONTROLLER);
    fwk_id_is_equal_ExpectAndReturn(
        mpmm_v2_ctx.domain_ctx->domain_config->perf_id, perf_id, true);
    fwk_get_delayed_response_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    __fwk_put_event_ExpectAnyArgsAndReturn(FWK_SUCCESS);

    params = (struct mod_perf_controller_notification_params *)event.params;
    params->power_limit = domain_ctx->domain_config->min_power + 100;

    status = mpmm_v2_process_notification(&event, &resp_event);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL(1, mpmm_v2_ctx.domain_ctx->num_cores_online);
    TEST_ASSERT_TRUE(mpmm_v2_ctx.domain_ctx->core_ctx[0].online);
    TEST_ASSERT_EQUAL(1, mpmm_v2_ctx.domain_ctx->core_ctx[0].selected_gear);
    TEST_ASSERT_EQUAL(
        params->power_limit, mpmm_v2_ctx.domain_ctx->current_power_level);
    TEST_ASSERT_FALSE(mpmm_v2_ctx.domain_ctx->core_ctx[0].pd_blocked);
    TEST_ASSERT_FALSE(mpmm_v2_ctx.domain_ctx->wait_for_perf_transition);
    TEST_ASSERT_EQUAL(event.cookie, mpmm_v2_ctx.domain_ctx->core_ctx[0].cookie);
}

void utest_mpmm_v2_controller_noti_pd_blocked_delayed_resp_fail(void)
{
    int status;
    struct mod_perf_controller_notification_params *params;
    struct fwk_event event = { .target_id =
                                   FWK_ID_EVENT(FWK_MODULE_IDX_MPMM_V2, 0) };
    struct fwk_event resp_event = { 0 };
    fwk_id_t perf_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_PERF_CONTROLLER, 0);
    struct mod_mpmm_v2_domain_ctx *domain_ctx = mpmm_v2_ctx.domain_ctx;

    domain_ctx->num_cores_online = 1;
    domain_ctx->wait_for_perf_transition = true;
    domain_ctx->core_ctx[0].pd_blocked = true;
    domain_ctx->core_ctx[0].selected_gear = 1;
    domain_ctx->current_power_level = __LINE__;

    fwk_module_is_valid_element_id_ExpectAndReturn(event.target_id, true);
    fwk_id_get_module_idx_ExpectAndReturn(
        event.id, FWK_MODULE_IDX_PERF_CONTROLLER);
    fwk_id_is_equal_ExpectAndReturn(
        mpmm_v2_ctx.domain_ctx->domain_config->perf_id, perf_id, true);
    fwk_get_delayed_response_ExpectAnyArgsAndReturn(FWK_E_PARAM);

    params = (struct mod_perf_controller_notification_params *)event.params;
    params->power_limit = domain_ctx->domain_config->min_power + 100;

    status = mpmm_v2_process_notification(&event, &resp_event);
    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
    TEST_ASSERT_EQUAL(1, mpmm_v2_ctx.domain_ctx->num_cores_online);
    TEST_ASSERT_TRUE(mpmm_v2_ctx.domain_ctx->core_ctx[0].online);
    TEST_ASSERT_EQUAL(1, mpmm_v2_ctx.domain_ctx->core_ctx[0].selected_gear);
    TEST_ASSERT_EQUAL(
        params->power_limit, mpmm_v2_ctx.domain_ctx->current_power_level);
    TEST_ASSERT_FALSE(mpmm_v2_ctx.domain_ctx->core_ctx[0].pd_blocked);
    TEST_ASSERT_FALSE(mpmm_v2_ctx.domain_ctx->wait_for_perf_transition);
    TEST_ASSERT_EQUAL(event.cookie, mpmm_v2_ctx.domain_ctx->core_ctx[0].cookie);
}

void utest_mpmm_v2_controller_noti_pd_blocked_put_event_fail(void)
{
    int status;
    struct mod_perf_controller_notification_params *params;
    struct fwk_event event = { .target_id =
                                   FWK_ID_EVENT(FWK_MODULE_IDX_MPMM_V2, 0) };
    struct fwk_event resp_event = { 0 };
    fwk_id_t perf_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_PERF_CONTROLLER, 0);
    struct mod_mpmm_v2_domain_ctx *domain_ctx = mpmm_v2_ctx.domain_ctx;

    domain_ctx->num_cores_online = 1;
    domain_ctx->wait_for_perf_transition = true;
    domain_ctx->core_ctx[0].pd_blocked = true;
    domain_ctx->core_ctx[0].selected_gear = 1;
    domain_ctx->current_power_level = __LINE__;

    fwk_module_is_valid_element_id_ExpectAndReturn(event.target_id, true);
    fwk_id_get_module_idx_ExpectAndReturn(
        event.id, FWK_MODULE_IDX_PERF_CONTROLLER);
    fwk_id_is_equal_ExpectAndReturn(
        mpmm_v2_ctx.domain_ctx->domain_config->perf_id, perf_id, true);
    fwk_get_delayed_response_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    __fwk_put_event_ExpectAnyArgsAndReturn(FWK_E_PARAM);

    params = (struct mod_perf_controller_notification_params *)event.params;
    params->power_limit = domain_ctx->domain_config->min_power + 100;

    status = mpmm_v2_process_notification(&event, &resp_event);
    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
    TEST_ASSERT_EQUAL(1, mpmm_v2_ctx.domain_ctx->num_cores_online);
    TEST_ASSERT_TRUE(mpmm_v2_ctx.domain_ctx->core_ctx[0].online);
    TEST_ASSERT_EQUAL(1, mpmm_v2_ctx.domain_ctx->core_ctx[0].selected_gear);
    TEST_ASSERT_EQUAL(
        params->power_limit, mpmm_v2_ctx.domain_ctx->current_power_level);
    TEST_ASSERT_FALSE(mpmm_v2_ctx.domain_ctx->core_ctx[0].pd_blocked);
    TEST_ASSERT_FALSE(mpmm_v2_ctx.domain_ctx->wait_for_perf_transition);
    TEST_ASSERT_EQUAL(event.cookie, mpmm_v2_ctx.domain_ctx->core_ctx[0].cookie);
}

void utest_mpmm_v2_core_evaluate_gear_success(void)
{
    uint64_t cached_counters = 0;
    uint64_t delta = 0;
    struct mpmm_reg mpmm_v2 = { .MPMMCR = MPMM_MPMMCR_EN_MASK };
    struct mod_mpmm_v2_core_ctx core_ctx;
    struct mod_mpmm_v2_domain_ctx *domain_ctx = mpmm_v2_ctx.domain_ctx;
    const struct mod_mpmm_v2_domain_config *config = domain_ctx->domain_config;

    cached_counters = 1;
    test_amu_counter[AMU_AUX0] = cached_counters + config->base_throtl_count;
    /* Here, expectation for get_counter can be stated */
    test_amu_api.get_counters = amu_mmap_get_counters;
    amu_mmap_get_counters_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    amu_mmap_get_counters_ReturnMemThruPtr_counter_buff(
        &(test_amu_counter[AMU_AUX0]), sizeof(uint64_t) * config->num_of_gears);

    core_ctx.mpmm_v2 = &mpmm_v2;
    core_ctx.base_aux_counter_id =
        FWK_ID_SUB_ELEMENT(FWK_MODULE_IDX_AMU_MMAP, CORE0_IDX, AMU_AUX0);
    core_ctx.cached_counters = &cached_counters;
    core_ctx.delta = &delta;
    core_ctx.selected_gear = __LINE__;

    mpmm_v2_core_evaluate_gear(domain_ctx, &core_ctx);
    TEST_ASSERT_EQUAL(
        (domain_table[0].domain_config->num_of_gears - 1),
        core_ctx.selected_gear);
    TEST_ASSERT_EQUAL(test_amu_counter[AMU_AUX0], *core_ctx.cached_counters);
    TEST_ASSERT_EQUAL(config->base_throtl_count, *core_ctx.delta);
}

void utest_mpmm_v2_core_evaluate_gear_counter_not_enabled(void)
{
    struct mpmm_reg mpmm_v2 = { .MPMMCR = 0 };
    struct mod_mpmm_v2_core_ctx core_ctx;

    core_ctx.mpmm_v2 = &mpmm_v2;

    mpmm_v2_core_evaluate_gear(&domain_table[0], &core_ctx);
    TEST_ASSERT_EQUAL(
        domain_table[0].domain_config->num_of_gears, core_ctx.selected_gear);
}

void utest_mpmm_v2_domain_set_gears_success(void)
{
    struct mpmm_reg mpmm_v2 = { 0 };

    domain_table[0].core_ctx[0].mpmm_v2 = &mpmm_v2;
    domain_table[0].core_ctx[0].online = true;

    mpmm_v2_domain_set_gears(&domain_table[0]);
    TEST_ASSERT_EQUAL(
        (1 << MPMM_MPMMCR_GEAR_POS),
        domain_table[0].core_ctx[0].mpmm_v2->MPMMCR);
}

void utest_get_domain_ctx_null(void)
{
    struct mod_mpmm_v2_domain_ctx *domain_ctx;
    fwk_id_t elem_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_MPMM_V2, 0);

    fwk_id_get_element_idx_ExpectAndReturn(
        elem_id, mpmm_v2_ctx.mpmm_v2_domain_count);

    domain_ctx = get_domain_ctx(elem_id);
    TEST_ASSERT_EQUAL(NULL, domain_ctx);
}

void utest_mpmm_v2_core_gear_policy_highest_gear(void)
{
    uint32_t gear;
    uint64_t delta = 0;
    struct mod_mpmm_v2_core_ctx core_ctx;

    core_ctx.delta = &delta;

    gear = mpmm_v2_core_gear_policy(&domain_table[0], &core_ctx);
    TEST_ASSERT_EQUAL((domain_table[0].domain_config->num_of_gears - 1), gear);
}

void utest_mpmm_v2_monitor_and_control_no_cores_online(void)
{
    struct mod_mpmm_v2_domain_ctx *domain_ctx = mpmm_v2_ctx.domain_ctx;
    uint32_t prev_power_limit = domain_ctx->power_limit;

    domain_ctx->num_cores_online = 0;

    mpmm_v2_monitor_and_control(domain_ctx);
    TEST_ASSERT_EQUAL(prev_power_limit, domain_ctx->power_limit);
}

void utest_mpmm_v2_core_counters_delta_read_two_counter(void)
{
    struct mod_mpmm_v2_domain_ctx *domain_ctx = mpmm_v2_ctx.domain_ctx;
    const struct mod_mpmm_v2_domain_config *config;
    /* Initialize cached_counter to a random value to check delta calculation
     */
    uint64_t cached_counters[2] = { 0x1111, 0x2222 };
    uint64_t delta[2] = { 0 };
    struct mod_mpmm_v2_core_ctx core_ctx;
    test_amu_counter[AMU_AUX0] = 0xAAAA;
    test_amu_counter[AMU_AUX0 + 1] = 0xBBBB;

    core_ctx.base_aux_counter_id =
        FWK_ID_SUB_ELEMENT(FWK_MODULE_IDX_AMU_MMAP, CORE0_IDX, AMU_AUX0);
    core_ctx.cached_counters = cached_counters;
    core_ctx.delta = delta;
    domain_ctx->domain_config = config =
        &fake_dom_conf[MPMM_V2_DOM_TWO_GEAR_COUNTER];
    test_amu_api.get_counters = amu_mmap_get_counters;
    amu_mmap_get_counters_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    amu_mmap_get_counters_ReturnMemThruPtr_counter_buff(
        &(test_amu_counter[AMU_AUX0]), sizeof(uint64_t) * config->num_of_gears);

    mpmm_v2_core_counters_delta(&domain_table[0], &core_ctx);

    TEST_ASSERT_EQUAL(test_amu_counter[AMU_AUX0], core_ctx.cached_counters[0]);
    TEST_ASSERT_EQUAL((test_amu_counter[AMU_AUX0] - 0x1111), core_ctx.delta[0]);

    TEST_ASSERT_EQUAL(test_amu_counter[AMU_AUX1], core_ctx.cached_counters[1]);
    TEST_ASSERT_EQUAL((test_amu_counter[AMU_AUX1] - 0x2222), core_ctx.delta[1]);
}

void utest_mpmm_v2_core_counters_delta_wraparound(void)
{
    struct mod_mpmm_v2_domain_ctx *domain_ctx = mpmm_v2_ctx.domain_ctx;
    const struct mod_mpmm_v2_domain_config *config = domain_ctx->domain_config;
    /* Initialize cached_counter to value close to max to trigger wraparound
     */
    uint64_t cached_counters = UINT64_MAX - 5;
    uint64_t delta = 0;
    struct mod_mpmm_v2_core_ctx core_ctx;
    test_amu_counter[AMU_AUX0] = 0xAAAA;
    test_amu_counter[AMU_AUX0 + 1] = 0xBBBB;

    core_ctx.base_aux_counter_id =
        FWK_ID_SUB_ELEMENT(FWK_MODULE_IDX_AMU_MMAP, CORE0_IDX, AMU_AUX0);
    core_ctx.cached_counters = &cached_counters;
    core_ctx.delta = &delta;
    test_amu_api.get_counters = amu_mmap_get_counters;
    amu_mmap_get_counters_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    amu_mmap_get_counters_ReturnMemThruPtr_counter_buff(
        &(test_amu_counter[AMU_AUX0]), sizeof(uint64_t) * config->num_of_gears);

    mpmm_v2_core_counters_delta(&domain_table[0], &core_ctx);
    TEST_ASSERT_EQUAL(test_amu_counter[AMU_AUX0], *core_ctx.cached_counters);
    TEST_ASSERT_EQUAL(test_amu_counter[AMU_AUX0] + 5, *core_ctx.delta);
}

void utest_mpmm_v2_core_counters_delta_read_fail(void)
{
    uint64_t cached_counters = UINT64_MAX;
    uint64_t delta = UINT64_MAX;
    struct mod_mpmm_v2_core_ctx core_ctx;

    core_ctx.base_aux_counter_id =
        FWK_ID_SUB_ELEMENT(FWK_MODULE_IDX_AMU_MMAP, CORE0_IDX, AMU_AUX0);
    core_ctx.cached_counters = &cached_counters;
    core_ctx.delta = &delta;
    test_amu_api.get_counters = amu_mmap_get_counters;
    amu_mmap_get_counters_ExpectAnyArgsAndReturn(FWK_E_RANGE);

    mpmm_v2_core_counters_delta(&domain_table[0], &core_ctx);
    /* cached_counters and delta should remain the same if read fail */
    TEST_ASSERT_EQUAL(UINT64_MAX, *core_ctx.cached_counters);
    TEST_ASSERT_EQUAL(UINT64_MAX, *core_ctx.delta);
}

int mod_mpmm_v2_test_main(void)
{
    UNITY_BEGIN();

    RUN_TEST(utest_mpmm_v2_init_success);
    RUN_TEST(utest_mpmm_v2_init_element_count_fail);

    RUN_TEST(utest_mpmm_v2_start_mod_id_success);
    RUN_TEST(utest_mpmm_v2_start_notif_subscribe_success);
    RUN_TEST(utest_mpmm_v2_start_notif_subscribe_pd_pre_transition_fail);
    RUN_TEST(utest_mpmm_v2_start_notif_subscribe_pd_post_transition_fail);
    RUN_TEST(utest_mpmm_v2_start_notif_subscribe_perf_controller_perf_set_fail);

    RUN_TEST(utest_mpmm_v2_element_init_two_core_success);
    RUN_TEST(utest_mpmm_v2_element_init_element_count_0_fail);
    RUN_TEST(utest_mpmm_v2_element_init_element_count_max_fail);
    RUN_TEST(utest_mpmm_v2_element_init_max_gear_count_fail);
    RUN_TEST(utest_mpmm_v2_element_init_num_gear_mismatch);

    RUN_TEST(utest_mpmm_v2_bind_first_round_success);
    RUN_TEST(utest_mpmm_v2_bind_invalid_module_success);
    RUN_TEST(utest_mpmm_v2_bind_module_bind_success);
    RUN_TEST(utest_mpmm_v2_bind_amu_bind_fail);

    RUN_TEST(utest_mpmm_v2_process_bind_request_success);
    RUN_TEST(utest_mpmm_v2_process_bind_request_id_not_equal);

    RUN_TEST(utest_mpmm_v2_process_notification_pre_state_to_on);
    RUN_TEST(utest_mpmm_v2_process_notification_post_state_to_off);
    RUN_TEST(utest_mpmm_v2_process_notification_core_idx_larger);
    RUN_TEST(utest_mpmm_v2_process_notification_no_perf_change);
    RUN_TEST(
        utest_mpmm_v2_process_notification_post_state_to_on_core_init_fail);

    RUN_TEST(utest_mpmm_v2_evaluate_power_limit_most_aggressive_gear);
    RUN_TEST(utest_mpmm_v2_evaluate_power_limit_least_aggressive_gear);
    RUN_TEST(
        utest_mpmm_v2_evaluate_power_limit_2_cores_online_most_aggressive_gear);
    RUN_TEST(utest_mpmm_v2_evaluate_power_limit_2_cores_online_mix_of_gears);
    RUN_TEST(
        utest_mpmm_v2_evaluate_power_limit_2_cores_online_least_aggressive_gear);

    RUN_TEST(utest_mpmm_v2_get_power_limit_success);
    RUN_TEST(utest_mpmm_v2_get_power_limit_domain_idx_not_found);
    RUN_TEST(utest_mpmm_v2_get_power_limit_no_cores_online);
    RUN_TEST(utest_mpmm_v2_get_power_limit_wait_for_perf_trasition);

    RUN_TEST(utest_mpmm_v2_controller_noti_not_waiting_perf_transition_success);
    RUN_TEST(
        utest_mpmm_v2_controller_noti_waiting_perf_transition_pd_unblocked_success);
    RUN_TEST(
        utest_mpmm_v2_controller_noti_waiting_perf_transition_pd_blocked_success);
    RUN_TEST(utest_mpmm_v2_controller_noti_pd_blocked_delayed_resp_fail);
    RUN_TEST(utest_mpmm_v2_controller_noti_pd_blocked_put_event_fail);

    RUN_TEST(utest_mpmm_v2_core_evaluate_gear_success);
    RUN_TEST(utest_mpmm_v2_core_evaluate_gear_counter_not_enabled);

    RUN_TEST(utest_mpmm_v2_domain_set_gears_success);

    RUN_TEST(utest_get_domain_ctx_null);

    RUN_TEST(utest_mpmm_v2_core_gear_policy_highest_gear);

    RUN_TEST(utest_mpmm_v2_monitor_and_control_no_cores_online);

    RUN_TEST(utest_mpmm_v2_core_counters_delta_read_two_counter);
    RUN_TEST(utest_mpmm_v2_core_counters_delta_wraparound);
    RUN_TEST(utest_mpmm_v2_core_counters_delta_read_fail);

    return UNITY_END();
}

#if !defined(TEST_ON_TARGET)
int main(void)
{
    return mod_mpmm_v2_test_main();
}
#endif
