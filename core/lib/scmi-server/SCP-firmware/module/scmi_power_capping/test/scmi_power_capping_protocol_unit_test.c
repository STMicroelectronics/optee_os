/*
 * Arm SCP/MCP Software
 * Copyright (c) 2023-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "scp_unity.h"
#include "string.h"
#include "unity.h"

#include <Mockfwk_id.h>
#include <Mockfwk_mm.h>
#include <Mockfwk_module.h>
#include <Mockfwk_notification.h>
#include <Mockmod_power_capping_extra.h>
#include <Mockmod_resource_perms_extra.h>
#include <Mockmod_scmi_extra.h>
#include <Mockscmi_power_capping_core.h>
#include <internal/Mockfwk_core_internal.h>

#include <mod_scmi_power_capping_unit_test.h>

#include <stdarg.h>

#include UNIT_TEST_SRC

#define EXPECT_RESPONSE(ret_payload, ret_payload_size) \
    respond_ExpectWithArrayAndReturn( \
        service_id_1, \
        (void *)&ret_payload, \
        ret_payload_size, \
        ret_payload_size, \
        FWK_SUCCESS)

#define EXPECT_RESPONSE_SUCCESS(ret_payload) \
    EXPECT_RESPONSE(ret_payload, sizeof(ret_payload))

#define EXPECT_RESPONSE_ERROR(ret_payload) \
    EXPECT_RESPONSE(ret_payload, sizeof(ret_payload.status))

#define TEST_SCMI_COMMAND_NO_PAYLOAD(message_id) \
    do { \
        status = handler_table[message_id]( \
            service_id_1, (void *)&dummy_protocol_id); \
        TEST_ASSERT_EQUAL(status, FWK_SUCCESS); \
    } while (0)

#define TEST_SCMI_COMMAND(message_id, cmd_payload) \
    do { \
        status = \
            handler_table[message_id](service_id_1, (void *)&cmd_payload); \
        TEST_ASSERT_EQUAL(status, FWK_SUCCESS); \
    } while (0)

#define RESOURCE_PERMISSION_RESOURCE_PASS_TEST() \
    do { \
        get_agent_id_ExpectAnyArgsAndReturn(FWK_SUCCESS); \
        agent_has_resource_permission_ExpectAnyArgsAndReturn( \
            MOD_RES_PERMS_ACCESS_ALLOWED); \
    } while (0)

static int status;

static const struct mod_scmi_from_protocol_api scmi_api = {
    .respond = respond,
    .get_agent_id = get_agent_id,
    .get_agent_count = get_agent_count,
    .scmi_message_validation = mod_scmi_from_protocol_api_scmi_frame_validation,
};

#ifdef BUILD_HAS_MOD_RESOURCE_PERMS
static const struct mod_res_permissions_api res_perms_api = {
    .agent_has_protocol_permission = agent_has_protocol_permission,
    .agent_has_resource_permission = agent_has_resource_permission,
};
#endif

#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
static const struct mod_scmi_notification_api scmi_notification_api = {
    .scmi_notification_init = scmi_notification_init,
    .scmi_notification_add_subscriber = scmi_notification_add_subscriber,
    .scmi_notification_remove_subscriber = scmi_notification_remove_subscriber,
    .scmi_notification_notify = scmi_notification_notify,
};
#endif

static fwk_id_t service_id_1 =
    FWK_ID_ELEMENT_INIT(FAKE_SCMI_MODULE_ID, FAKE_SERVICE_IDX_1);
static fwk_id_t dummy_protocol_id;

static const uint32_t message_handler_helper_payload = __LINE__;

/* Test functions */
/* Initialize the tests */
static void test_init(void)
{
    pcapping_protocol_ctx.scmi_api = &scmi_api;
#ifdef BUILD_HAS_MOD_RESOURCE_PERMS
    pcapping_protocol_ctx.res_perms_api = &res_perms_api;
#endif

#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
    pcapping_protocol_ctx.scmi_notification_api = &scmi_notification_api;
#endif
}

void setUp(void)
{
    status = FWK_E_STATE;
}

void tearDown(void)
{
    Mockmod_scmi_extra_Verify();
    Mockfwk_id_Verify();
#ifdef BUILD_HAS_MOD_RESOURCE_PERMS
    Mockmod_resource_perms_extra_Verify();
#endif
}

void utest_get_scmi_protocol_id(void)
{
    uint8_t scmi_protocol_id;

    status = scmi_power_capping_get_scmi_protocol_id(
        dummy_protocol_id, &scmi_protocol_id);

    TEST_ASSERT_EQUAL(scmi_protocol_id, MOD_SCMI_PROTOCOL_ID_POWER_CAPPING);
    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
}

void utest_message_handler_scmi_validation_respond_error(void)
{
    fwk_id_t protocol_id = dummy_protocol_id;
    const uint32_t *payload = &message_handler_helper_payload;
    unsigned int message_id = __LINE__;
    int validation_result = SCMI_PROTOCOL_ERROR;

    mod_scmi_from_protocol_api_scmi_frame_validation_ExpectAndReturn(
        MOD_SCMI_PROTOCOL_ID_POWER_CAPPING,
        service_id_1,
        payload,
        sizeof(*payload),
        message_id,
        payload_size_table,
        (unsigned int)MOD_SCMI_POWER_CAPPING_COMMAND_COUNT,
        handler_table,
        validation_result);

    EXPECT_RESPONSE(validation_result, sizeof(validation_result));

    scmi_power_capping_message_handler(
        protocol_id, service_id_1, payload, sizeof(*payload), message_id);
}

int message_handler_helper_mock(fwk_id_t service_id, const uint32_t *payload)
{
    TEST_ASSERT_EQUAL_MEMORY(&service_id, &service_id_1, sizeof(service_id));
    TEST_ASSERT_EQUAL(message_handler_helper_payload, *payload);

    return FWK_SUCCESS;
}

void utest_message_handler_scmi_validation_respond_success(void)
{
    fwk_id_t protocol_id = dummy_protocol_id;
    const uint32_t *payload = &message_handler_helper_payload;
    unsigned int message_id = MOD_SCMI_PROTOCOL_VERSION;
    int validation_result = SCMI_SUCCESS;

    mod_scmi_from_protocol_api_scmi_frame_validation_ExpectAndReturn(
        MOD_SCMI_PROTOCOL_ID_POWER_CAPPING,
        service_id_1,
        payload,
        sizeof(*payload),
        message_id,
        payload_size_table,
        (unsigned int)MOD_SCMI_POWER_CAPPING_COMMAND_COUNT,
        handler_table,
        validation_result);
    handler_table_t message_handler_temp_ptr = handler_table[message_id];
    handler_table[message_id] = message_handler_helper_mock;

    scmi_power_capping_message_handler(
        protocol_id, service_id_1, payload, sizeof(*payload), message_id);

    handler_table[message_id] = message_handler_temp_ptr;
}

void utest_message_handler_protocol_version(void)
{
    struct scmi_protocol_version_p2a ret_payload = {
        .status = SCMI_SUCCESS,
        .version = SCMI_PROTOCOL_VERSION_POWER_CAPPING,
    };

    EXPECT_RESPONSE_SUCCESS(ret_payload);
    TEST_SCMI_COMMAND_NO_PAYLOAD(MOD_SCMI_PROTOCOL_VERSION);
}

void utest_message_handler_protocol_attributes(void)
{
    uint32_t domain_count = FAKE_POWER_CAPPING_IDX_COUNT;

    struct scmi_protocol_attributes_p2a ret_payload = {
        .status = SCMI_SUCCESS,
        .attributes = domain_count,
    };

    pcapping_core_get_domain_count_ExpectAndReturn(domain_count);

    EXPECT_RESPONSE_SUCCESS(ret_payload);
    TEST_SCMI_COMMAND_NO_PAYLOAD(MOD_SCMI_PROTOCOL_ATTRIBUTES);
}

void utest_message_handler_protocol_msg_attributes_supported_msgs(void)
{
    uint32_t message_id;
    struct scmi_protocol_message_attributes_a2p cmd_payload;
    struct scmi_protocol_message_attributes_p2a ret_payload = {
        .status = SCMI_SUCCESS
    };
    /* Test all supported messages */

    for (message_id = 0; message_id < MOD_SCMI_POWER_CAPPING_CAP_SET;
         message_id++) {
        cmd_payload.message_id = message_id;

#ifdef BUILD_HAS_MOD_RESOURCE_PERMS
        RESOURCE_PERMISSION_RESOURCE_PASS_TEST();
#endif
        EXPECT_RESPONSE_SUCCESS(ret_payload);
        TEST_SCMI_COMMAND(MOD_SCMI_PROTOCOL_MESSAGE_ATTRIBUTES, cmd_payload);
    }
}

void utest_message_handler_domain_attributes_valid(void)
{
    struct scmi_power_capping_domain_attributes_a2p cmd_payload = {
        .domain_id = FAKE_POWER_CAPPING_IDX_1,
    };

    const struct mod_scmi_power_capping_domain_config config = {
        .parent_idx = __LINE__,
        .power_capping_domain_id =
            FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_POWER_CAPPING, __LINE__),
#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
        .cap_pai_change_notification_support = true,
#endif
        .min_power_cap = MIN_DEFAULT_POWER_CAP,
        .max_power_cap = MAX_DEFAULT_POWER_CAP,
        .power_cap_step = 1u,
        .max_sustainable_power = 50u,
        .pai_config_support = true,
    };

    const struct mod_scmi_power_capping_domain_config *config_ptr = &config;

    uint32_t min_pai = 1u;
    uint32_t max_pai = 100u;
    uint32_t pai_step = 2u;

    struct scmi_power_capping_domain_attributes_p2a ret_payload = {
        .status = SCMI_SUCCESS,
        .attributes = 1u << POWER_CAP_CONF_SUP_POS |
            config.power_cap_unit << POWER_UNIT_POS |
            config.pai_config_support << PAI_CONF_SUP_POS,
        .name = "TestPowerCap",
        .min_pai = min_pai,
        .max_pai = max_pai,
        .pai_step = pai_step,
        .min_power_cap = config.min_power_cap,
        .max_power_cap = config.max_power_cap,
        .power_cap_step = config.power_cap_step,
        .max_sustainable_power = config.max_sustainable_power,
        .parent_id = config.parent_idx,
    };

#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
    ret_payload.attributes |= config.cap_pai_change_notification_support
        << POWER_CAPPING_NOTIF_SUP_POS;
    ret_payload.attributes |=
        config.power_measurements_change_notification_support
        << POWER_MEAS_NOTIF_SUP_POS;
#endif

    pcapping_core_get_pai_info_ExpectAndReturn(
        cmd_payload.domain_id, NULL, NULL, NULL, FWK_SUCCESS);
    pcapping_core_get_pai_info_IgnoreArg_min_pai();
    pcapping_core_get_pai_info_IgnoreArg_max_pai();
    pcapping_core_get_pai_info_IgnoreArg_pai_step();

    pcapping_core_get_pai_info_ReturnThruPtr_min_pai(&min_pai);
    pcapping_core_get_pai_info_ReturnThruPtr_max_pai(&max_pai);
    pcapping_core_get_pai_info_ReturnThruPtr_pai_step(&pai_step);

    pcapping_core_get_config_ExpectAndReturn(
        cmd_payload.domain_id, NULL, FWK_SUCCESS);
    pcapping_core_get_config_IgnoreArg_config();
    pcapping_core_get_config_ReturnThruPtr_config(&config_ptr);

    pcapping_core_get_cap_support_ExpectAndReturn(
        cmd_payload.domain_id, NULL, FWK_SUCCESS);
    pcapping_core_get_cap_support_IgnoreArg_support();
    bool cap_support = true;
    pcapping_core_get_cap_support_ReturnThruPtr_support(&cap_support);

    fwk_module_get_element_name_ExpectAndReturn(
        FWK_ID_ELEMENT(
            FWK_MODULE_IDX_SCMI_POWER_CAPPING, cmd_payload.domain_id),
        (char *)ret_payload.name);

    EXPECT_RESPONSE_SUCCESS(ret_payload);
    TEST_SCMI_COMMAND(MOD_SCMI_POWER_CAPPING_DOMAIN_ATTRIBUTES, cmd_payload);
}

void utest_message_handler_power_capping_get_valid(void)
{
    uint32_t cap = __LINE__; /* Arbitrary value */

    struct scmi_power_capping_cap_get_a2p cmd_payload = {
        .domain_id = FAKE_POWER_CAPPING_IDX_1
    };

    struct scmi_power_capping_cap_get_p2a ret_payload = {
        .status = SCMI_SUCCESS,
        .power_cap = cap,
    };

    pcapping_core_get_cap_ExpectWithArrayAndReturn(
        cmd_payload.domain_id, &cap, sizeof(cap), FWK_SUCCESS);
    pcapping_core_get_cap_IgnoreArg_cap();
    pcapping_core_get_cap_ReturnMemThruPtr_cap(&cap, sizeof(cap));

    EXPECT_RESPONSE_SUCCESS(ret_payload);
    TEST_SCMI_COMMAND(MOD_SCMI_POWER_CAPPING_CAP_GET, cmd_payload);
}

void utest_message_handler_power_capping_get_failure(void)
{
    uint32_t cap = __LINE__; /* Arbitrary value */

    struct scmi_power_capping_cap_get_a2p cmd_payload = {
        .domain_id = FAKE_POWER_CAPPING_IDX_1
    };

    struct scmi_power_capping_cap_get_p2a ret_payload = {
        .status = SCMI_GENERIC_ERROR,
    };

    pcapping_core_get_cap_ExpectWithArrayAndReturn(
        cmd_payload.domain_id, &cap, sizeof(cap), FWK_E_DEVICE);
    pcapping_core_get_cap_IgnoreArg_cap();

    EXPECT_RESPONSE_ERROR(ret_payload);
    TEST_SCMI_COMMAND(MOD_SCMI_POWER_CAPPING_CAP_GET, cmd_payload);
}

void utest_message_handler_power_capping_set_invalid_flags(void)
{
    struct scmi_power_capping_cap_set_a2p cmd_payload = {
        .domain_id = FAKE_POWER_CAPPING_IDX_1,
        .flags = ~(ASYNC_FLAG(1) | IGN_DEL_RESP_FLAG(1)),
    };

    struct scmi_power_capping_cap_set_p2a ret_payload = {
        .status = SCMI_INVALID_PARAMETERS,
    };

    EXPECT_RESPONSE_ERROR(ret_payload);
    TEST_SCMI_COMMAND(MOD_SCMI_POWER_CAPPING_CAP_SET, cmd_payload);
}

void utest_message_handler_power_capping_set_config_not_supported(void)
{
    uint32_t cap = 12U;

    struct scmi_power_capping_cap_set_a2p cmd_payload = {
        .domain_id = FAKE_POWER_CAPPING_IDX_1,
        .flags = ASYNC_FLAG(1) | IGN_DEL_RESP_FLAG(1),
        .power_cap = cap,
    };

    struct scmi_power_capping_cap_set_p2a ret_payload = {
        .status = SCMI_NOT_SUPPORTED,
    };

    pcapping_core_set_cap_ExpectAndReturn(
        service_id_1, cmd_payload.domain_id, true, cap, FWK_E_SUPPORT);

    EXPECT_RESPONSE_SUCCESS(ret_payload);
    TEST_SCMI_COMMAND(MOD_SCMI_POWER_CAPPING_CAP_SET, cmd_payload);
}

void utest_message_handler_power_capping_set_async_del_not_supported(void)
{
    struct scmi_power_capping_cap_set_a2p cmd_payload = {
        .domain_id = FAKE_POWER_CAPPING_IDX_2,
        .flags = (ASYNC_FLAG(1) | IGN_DEL_RESP_FLAG(0)),
    };

    struct scmi_power_capping_cap_set_p2a ret_payload = {
        .status = SCMI_NOT_SUPPORTED,
    };

    EXPECT_RESPONSE_SUCCESS(ret_payload);
    TEST_SCMI_COMMAND(MOD_SCMI_POWER_CAPPING_CAP_SET, cmd_payload);
}

void utest_message_handler_power_capping_set_domain_busy(void)
{
    uint32_t cap = MIN_DEFAULT_POWER_CAP;
    struct scmi_power_capping_cap_set_a2p cmd_payload = {
        .domain_id = FAKE_POWER_CAPPING_IDX_1,
        .power_cap = cap,
        .flags = ASYNC_FLAG(0),
    };

    struct scmi_power_capping_cap_set_p2a ret_payload = {
        .status = SCMI_BUSY,
    };

    pcapping_core_set_cap_ExpectAndReturn(
        service_id_1, cmd_payload.domain_id, false, cap, FWK_E_BUSY);

    EXPECT_RESPONSE_ERROR(ret_payload);
    TEST_SCMI_COMMAND(MOD_SCMI_POWER_CAPPING_CAP_SET, cmd_payload);
}

void utest_message_handler_power_capping_set_out_of_range(void)
{
    uint32_t cap = MIN_DEFAULT_POWER_CAP - 1u;
    struct scmi_power_capping_cap_set_a2p cmd_payload = {
        .domain_id = FAKE_POWER_CAPPING_IDX_1,
        .power_cap = cap,
        .flags = ASYNC_FLAG(0),
    };

    struct scmi_power_capping_cap_set_p2a ret_payload = {
        .status = SCMI_OUT_OF_RANGE,
    };

    pcapping_core_set_cap_ExpectAndReturn(
        service_id_1, cmd_payload.domain_id, false, cap, FWK_E_RANGE);

    EXPECT_RESPONSE_ERROR(ret_payload);
    TEST_SCMI_COMMAND(MOD_SCMI_POWER_CAPPING_CAP_SET, cmd_payload);
}

void utest_message_handler_power_capping_set_success_pending(void)
{
    uint32_t cap = MAX_DEFAULT_POWER_CAP;
    int status;

    struct scmi_power_capping_cap_set_a2p cmd_payload = {
        .domain_id = FAKE_POWER_CAPPING_IDX_1,
        .power_cap = cap,
        .flags = ASYNC_FLAG(0),
    };

    pcapping_core_set_cap_ExpectAndReturn(
        service_id_1, cmd_payload.domain_id, false, cap, FWK_PENDING);

    status = handler_table[MOD_SCMI_POWER_CAPPING_CAP_SET](
        service_id_1, (void *)&cmd_payload);

    TEST_ASSERT_EQUAL_UINT32(status, FWK_SUCCESS);
}

void utest_message_handler_power_capping_set_success_sync(void)
{
    uint32_t cap = MIN_DEFAULT_POWER_CAP;
    struct scmi_power_capping_cap_set_a2p cmd_payload = {
        .domain_id = FAKE_POWER_CAPPING_IDX_1,
        .power_cap = cap,
        .flags = ASYNC_FLAG(0),
    };

    struct scmi_power_capping_cap_set_p2a ret_payload = {
        .status = SCMI_SUCCESS,
    };

    pcapping_core_set_cap_ExpectAndReturn(
        service_id_1, cmd_payload.domain_id, false, cap, FWK_SUCCESS);

    EXPECT_RESPONSE_SUCCESS(ret_payload);
    TEST_SCMI_COMMAND(MOD_SCMI_POWER_CAPPING_CAP_SET, cmd_payload);
}

void utest_message_handler_power_capping_get_pai_valid(void)
{
    uint32_t pai = __LINE__; /* Arbitrary value */

    struct scmi_power_capping_pai_get_a2p cmd_payload = {
        .domain_id = FAKE_POWER_CAPPING_IDX_1
    };

    struct scmi_power_capping_pai_get_p2a ret_payload = {
        .status = SCMI_SUCCESS,
        .pai = pai,
    };

    pcapping_core_get_pai_ExpectAndReturn(
        cmd_payload.domain_id, NULL, FWK_SUCCESS);

    pcapping_core_get_pai_IgnoreArg_pai();

    pcapping_core_get_pai_ReturnThruPtr_pai(&pai);

    EXPECT_RESPONSE_SUCCESS(ret_payload);
    TEST_SCMI_COMMAND(MOD_SCMI_POWER_CAPPING_PAI_GET, cmd_payload);
}

void utest_message_handler_power_capping_get_pai_failure(void)
{
    struct scmi_power_capping_pai_get_a2p cmd_payload = {
        .domain_id = FAKE_POWER_CAPPING_IDX_1
    };

    struct scmi_power_capping_pai_get_p2a ret_payload = {
        .status = SCMI_GENERIC_ERROR,
    };

    pcapping_core_get_pai_ExpectAnyArgsAndReturn(FWK_E_STATE);

    EXPECT_RESPONSE_ERROR(ret_payload);
    TEST_SCMI_COMMAND(MOD_SCMI_POWER_CAPPING_PAI_GET, cmd_payload);
}

void utest_message_handler_power_capping_set_pai_valid(void)
{
    uint32_t pai = 2u;

    struct scmi_power_capping_pai_set_a2p cmd_payload = {
        .domain_id = FAKE_POWER_CAPPING_IDX_1,
        .pai = pai,
    };

    struct scmi_power_capping_pai_set_p2a ret_payload = {
        .status = SCMI_SUCCESS,
    };

    pcapping_core_set_pai_ExpectAndReturn(
        service_id_1, cmd_payload.domain_id, pai, FWK_SUCCESS);

    EXPECT_RESPONSE_SUCCESS(ret_payload);
    TEST_SCMI_COMMAND(MOD_SCMI_POWER_CAPPING_PAI_SET, cmd_payload);
}

void utest_message_handler_power_capping_set_pai_failure(void)
{
    uint32_t pai = 3u;

    struct scmi_power_capping_pai_set_a2p cmd_payload = {
        .domain_id = FAKE_POWER_CAPPING_IDX_1,
        .pai = pai,
    };

    struct scmi_power_capping_pai_set_p2a ret_payload = {
        .status = SCMI_GENERIC_ERROR,
    };

    pcapping_core_set_pai_ExpectAndReturn(
        service_id_1, cmd_payload.domain_id, pai, FWK_E_DEVICE);

    EXPECT_RESPONSE_ERROR(ret_payload);
    TEST_SCMI_COMMAND(MOD_SCMI_POWER_CAPPING_PAI_SET, cmd_payload);
}

void utest_message_handler_power_capping_set_pai_out_of_range(void)
{
    uint32_t pai = 3u;
    struct scmi_power_capping_pai_set_a2p cmd_payload = {
        .domain_id = FAKE_POWER_CAPPING_IDX_1,
        .pai = pai,
    };

    struct scmi_power_capping_cap_set_p2a ret_payload = {
        .status = SCMI_OUT_OF_RANGE,
    };

    pcapping_core_set_pai_ExpectAndReturn(
        service_id_1, cmd_payload.domain_id, pai, FWK_E_RANGE);

    EXPECT_RESPONSE_ERROR(ret_payload);
    TEST_SCMI_COMMAND(MOD_SCMI_POWER_CAPPING_PAI_SET, cmd_payload);
}

void utest_message_handler_power_capping_get_power_measurement_valid(void)
{
    uint32_t power = __LINE__; /* Arbitrary value */
    uint32_t pai = __LINE__; /* Arbitrary value */

    struct scmi_power_capping_measurements_get_a2p cmd_payload = {
        .domain_id = FAKE_POWER_CAPPING_IDX_1
    };

    struct scmi_power_capping_measurements_get_p2a ret_payload = {
        .status = SCMI_SUCCESS,
        .power = power,
        .pai = pai,
    };

    pcapping_core_get_power_ExpectAndReturn(
        cmd_payload.domain_id, NULL, FWK_SUCCESS);

    pcapping_core_get_power_IgnoreArg_power();

    pcapping_core_get_power_ReturnThruPtr_power(&power);

    pcapping_core_get_pai_ExpectAndReturn(
        cmd_payload.domain_id, NULL, FWK_SUCCESS);

    pcapping_core_get_pai_IgnoreArg_pai();

    pcapping_core_get_pai_ReturnThruPtr_pai(&pai);

    EXPECT_RESPONSE_SUCCESS(ret_payload);
    TEST_SCMI_COMMAND(MOD_SCMI_POWER_CAPPING_MEASUREMENTS_GET, cmd_payload);
}

void utest_message_handler_power_capping_get_power_measurement_failure_power(
    void)
{
    struct scmi_power_capping_measurements_get_a2p cmd_payload = {
        .domain_id = FAKE_POWER_CAPPING_IDX_1,
    };

    struct scmi_power_capping_measurements_get_p2a ret_payload = {
        .status = SCMI_GENERIC_ERROR,
    };

    pcapping_core_get_power_ExpectAnyArgsAndReturn(FWK_E_DEVICE);

    EXPECT_RESPONSE_ERROR(ret_payload);
    TEST_SCMI_COMMAND(MOD_SCMI_POWER_CAPPING_MEASUREMENTS_GET, cmd_payload);
}

void utest_message_handler_power_capping_get_power_measurement_failure_pai(void)
{
    struct scmi_power_capping_measurements_get_a2p cmd_payload = {
        .domain_id = FAKE_POWER_CAPPING_IDX_1,
    };

    struct scmi_power_capping_measurements_get_p2a ret_payload = {
        .status = SCMI_GENERIC_ERROR,
    };

    pcapping_core_get_power_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    pcapping_core_get_pai_ExpectAnyArgsAndReturn(FWK_E_DEVICE);

    EXPECT_RESPONSE_ERROR(ret_payload);
    TEST_SCMI_COMMAND(MOD_SCMI_POWER_CAPPING_MEASUREMENTS_GET, cmd_payload);
}

#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
void utest_message_handler_power_capping_cap_notify_valid_enable(void)
{
    struct scmi_power_capping_cap_notify_a2p cmd_payload = {
        .domain_id = FAKE_POWER_CAPPING_IDX_1,
        .notify_enable = POWER_CAP_NOTIFY_ENABLE,
    };

    struct scmi_power_capping_cap_notify_p2a ret_payload = {
        .status = SCMI_SUCCESS,
    };

    scmi_notification_add_subscriber_ExpectAndReturn(
        MOD_SCMI_PROTOCOL_ID_POWER_CAPPING,
        FAKE_POWER_CAPPING_IDX_1,
        MOD_SCMI_POWER_CAPPING_CAP_NOTIFY,
        service_id_1,
        FWK_SUCCESS);

    EXPECT_RESPONSE_SUCCESS(ret_payload);
    TEST_SCMI_COMMAND(MOD_SCMI_POWER_CAPPING_CAP_NOTIFY, cmd_payload);
}

void utest_message_handler_power_capping_cap_notify_valid_disable(void)
{
    unsigned int agent_id;

    struct scmi_power_capping_cap_notify_a2p cmd_payload = {
        .domain_id = FAKE_POWER_CAPPING_IDX_1,
        .notify_enable = POWER_CAP_NOTIFY_DISABLE,
    };

    struct scmi_power_capping_cap_notify_p2a ret_payload = {
        .status = SCMI_SUCCESS,
    };

    get_agent_id_ExpectAndReturn(service_id_1, NULL, FWK_SUCCESS);
    get_agent_id_IgnoreArg_agent_id();
    get_agent_id_ReturnMemThruPtr_agent_id(&agent_id, sizeof(agent_id));

    scmi_notification_remove_subscriber_ExpectAndReturn(
        MOD_SCMI_PROTOCOL_ID_POWER_CAPPING,
        agent_id,
        cmd_payload.domain_id,
        MOD_SCMI_POWER_CAPPING_CAP_NOTIFY,
        FWK_SUCCESS);

    EXPECT_RESPONSE_SUCCESS(ret_payload);
    TEST_SCMI_COMMAND(MOD_SCMI_POWER_CAPPING_CAP_NOTIFY, cmd_payload);
}

void utest_message_handler_power_capping_measurements_notify_valid_enable(void)
{
    uint32_t threshold_low = MIN_DEFAULT_POWER_THRESH;
    uint32_t threshold_high = MAX_DEFAULT_POWER_THRESH;

    struct scmi_power_capping_measurements_notify_a2p cmd_payload = {
        .domain_id = FAKE_POWER_CAPPING_IDX_1,
        .notify_enable = MEASUREMENTS_NOTIFY_ENABLE,
        .threshold_low = threshold_low,
        .threshold_high = threshold_high,
    };

    struct scmi_power_capping_measurements_notify_p2a ret_payload = {
        .status = SCMI_SUCCESS,
    };

    scmi_notification_add_subscriber_ExpectAndReturn(
        MOD_SCMI_PROTOCOL_ID_POWER_CAPPING,
        FAKE_POWER_CAPPING_IDX_1,
        MOD_SCMI_POWER_CAPPING_MEASUREMENTS_NOTIFY,
        service_id_1,
        FWK_SUCCESS);

    pcapping_core_set_power_thresholds_ExpectAndReturn(
        cmd_payload.domain_id, threshold_low, threshold_high, FWK_SUCCESS);

    EXPECT_RESPONSE_SUCCESS(ret_payload);
    TEST_SCMI_COMMAND(MOD_SCMI_POWER_CAPPING_MEASUREMENTS_NOTIFY, cmd_payload);
}

void utest_message_handler_power_capping_measurements_notify_valid_disable(void)
{
    unsigned int agent_id;

    struct scmi_power_capping_measurements_notify_a2p cmd_payload = {
        .domain_id = FAKE_POWER_CAPPING_IDX_1,
        .notify_enable = MEASUREMENTS_NOTIFY_DISABLE,
    };

    struct scmi_power_capping_measurements_notify_p2a ret_payload = {
        .status = SCMI_SUCCESS,
    };

    get_agent_id_ExpectAndReturn(service_id_1, NULL, FWK_SUCCESS);
    get_agent_id_IgnoreArg_agent_id();
    get_agent_id_ReturnMemThruPtr_agent_id(&agent_id, sizeof(agent_id));

    scmi_notification_remove_subscriber_ExpectAndReturn(
        MOD_SCMI_PROTOCOL_ID_POWER_CAPPING,
        agent_id,
        cmd_payload.domain_id,
        MOD_SCMI_POWER_CAPPING_MEASUREMENTS_NOTIFY,
        FWK_SUCCESS);

    EXPECT_RESPONSE_SUCCESS(ret_payload);
    TEST_SCMI_COMMAND(MOD_SCMI_POWER_CAPPING_MEASUREMENTS_NOTIFY, cmd_payload);
}

void utest_pcapping_protocol_process_cap_pai_notify_event_success(void)
{
    unsigned int agent_id = __LINE__;
    uint32_t cap = __LINE__;
    uint32_t pai = __LINE__;
    int status = FWK_SUCCESS;

    struct pcapping_core_cap_pai_event_parameters event_params = {
        .domain_idx = FAKE_POWER_CAPPING_IDX_1,
        .service_id = service_id_1,
        .cap = cap,
        .pai = pai,
    };

    struct scmi_power_capping_cap_changed_p2a payload = {
        .agent_id = agent_id,
        .domain_id = FAKE_POWER_CAPPING_IDX_1,
        .cap = cap,
        .pai = pai,
    };

    struct fwk_event cap_pai_notify_event;

    struct pcapping_core_cap_pai_event_parameters *event_params_ptr =
        (struct pcapping_core_cap_pai_event_parameters *)
            cap_pai_notify_event.params;

    *event_params_ptr = event_params;

    pcapping_core_is_cap_request_async_ExpectAndReturn(
        event_params.domain_idx, true);
    fwk_id_is_equal_ExpectAndReturn(service_id_1, FWK_ID_NONE, false);
    get_agent_id_ExpectAndReturn(service_id_1, NULL, FWK_SUCCESS);
    get_agent_id_IgnoreArg_agent_id();
    get_agent_id_ReturnMemThruPtr_agent_id(&agent_id, sizeof(agent_id));

    scmi_notification_notify_ExpectWithArrayAndReturn(
        MOD_SCMI_PROTOCOL_ID_POWER_CAPPING,
        MOD_SCMI_POWER_CAPPING_CAP_NOTIFY,
        payload.domain_id,
        SCMI_POWER_CAPPING_CAP_CHANGED,
        &payload,
        sizeof(payload),
        sizeof(payload),
        FWK_SUCCESS);

    status =
        pcapping_protocol_process_cap_pai_notify_event(&cap_pai_notify_event);

    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
}

void utest_pcapping_protocol_process_measurements_notify_event_success(void)
{
    uint32_t power = __LINE__;
    int status = FWK_SUCCESS;
    struct fwk_event measurements_notify_event;

    struct pcapping_core_pwr_meas_event_parameters event_params = {
        .domain_idx = FAKE_POWER_CAPPING_IDX_1,
        .service_id = service_id_1,
        .power = power,
    };

    struct scmi_power_capping_measurements_changed_p2a payload = {
        .agent_id = SCMI_POWER_CAPPING_AGENT_ID_PLATFORM,
        .domain_id = FAKE_POWER_CAPPING_IDX_1,
        .power = power,
    };

    struct pcapping_core_pwr_meas_event_parameters *event_params_ptr =
        (struct pcapping_core_pwr_meas_event_parameters *)
            measurements_notify_event.params;

    *event_params_ptr = event_params;

    scmi_notification_notify_ExpectWithArrayAndReturn(
        MOD_SCMI_PROTOCOL_ID_POWER_CAPPING,
        MOD_SCMI_POWER_CAPPING_MEASUREMENTS_NOTIFY,
        payload.domain_id,
        SCMI_POWER_CAPPING_MEASUREMENTS_CHANGED,
        &payload,
        sizeof(payload),
        sizeof(payload),
        FWK_SUCCESS);

    status = pcapping_protocol_process_measurements_notify_event(
        &measurements_notify_event);

    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
}
#endif

#ifdef BUILD_HAS_MOD_RESOURCE_PERMS
void utest_message_handler_invalid_resource_permissions(void)
{
    uint32_t message_id;
    /*
     * As the domain id is the first element of the payload and it is the only
     * necessary data in this test , we will use a u32t to represent the
     * payload and the domain id.
     */
    uint32_t domain_id = FAKE_POWER_CAPPING_IDX_COUNT;
    uint32_t agent_id = __LINE__;

    for (message_id = MOD_SCMI_PROTOCOL_VERSION;
         message_id <= MOD_SCMI_POWER_CAPPING_CAP_SET;
         message_id++) {
        get_agent_id_ExpectAndReturn(service_id_1, NULL, FWK_SUCCESS);
        get_agent_id_IgnoreArg_agent_id();
        get_agent_id_ReturnThruPtr_agent_id(&agent_id);

        agent_has_resource_permission_ExpectAndReturn(
            agent_id,
            MOD_SCMI_PROTOCOL_ID_POWER_CAPPING,
            message_id,
            domain_id,
            MOD_RES_PERMS_ACCESS_DENIED);

        status = scmi_power_capping_permissions_handler(
            message_id, service_id_1, &domain_id);
        TEST_ASSERT_EQUAL(status, FWK_E_ACCESS);
    }
}
#endif

void utest_pcapping_protocol_bind_scmi_failure(void)
{
    int status;

    fwk_module_bind_ExpectAndReturn(
        FWK_ID_MODULE(FWK_MODULE_IDX_SCMI),
        FWK_ID_API(FWK_MODULE_IDX_SCMI, MOD_SCMI_API_IDX_PROTOCOL),
        &(pcapping_protocol_ctx.scmi_api),
        FWK_E_DEVICE);

    status = pcapping_protocol_bind();
    TEST_ASSERT_EQUAL(status, FWK_E_DEVICE);
}

void utest_pcapping_protocol_bind(void)
{
    int status;

    fwk_module_bind_ExpectAndReturn(
        FWK_ID_MODULE(FWK_MODULE_IDX_SCMI),
        FWK_ID_API(FWK_MODULE_IDX_SCMI, MOD_SCMI_API_IDX_PROTOCOL),
        &(pcapping_protocol_ctx.scmi_api),
        FWK_SUCCESS);

#ifdef BUILD_HAS_MOD_RESOURCE_PERMS
    fwk_module_bind_ExpectAndReturn(
        FWK_ID_MODULE(FWK_MODULE_IDX_RESOURCE_PERMS),
        FWK_ID_API(FWK_MODULE_IDX_RESOURCE_PERMS, MOD_RES_PERM_RESOURCE_PERMS),
        &(pcapping_protocol_ctx.res_perms_api),
        FWK_SUCCESS);
#endif

#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
    fwk_module_bind_ExpectAndReturn(
        FWK_ID_MODULE(FWK_MODULE_IDX_SCMI),
        FWK_ID_API(FWK_MODULE_IDX_SCMI, MOD_SCMI_API_IDX_NOTIFICATION),
        &(pcapping_protocol_ctx.scmi_notification_api),
        FWK_SUCCESS);
#endif
    status = pcapping_protocol_bind();
    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
}

#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
void utest_pcapping_protocol_start(void)
{
    int status;
    unsigned int agent_count = __LINE__;
    unsigned int domain_count = 4U;

    get_agent_count_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    get_agent_count_ReturnMemThruPtr_agent_count(
        &agent_count, sizeof(agent_count));

    pcapping_core_get_domain_count_ExpectAndReturn(domain_count);

    scmi_notification_init_ExpectAndReturn(
        MOD_SCMI_PROTOCOL_ID_POWER_CAPPING,
        agent_count,
        domain_count,
        MOD_SCMI_POWER_CAPPING_NOTIFICATION_COUNT,
        FWK_SUCCESS);

    status = pcapping_protocol_start();
    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
}
#endif

void utest_pcapping_protocol_process_bind_request_success(void)
{
    int status;
    const void *api;
    fwk_id_t api_id = FWK_ID_API(
        FWK_MODULE_IDX_SCMI_POWER_CAPPING,
        MOD_SCMI_POWER_CAPPING_API_IDX_REQUEST);

    fwk_id_is_equal_ExpectAndReturn(api_id, api_id, true);

    status = pcapping_protocol_process_bind_request(api_id, &api);
    TEST_ASSERT_EQUAL_PTR(&scmi_power_capping_mod_scmi_to_protocol_api, api);
    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
}

void utest_pcapping_protocol_process_bind_request_failure(void)
{
    int status;
    const void *api;
    fwk_id_t api_id_invalid;
    fwk_id_t api_id_valid = FWK_ID_API(
        FWK_MODULE_IDX_SCMI_POWER_CAPPING,
        MOD_SCMI_POWER_CAPPING_API_IDX_REQUEST);

    fwk_id_is_equal_ExpectAndReturn(api_id_invalid, api_id_valid, false);

    status = pcapping_protocol_process_bind_request(api_id_invalid, &api);
    TEST_ASSERT_EQUAL(status, FWK_E_SUPPORT);
}

int scmi_power_capping_protocol_test_main(void)
{
    test_init();
    RUN_TEST(utest_message_handler_scmi_validation_respond_error);
    RUN_TEST(utest_message_handler_scmi_validation_respond_success);
    RUN_TEST(utest_get_scmi_protocol_id);
    RUN_TEST(utest_message_handler_protocol_version);
    RUN_TEST(utest_message_handler_protocol_attributes);
    RUN_TEST(utest_message_handler_protocol_msg_attributes_supported_msgs);
    RUN_TEST(utest_message_handler_domain_attributes_valid);
    RUN_TEST(utest_message_handler_power_capping_get_valid);
    RUN_TEST(utest_message_handler_power_capping_get_failure);
    RUN_TEST(utest_message_handler_power_capping_set_invalid_flags);
    RUN_TEST(utest_message_handler_power_capping_set_config_not_supported);
    RUN_TEST(utest_message_handler_power_capping_set_async_del_not_supported);
    RUN_TEST(utest_message_handler_power_capping_set_domain_busy);
    RUN_TEST(utest_message_handler_power_capping_set_out_of_range);
    RUN_TEST(utest_message_handler_power_capping_set_success_pending);
    RUN_TEST(utest_message_handler_power_capping_set_success_sync);
    RUN_TEST(utest_message_handler_power_capping_get_pai_valid);
    RUN_TEST(utest_message_handler_power_capping_get_pai_failure);
    RUN_TEST(utest_message_handler_power_capping_set_pai_valid);
    RUN_TEST(utest_message_handler_power_capping_set_pai_failure);
    RUN_TEST(utest_message_handler_power_capping_set_pai_out_of_range);
    RUN_TEST(utest_message_handler_power_capping_get_power_measurement_valid);
    RUN_TEST(
        utest_message_handler_power_capping_get_power_measurement_failure_power);
    RUN_TEST(
        utest_message_handler_power_capping_get_power_measurement_failure_pai);
#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
    RUN_TEST(utest_message_handler_power_capping_cap_notify_valid_enable);
    RUN_TEST(utest_message_handler_power_capping_cap_notify_valid_disable);
    RUN_TEST(
        utest_message_handler_power_capping_measurements_notify_valid_enable);
    RUN_TEST(
        utest_message_handler_power_capping_measurements_notify_valid_disable);
    RUN_TEST(utest_pcapping_protocol_process_cap_pai_notify_event_success);
    RUN_TEST(utest_pcapping_protocol_process_measurements_notify_event_success);
    RUN_TEST(utest_pcapping_protocol_start);
#endif
#ifdef BUILD_HAS_MOD_RESOURCE_PERMS
    RUN_TEST(utest_message_handler_invalid_resource_permissions);
#endif
    RUN_TEST(utest_pcapping_protocol_bind_scmi_failure);
    RUN_TEST(utest_pcapping_protocol_bind);
    RUN_TEST(utest_pcapping_protocol_process_bind_request_success);
    RUN_TEST(utest_pcapping_protocol_process_bind_request_failure);
    return UNITY_END();
}

#if !defined(TEST_ON_TARGET)
int main(void)
{
    return scmi_power_capping_protocol_test_main();
}
#endif
