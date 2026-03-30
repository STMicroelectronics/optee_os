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

#include <mod_pfdi_monitor.h>
#include <mod_timer.h>

#include <fwk_module_idx.h>

#include UNIT_TEST_SRC
#include "config_pfdi_monitor.h"

#define PFDI_SUCCESS 0
#define PFDI_ERROR   1

enum alarm_state {
    ALARM_STOPPED,
    ALARM_STARTED,
};

enum alarm_state alarm_state[SI0_MOD_PFDI_MONITOR_EIDX_COUNT] = {
    ALARM_STOPPED,
    ALARM_STOPPED,
};

int alarm_start(
    fwk_id_t alarm_id,
    uint32_t microseconds,
    enum mod_timer_alarm_type type,
    void (*callback)(uintptr_t param),
    uintptr_t param)
{
    unsigned int element_idx = fwk_id_get_element_idx(alarm_id);

    switch (alarm_state[element_idx]) {
    case ALARM_STARTED:
    case ALARM_STOPPED:
        alarm_state[element_idx] = ALARM_STARTED;
        break;
    }
    return FWK_SUCCESS;
}

int alarm_stop(fwk_id_t alarm_id)
{
    int ret = FWK_SUCCESS;
    unsigned int element_idx = fwk_id_get_element_idx(alarm_id);

    switch (alarm_state[element_idx]) {
    case ALARM_STOPPED:
        ret = FWK_E_STATE;
        break;
    case ALARM_STARTED:
        alarm_state[element_idx] = ALARM_STOPPED;
        break;
    }
    return ret;
}

static struct mod_timer_alarm_api alarm_api = {
    .start = alarm_start,
    .stop = alarm_stop,
};

void setUp(void)
{
    /* Do Nothing */
}

void tearDown(void)
{
    /* Do Nothing */
}

void pfdi_monitor_state_machine(void)
{
    int status;

    /* Initialize module */
    status = pfdi_monitor_init(
        fwk_module_id_pfdi_monitor, SI0_MOD_PFDI_MONITOR_EIDX_COUNT, NULL);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

    for (uint32_t idx = 0; idx < SI0_MOD_PFDI_MONITOR_EIDX_COUNT; idx++) {
        fwk_id_t element_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_PFDI_MONITOR, idx);

        fwk_id_get_element_idx_ExpectAndReturn(element_id, idx);

        /* Initialize elements */
        status = pfdi_monitor_element_init(
            element_id, 1, (const void *)element_table[idx].data);
        TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

        fwk_id_is_type_ExpectAndReturn(element_id, FWK_ID_TYPE_MODULE, false);
        fwk_id_get_element_idx_ExpectAndReturn(element_id, idx);
        fwk_module_bind_IgnoreAndReturn(FWK_SUCCESS);

        /* Bind elements */
        status = pfdi_monitor_bind(element_id, 0U);
        TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

        fwk_id_t api_id = FWK_ID_API(
            FWK_MODULE_IDX_PFDI_MONITOR, MOD_PFDI_MONITOR_API_IDX_PFDI_MONITOR);
        const struct mod_pfdi_monitor_api *api = NULL;
        fwk_id_get_api_idx_ExpectAndReturn(
            api_id, MOD_PFDI_MONITOR_API_IDX_PFDI_MONITOR);

        /* Bind to PFDI monitor module */
        status = pfdi_monitor_process_bind_request(
            fwk_module_id_test_module, element_id, api_id, (const void **)&api);
        TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
        TEST_ASSERT_NOT_NULL(api);

        fwk_id_is_type_ExpectAndReturn(element_id, FWK_ID_TYPE_MODULE, false);
        fwk_id_get_element_idx_IgnoreAndReturn(idx);
        fwk_module_get_element_name_IgnoreAndReturn("Test");

        fwk_id_t pd_transition_source_id =
            FWK_ID_ELEMENT(FWK_MODULE_IDX_POWER_DOMAIN, idx);
        fwk_id_build_element_id_ExpectAndReturn(
            fwk_module_id_power_domain, idx, pd_transition_source_id);

        fwk_id_t pd_transition_notification_id = FWK_ID_NOTIFICATION_INIT(
            FWK_MODULE_IDX_POWER_DOMAIN,
            MOD_PD_NOTIFICATION_IDX_POWER_STATE_TRANSITION);
        fwk_notification_subscribe_ExpectAndReturn(
            pd_transition_notification_id,
            pd_transition_source_id,
            element_id,
            FWK_SUCCESS);

        ctx.core_ctx_table[idx].alarm_api = &alarm_api;

        /* Start PFDI monitor */
        status = pfdi_monitor_start(element_id);
        TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

        struct fwk_event event = {
            .target_id = element_id,
            .id = FWK_ID_EVENT(
                FWK_MODULE_IDX_PFDI_MONITOR, PFDI_MONITOR_EVENT_IDX_OOR_STATUS),
            .params = { PFDI_SUCCESS },
        };

        fwk_id_get_event_idx_IgnoreAndReturn(PFDI_MONITOR_EVENT_IDX_OOR_STATUS);

        /* Send a successfull PFDI OoR event */
        status = pfdi_monitor_process_event(&event, NULL);
        TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

        event.id = FWK_ID_EVENT(
            FWK_MODULE_IDX_PFDI_MONITOR, PFDI_MONITOR_EVENT_IDX_ONL_STATUS);
        fwk_id_get_event_idx_IgnoreAndReturn(PFDI_MONITOR_EVENT_IDX_ONL_STATUS);

        /* Send a successfull boot completion PFDI Online event */
        status = pfdi_monitor_process_event(&event, NULL);
        TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

        /* Send a successfull PFDI Online event */
        status = pfdi_monitor_process_event(&event, NULL);
        TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

        event.params[0] = PFDI_ERROR;

        /* Send a failed PFDI Online event */
        status = pfdi_monitor_process_event(&event, NULL);
        TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

        TEST_ASSERT_EQUAL(ALARM_STOPPED, alarm_state[idx]);

        event.params[0] = PFDI_SUCCESS;

        /* Send a successfull PFDI Online event */
        status = pfdi_monitor_process_event(&event, NULL);
        TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

        TEST_ASSERT_EQUAL(ALARM_STARTED, alarm_state[idx]);
    }
}

int pfdi_monitor_test_main(void)
{
    UNITY_BEGIN();

    RUN_TEST(pfdi_monitor_state_machine);

    return UNITY_END();
}

#if !defined(TEST_ON_TARGET)
int main(void)
{
    return pfdi_monitor_test_main();
}
#endif
