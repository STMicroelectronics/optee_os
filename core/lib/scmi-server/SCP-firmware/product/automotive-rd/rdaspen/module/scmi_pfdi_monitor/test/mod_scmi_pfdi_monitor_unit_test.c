/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "unity.h"

#include <Mockfwk_id.h>
#include <Mockfwk_module.h>

#include <mod_pfdi_monitor.h>
#include <mod_scmi.h>
#include <mod_scmi_pfdi_monitor.h>

#include <fwk_module_idx.h>
#include <fwk_string.h>

#include UNIT_TEST_SRC
#include "config_scmi_pfdi_monitor.h"

#define PFDI_SUCCESS 0
#define PFDI_ERROR   1

#define SCMI_PFDI_MONITOR_PROTOCOL_ID UINT32_C(0x90)

uint32_t scmi_response[16];

void setUp(void)
{
    /* Do Nothing */
}

void tearDown(void)
{
    /* Do Nothing */
}

int scmi_respond(fwk_id_t service_id, const void *payload, size_t size)
{
    memcpy(scmi_response, payload, size);
    return FWK_SUCCESS;
}

static const struct mod_scmi_from_protocol_api scmi_api = {
    .respond = scmi_respond,
};

int pfdi_monitor_oor_status(fwk_id_t id, uint32_t status)
{
    return FWK_SUCCESS;
}

int pfdi_monitor_onl_status(fwk_id_t id, uint32_t status)
{
    return FWK_SUCCESS;
}

static const struct mod_pfdi_monitor_api pfdi_monitor_api = {
    .oor_status = pfdi_monitor_oor_status,
    .onl_status = pfdi_monitor_onl_status,
};

bool fwk_id_is_equal_stub(fwk_id_t left, fwk_id_t right, int _)
{
    return left.value == right.value;
}

void scmi_pfdi_monitor_test(void)
{
    int status;

    /* Initialize module */
    status = scmi_pfdi_monitor_init(
        fwk_module_id_scmi_pfdi_monitor,
        SI0_MOD_SCMI_PFDI_MONITOR_EIDX_COUNT,
        NULL);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

    for (uint32_t idx = 0; idx < SI0_MOD_SCMI_PFDI_MONITOR_EIDX_COUNT; idx++) {
        fwk_id_t element_id =
            FWK_ID_ELEMENT(FWK_MODULE_IDX_SCMI_PFDI_MONITOR, idx);

        fwk_id_get_element_idx_ExpectAndReturn(element_id, idx);

        /* Initialize elements */
        status = scmi_pfdi_monitor_element_init(
            element_id, 1, (const void *)element_table[idx].data);
        TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

        fwk_id_is_type_ExpectAndReturn(element_id, FWK_ID_TYPE_MODULE, false);

        /* Bind elements */
        status = scmi_pfdi_monitor_bind(element_id, 0U);
        TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

        ctx.scmi_api = &scmi_api;
        ctx.pfdi_monitor_api = &pfdi_monitor_api;

        fwk_id_t api_id = FWK_ID_API(FWK_MODULE_IDX_SCMI_PFDI_MONITOR, 0);
        const struct mod_scmi_to_protocol_api *api = NULL;

        fwk_id_is_equal_ExpectAndReturn(
            fwk_module_id_scmi, fwk_module_id_scmi, true);

        /* Bind to PFDI monitor module */
        status = scmi_pfdi_monitor_process_bind_request(
            fwk_module_id_scmi, element_id, api_id, (const void **)&api);
        TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
        TEST_ASSERT_NOT_NULL(api);

        uint8_t protocol_id;

        status = api->get_scmi_protocol_id(fwk_module_id_scmi, &protocol_id);
        TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
        TEST_ASSERT_EQUAL(SCMI_PFDI_MONITOR_PROTOCOL_ID, protocol_id);

        uint32_t payload[] = { PFDI_SUCCESS };

        fwk_id_is_equal_StubWithCallback(fwk_id_is_equal_stub);

        status = api->message_handler(
            fwk_module_id_scmi,
            FWK_ID_ELEMENT(FWK_MODULE_IDX_SCMI, idx),
            payload,
            sizeof(payload),
            SCMI_PFDI_MONITOR_OOR_STATUS);
        TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
        TEST_ASSERT_EQUAL(scmi_response[0], FWK_SUCCESS);

        status = api->message_handler(
            fwk_module_id_scmi,
            FWK_ID_ELEMENT(FWK_MODULE_IDX_SCMI, idx),
            payload,
            sizeof(payload),
            SCMI_PFDI_MONITOR_ONL_STATUS);
        TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
        TEST_ASSERT_EQUAL(scmi_response[0], FWK_SUCCESS);
    }
}

int scmi_pfdi_monitor_test_main(void)
{
    UNITY_BEGIN();

    RUN_TEST(scmi_pfdi_monitor_test);

    return UNITY_END();
}

#if !defined(TEST_ON_TARGET)
int main(void)
{
    return scmi_pfdi_monitor_test_main();
}
#endif
