/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config_system_coordinator.h"
#include "scp_unity.h"
#include "unity.h"

#include <Mockfwk_mm.h>
#include <Mockfwk_module.h>
#include <Mockmod_system_coordinator_extra.h>
#include <internal/Mockfwk_core_internal.h>

#include <mod_system_coordinator_extra.h>

#include <fwk_id.h>

#include UNIT_TEST_SRC

#define CYCLE_COUNT 100
#define MOD_SYSTEM_COORDINATOR_ID \
    FWK_ID_MODULE(FWK_MODULE_IDX_SYSTEM_COORDINATOR)

/* Runtime allocated variables */

struct mod_system_coordinator_phase_ctx phase[SYS_COOR_PHASE_COUNT];
struct mod_system_coordinator_phase_api phase_api = {
    .phase_handler = &phase_api_stub,
};

struct mod_timer_alarm_api alarm_api_driver = {
    .start = start_alarm_api,
};

void setUp(void)
{
    struct mod_system_coordinator_phase_ctx *ctx;

    memset(&system_coordinator_ctx, 0, sizeof(system_coordinator_ctx));

    system_coordinator_ctx.config = module_config.data;
    system_coordinator_ctx.phase_ctx = phase;
    system_coordinator_ctx.cycle_alarm_api = &alarm_api_driver;
    system_coordinator_ctx.phase_alarm_api = &alarm_api_driver;
    system_coordinator_ctx.phase_count = SYS_COOR_PHASE_COUNT;
    system_coordinator_ctx.cycle_count = CYCLE_COUNT;

    /* Initialise each phase */
    for (int i = 0; i < SYS_COOR_PHASE_COUNT; i++) {
        ctx = &system_coordinator_ctx.phase_ctx[i];
        ctx->phase_config =
            (struct mod_system_coordinator_phase_config *)phase_config[i].data;
        ctx->phase_api = &phase_api;
    }
}

void tearDown(void)
{
    Mockmod_system_coordinator_extra_Verify();
    Mockmod_system_coordinator_extra_Destroy();
    Mockfwk_mm_Verify();
    Mockfwk_mm_Destroy();
    Mockfwk_module_Verify();
    Mockfwk_module_Destroy();
}

int start_alarm_callback(
    fwk_id_t alarm_id,
    unsigned int microseconds,
    enum mod_timer_alarm_type type,
    cmock_mod_system_coordinator_extra_func_ptr1 callback,
    uintptr_t param,
    int cmock_num_calls)
{
    struct phase_event_params *evt_params = (struct phase_event_params *)param;

    TEST_ASSERT_EQUAL(SYS_COOR_PHASE_TWO, evt_params->phase_idx);

    return FWK_SUCCESS;
}

void utest_cycle_alarm_callback_cycle_increment(void)
{
    struct fwk_event event = {
        .source_id = FWK_ID_MODULE(FWK_MODULE_IDX_SYSTEM_COORDINATOR),
        .target_id = FWK_ID_MODULE(FWK_MODULE_IDX_SYSTEM_COORDINATOR),
        .id = mod_system_coordinator_event_phase,
    };
    struct phase_event_params *evt_params =
        (struct phase_event_params *)event.params;

    evt_params->phase_idx = SYS_COOR_PHASE_ONE;
    evt_params->cycle_count = CYCLE_COUNT + 1;

    __fwk_put_event_ExpectAndReturn(&event, FWK_SUCCESS);

    system_coordinator_cycle_alarm_callback((uintptr_t)NULL);

    TEST_ASSERT_EQUAL(CYCLE_COUNT + 1, system_coordinator_ctx.cycle_count);
}

void utest_cycle_alarm_callback_cycle_count_wraparound(void)
{
    struct fwk_event event = {
        .source_id = FWK_ID_MODULE(FWK_MODULE_IDX_SYSTEM_COORDINATOR),
        .target_id = FWK_ID_MODULE(FWK_MODULE_IDX_SYSTEM_COORDINATOR),
        .id = mod_system_coordinator_event_phase,
    };
    struct phase_event_params *evt_params =
        (struct phase_event_params *)event.params;

    system_coordinator_ctx.cycle_count = UINT32_MAX;
    evt_params->phase_idx = SYS_COOR_PHASE_ONE;
    evt_params->cycle_count = 0;

    __fwk_put_event_ExpectAndReturn(&event, FWK_SUCCESS);

    system_coordinator_cycle_alarm_callback((uintptr_t)NULL);

    TEST_ASSERT_EQUAL(0, system_coordinator_ctx.cycle_count);
}

void utest_process_current_phase_phase_count_invalid(void)
{
    int status = FWK_E_INIT;
    struct phase_event_params params = {
        .phase_idx = SYS_COOR_PHASE_COUNT,
        .cycle_count = CYCLE_COUNT,
    };

    status = process_current_phase(&params);

    TEST_ASSERT_EQUAL(FWK_E_RANGE, status);
}

void utest_process_current_phase_cycle_count_mismatch(void)
{
    int status = FWK_E_INIT;
    struct phase_event_params params = {
        .phase_idx = SYS_COOR_PHASE_ONE,
        .cycle_count = CYCLE_COUNT - 1,
    };

    status = process_current_phase(&params);

    TEST_ASSERT_EQUAL(FWK_E_STATE, status);
}

void utest_process_current_phase_start_phase_timer(void)
{
    int status = FWK_E_INIT;
    struct phase_event_params params = {
        .phase_idx = SYS_COOR_PHASE_ONE,
        .cycle_count = CYCLE_COUNT,
    };

    start_alarm_api_StubWithCallback(start_alarm_callback);

    phase_api_stub_ExpectAndReturn(FWK_SUCCESS);

    status = process_current_phase(&params);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void utest_process_current_phase_alarm_error(void)
{
    int status = FWK_E_INIT;
    struct phase_event_params params = {
        .phase_idx = SYS_COOR_PHASE_ONE,
        .cycle_count = CYCLE_COUNT,
    };

    start_alarm_api_ExpectAnyArgsAndReturn(FWK_E_PARAM);

    status = process_current_phase(&params);

    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
}

void utest_process_current_phase_last_phase(void)
{
    int status = FWK_E_INIT;
    struct phase_event_params params = {
        .phase_idx = SYS_COOR_PHASE_COUNT - 1,
        .cycle_count = CYCLE_COUNT,
    };

    phase_api_stub_ExpectAndReturn(FWK_SUCCESS);

    status = process_current_phase(&params);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void utest_process_current_phase_phase_time_zero(void)
{
    int status = FWK_E_INIT;
    struct phase_event_params params = {
        .phase_idx = SYS_COOR_PHASE_TWO,
        .cycle_count = CYCLE_COUNT,
    };
    struct fwk_event event = {
        .source_id = FWK_ID_MODULE(FWK_MODULE_IDX_SYSTEM_COORDINATOR),
        .target_id = FWK_ID_MODULE(FWK_MODULE_IDX_SYSTEM_COORDINATOR),
        .id = mod_system_coordinator_event_phase,
    };
    struct phase_event_params *evt_params =
        (struct phase_event_params *)event.params;

    evt_params->phase_idx = SYS_COOR_PHASE_THREE;
    evt_params->cycle_count = CYCLE_COUNT;

    phase_api_stub_ExpectAndReturn(FWK_SUCCESS);
    __fwk_put_event_ExpectAndReturn(&event, FWK_SUCCESS);

    status = process_current_phase(&params);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void utest_system_coordinator_init_zero_element(void)
{
    int status = FWK_E_INIT;
    unsigned int elem_count = 0;

    status = system_coordinator_init(
        fwk_module_id_system_coordinator, elem_count, NULL);
    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
}

void utest_system_coordinator_init_success(void)
{
    int status = FWK_E_INIT;
    size_t elem_count = SYS_COOR_PHASE_COUNT;
    struct mod_system_coordinator_phase_ctx phase[SYS_COOR_PHASE_COUNT];

    /* Reset system_coordinator_ctx to let init() initialise the context */
    memset(&system_coordinator_ctx, 0, sizeof(system_coordinator_ctx));

    fwk_mm_calloc_ExpectAndReturn(
        elem_count, sizeof(struct mod_system_coordinator_phase_ctx), phase);

    status = system_coordinator_init(
        fwk_module_id_system_coordinator, elem_count, module_config.data);
    TEST_ASSERT_EQUAL(elem_count, system_coordinator_ctx.phase_count);
    TEST_ASSERT_EQUAL_PTR(phase, system_coordinator_ctx.phase_ctx);
    TEST_ASSERT_EQUAL_PTR(module_config.data, system_coordinator_ctx.config);
    TEST_ASSERT_EQUAL_PTR(
        COORDINATOR_CYCLE_TIME, system_coordinator_ctx.config->cycle_us);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void utest_system_coordinator_element_init_element_count_invalid(void)
{
    int status = FWK_E_INIT;
    unsigned int sub_element_count = 0;
    fwk_id_t element_id;

    element_id =
        FWK_ID_ELEMENT(FWK_MODULE_IDX_SYSTEM_COORDINATOR, SYS_COOR_PHASE_COUNT);

    status =
        system_coordinator_element_init(element_id, sub_element_count, NULL);
    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
}

void utest_system_coordinator_element_init_success(void)
{
    int status = FWK_E_INIT;
    unsigned int sub_element_count = 0;
    fwk_id_t element_id;

    system_coordinator_ctx.phase_ctx->phase_config = NULL;

    /* Loop all phases */
    for (int i = 0; i < SYS_COOR_PHASE_COUNT; i++) {
        element_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_SYSTEM_COORDINATOR, i);
        status = system_coordinator_element_init(
            element_id, sub_element_count, phase_config[i].data);
        TEST_ASSERT_EQUAL_PTR(
            phase_config[i].data,
            system_coordinator_ctx.phase_ctx[i].phase_config);
        TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    }
}

void utest_system_coordinator_post_init_total_phase_more_than_cycle_time(void)
{
    int status = FWK_E_INIT;
    struct mod_system_coordinator_phase_config fake_phase_time = {
        .phase_us = COORDINATOR_CYCLE_TIME,
    };

    /* Replace phase one time with a bigger value like cycle time */
    system_coordinator_ctx.phase_ctx[0].phase_config = &fake_phase_time;

    status = system_coordinator_post_init(fwk_module_id_system_coordinator);
    TEST_ASSERT_EQUAL(FWK_E_SUPPORT, status);
}

void utest_system_coordinator_post_init_total_phase_less_than_cycle_time(void)
{
    int status = FWK_E_INIT;

    status = system_coordinator_post_init(fwk_module_id_system_coordinator);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void utest_system_coordinator_bind_module_success(void)
{
    int status = FWK_E_INIT;
    unsigned int round = 0;

    fwk_module_is_valid_module_id_ExpectAndReturn(
        fwk_module_id_system_coordinator, true);
    fwk_module_bind_ExpectAndReturn(
        system_coordinator_ctx.config->cycle_alarm_id,
        MOD_TIMER_API_ID_ALARM,
        &system_coordinator_ctx.cycle_alarm_api,
        FWK_SUCCESS);

    fwk_module_bind_ExpectAndReturn(
        system_coordinator_ctx.config->phase_alarm_id,
        MOD_TIMER_API_ID_ALARM,
        &system_coordinator_ctx.phase_alarm_api,
        FWK_SUCCESS);

    status = system_coordinator_bind(fwk_module_id_system_coordinator, round);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void utest_system_coordinator_bind_element_success(void)
{
    int status = FWK_E_INIT;
    unsigned int round = 0;
    fwk_id_t element_id = FWK_ID_ELEMENT_INIT(
        FWK_MODULE_IDX_SYSTEM_COORDINATOR, SYS_COOR_PHASE_ONE);
    struct mod_system_coordinator_phase_config *cfg =
        (struct mod_system_coordinator_phase_config *)
            phase_config[SYS_COOR_PHASE_ONE]
                .data;

    fwk_module_is_valid_module_id_ExpectAndReturn(element_id, false);
    fwk_module_is_valid_element_id_ExpectAndReturn(element_id, true);
    fwk_module_bind_ExpectAndReturn(
        cfg->module_id,
        cfg->api_id,
        &system_coordinator_ctx.phase_ctx[SYS_COOR_PHASE_ONE].phase_api,
        FWK_SUCCESS);

    status = system_coordinator_bind(element_id, round);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void utest_system_coordinator_start_success(void)
{
    int status = FWK_E_INIT;

    start_alarm_api_ExpectAndReturn(
        system_coordinator_ctx.config->cycle_alarm_id,
        system_coordinator_ctx.config->cycle_us,
        MOD_TIMER_ALARM_TYPE_PERIODIC,
        system_coordinator_cycle_alarm_callback,
        0,
        FWK_SUCCESS);

    start_alarm_api_ExpectAnyArgsAndReturn(FWK_SUCCESS);

    phase_api_stub_ExpectAndReturn(FWK_SUCCESS);

    status = system_coordinator_start(fwk_module_id_system_coordinator);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL(0, system_coordinator_ctx.cycle_count);
}

void utest_system_coordinator_process_event_event_not_found(void)
{
    int status = FWK_E_INIT;
    struct fwk_event event;

    event.target_id =
        FWK_ID_ELEMENT(FWK_MODULE_IDX_SYSTEM_COORDINATOR, SYS_COOR_PHASE_ONE);
    /* Set incorrect event index*/
    event.id = FWK_ID_EVENT(
        FWK_MODULE_IDX_SYSTEM_COORDINATOR,
        MOD_SYSTEM_COORDINATOR_EVENT_IDX_COUNT);

    status = system_coordinator_process_event(&event, NULL);
    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
}

int system_coordinator_test_main(void)
{
    UNITY_BEGIN();

    RUN_TEST(utest_cycle_alarm_callback_cycle_increment);
    RUN_TEST(utest_cycle_alarm_callback_cycle_count_wraparound);

    RUN_TEST(utest_process_current_phase_phase_count_invalid);
    RUN_TEST(utest_process_current_phase_cycle_count_mismatch);
    RUN_TEST(utest_process_current_phase_start_phase_timer);
    RUN_TEST(utest_process_current_phase_alarm_error);
    RUN_TEST(utest_process_current_phase_last_phase);
    RUN_TEST(utest_process_current_phase_phase_time_zero);

    RUN_TEST(utest_system_coordinator_init_zero_element);
    RUN_TEST(utest_system_coordinator_init_success);

    RUN_TEST(utest_system_coordinator_element_init_element_count_invalid);
    RUN_TEST(utest_system_coordinator_element_init_success);

    RUN_TEST(
        utest_system_coordinator_post_init_total_phase_more_than_cycle_time);
    RUN_TEST(
        utest_system_coordinator_post_init_total_phase_less_than_cycle_time);

    RUN_TEST(utest_system_coordinator_bind_module_success);
    RUN_TEST(utest_system_coordinator_bind_element_success);

    RUN_TEST(utest_system_coordinator_start_success);

    RUN_TEST(utest_system_coordinator_process_event_event_not_found);

    return UNITY_END();
}

int main(void)
{
    return system_coordinator_test_main();
}
