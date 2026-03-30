/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "scp_unity.h"
#include "string.h"
#include "unity.h"

#include <Mockfwk_id.h>
#include <Mockmod_pinctrl_extra.h>
#include <Mockmod_scmi_extra.h>
#include <internal/Mockfwk_core_internal.h>
#include <scmi_pin_control.h>

#include <mod_scmi_pin_control.h>

#include UNIT_TEST_SRC

#define SCMI_MODULE_ID 0x5

#define GET_LOW_BYTES_OF_32_BIT_NUMBER(X)  (X & 0xFFFF)
#define GET_HIGH_BYTES_OF_32_BIT_NUMBER(X) (X & 0xFFFF0000)
#define BIT_CHECK(X, BIT_POS)              ((X & (1 << BIT_POS)) >> BIT_POS)

#define SHIFT_LEFT_BY_POS(X, POS) (X << POS)

#define GPIO_FUNCTION_DESCRIPTOR_POSITION     17
#define PIN_ONLY_FUNCTION_DESCRIPTOR_POSITION 16
#define EXTENDED_NAME_POSITION                31

#define SETTING_GET_CONFIG_FLAG_POS 18
#define SETTING_GET_SELECTOR_POS    16
#define SETTING_GET_SKIP_CONFIG_POS 8
#define SETTING_GET_CONFIG_TYPE_POS 0

#define SETTING_FUNC_POS             10
#define SETTING_NUMBER_OF_CONFIG_POS 2
#define SETTING_FLAG_POS             0

#define EXPECTED_NUMBER_OF_PINS         8
#define EXPECTED_NUMBER_OF_GROUPS       4
#define EXPECTED_NUMBER_OF_FUNC         6
#define PROTOCOL_ATTRIBUTE_PAYLOAD_SIZE 12

#define TOTAL_NUMBER_OF_ASSOCIATED_PINS 5U
#define MAX_ALLOWED_RESPOND_BUFFER_SIZE 11U
#define EXPECTED_NUMBER_OF_IDENTIFIERS  1U

#define PINCTRL_SETTING_CONFIG_SIZE_OF_PAYLOAD \
    ((sizeof(struct scmi_pin_control_settings_configure_a2p) + \
      sizeof(struct mod_pinctrl_drv_pin_configuration) * 2) / \
     sizeof(uint32_t))

enum services {
    SERVICE_IDX,
    SERVICE_IDX_COUNT,
};

static fwk_id_t service_id = FWK_ID_ELEMENT_INIT(SCMI_MODULE_ID, SERVICE_IDX);

static fwk_id_t protocol_id = FWK_ID_MODULE(MOD_SCMI_PROTOCOL_ID_PIN_CONTROL);

static const struct config_domain domains = {
    .identifier_begin = SCMI_PIN_CONTROL_MIN_IDENTIFIER,
    .identifier_end = SCMI_PIN_CONTROL_MAX_IDENTIFIER,
    .shift_factor = 0,
};

static const struct mod_scmi_pinctrl_domain_config domains_table = {
    &domains,
    .config_domain_count = 1,
};

static const struct mod_scmi_from_protocol_api scmi_api = {
    .respond = respond,
    .scmi_message_validation = mod_scmi_from_protocol_api_scmi_frame_validation,
    .write_payload = scmi_write_payload,
    .get_max_payload_size = get_max_payload_size,
};

static const struct mod_pinctrl_api pinctrl_api = {
    .get_attributes = get_attributes,
    .get_info = get_info,
    .get_list_associations = get_list_associations,
    .get_total_number_of_associations = get_total_number_of_associations,
    .get_configuration_value_from_type = get_configuration_value_from_type,
    .get_total_number_of_configurations = get_total_number_of_configurations,
    .get_configuration = get_configuration,
    .get_current_associated_function = get_current_associated_function,
    .set_configuration = set_configuration,
    .set_function = set_function,
};

void setUp(void)
{
    scmi_pin_control_ctx.scmi_api = &scmi_api;
    scmi_pin_control_ctx.pinctrl_api = &pinctrl_api;
    scmi_pin_control_ctx.config_domain_table = &domains_table;
}

void tearDown(void)
{
    Mockmod_scmi_extra_Verify();
    Mockmod_scmi_extra_Destroy();
    Mockmod_pinctrl_extra_Verify();
    Mockmod_pinctrl_extra_Destroy();
}

void utest_protocol_version(void)
{
    int status = SCMI_GENERIC_ERROR;
    struct scmi_protocol_version_p2a ret_payload = {
        .status = SCMI_SUCCESS,
        .version = SCMI_PROTOCOL_VERSION_PIN_CONTROL,
    };

    mod_scmi_from_protocol_api_scmi_frame_validation_ExpectAnyArgsAndReturn(
        SCMI_SUCCESS);

    respond_ExpectWithArrayAndReturn(
        service_id,
        (void *)&ret_payload,
        sizeof(ret_payload),
        sizeof(ret_payload),
        FWK_SUCCESS);

    status = scmi_pin_control_message_handler(
        protocol_id,
        service_id,
        (void *)&protocol_id,
        0,
        MOD_SCMI_PROTOCOL_VERSION);
    TEST_ASSERT_EQUAL(SCMI_SUCCESS, status);
}

void utest_protocol_message_attributes(void)
{
    int status = SCMI_GENERIC_ERROR;
    struct scmi_protocol_message_attributes_a2p payload = {
        .message_id = MOD_SCMI_PIN_CONTROL_LIST_ASSOCIATIONS,
    };

    struct scmi_protocol_message_attributes_p2a return_values = {
        .status = (int32_t)SCMI_SUCCESS,
        .attributes = 0,
    };

    mod_scmi_from_protocol_api_scmi_frame_validation_ExpectAnyArgsAndReturn(
        SCMI_SUCCESS);

    respond_ExpectWithArrayAndReturn(
        service_id,
        (void *)&return_values,
        sizeof(return_values),
        sizeof(return_values),
        FWK_SUCCESS);

    status = scmi_pin_control_message_handler(
        protocol_id,
        service_id,
        (void *)&payload,
        0,
        MOD_SCMI_PROTOCOL_MESSAGE_ATTRIBUTES);

    TEST_ASSERT_EQUAL(SCMI_SUCCESS, status);
}

void utest_protocol_message_attributes_message_id_out_of_range(void)
{
    int status = SCMI_GENERIC_ERROR;
    struct scmi_protocol_message_attributes_a2p payload = {
        .message_id = MOD_SCMI_PIN_CONTROL_COMMAND_COUNT,
    };

    struct scmi_protocol_message_attributes_p2a return_values = {
        .status = (int32_t)SCMI_NOT_FOUND,
        .attributes = 0,
    };

    mod_scmi_from_protocol_api_scmi_frame_validation_ExpectAnyArgsAndReturn(
        SCMI_SUCCESS);

    respond_ExpectWithArrayAndReturn(
        service_id,
        (void *)&return_values,
        sizeof(return_values.status),
        sizeof(return_values.status),
        FWK_E_RANGE);

    status = scmi_pin_control_message_handler(
        protocol_id,
        service_id,
        (void *)&payload,
        0,
        MOD_SCMI_PROTOCOL_MESSAGE_ATTRIBUTES);

    TEST_ASSERT_EQUAL(FWK_E_RANGE, status);
}

int protocol_attributes_respond_callback(
    fwk_id_t service_id,
    const void *payload,
    size_t size,
    int cmock_num_calls)
{
    const int payload_size = PROTOCOL_ATTRIBUTE_PAYLOAD_SIZE;
    const int number_of_pins = EXPECTED_NUMBER_OF_PINS;
    const int number_of_groups =
        SHIFT_LEFT_BY_POS(EXPECTED_NUMBER_OF_GROUPS, NUM_OF_PIN_GROUPS_POS);
    const int number_of_functions = EXPECTED_NUMBER_OF_FUNC;

    struct scmi_pin_control_protocol_attributes_p2a *return_values =
        (struct scmi_pin_control_protocol_attributes_p2a *)payload;

    TEST_ASSERT_EQUAL(payload_size, size);
    TEST_ASSERT_EQUAL(
        number_of_pins,
        GET_LOW_BYTES_OF_32_BIT_NUMBER(return_values->attributes_low));
    TEST_ASSERT_EQUAL(
        number_of_groups,
        GET_HIGH_BYTES_OF_32_BIT_NUMBER(return_values->attributes_low));
    TEST_ASSERT_EQUAL(
        number_of_functions,
        GET_LOW_BYTES_OF_32_BIT_NUMBER(return_values->attributes_high));
    TEST_ASSERT_EQUAL(SCMI_SUCCESS, return_values->status);

    return FWK_SUCCESS;
}

void utest_protocol_attributes(void)
{
    int status = SCMI_GENERIC_ERROR;
    struct mod_pinctrl_info expected_protocol_attributes = {
        .number_of_pins = EXPECTED_NUMBER_OF_PINS,
        .number_of_groups = EXPECTED_NUMBER_OF_GROUPS,
        .number_of_functions = EXPECTED_NUMBER_OF_FUNC,
    };

    mod_scmi_from_protocol_api_scmi_frame_validation_ExpectAnyArgsAndReturn(
        SCMI_SUCCESS);

    get_info_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    get_info_ReturnThruPtr_info(&expected_protocol_attributes);

    respond_StubWithCallback(protocol_attributes_respond_callback);

    status = scmi_pin_control_message_handler(
        protocol_id,
        service_id,
        (void *)&protocol_id,
        0,
        MOD_SCMI_PROTOCOL_ATTRIBUTES);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

int attributes_respond_callback_group_check(
    fwk_id_t service_id,
    const void *payload,
    size_t size,
    int cmock_num_calls)
{
    const char expected_name[] = "grp_i2c_a";
    const uint16_t expected_number_of_elements = 2;

    struct scmi_pin_control_attributes_p2a *return_values =
        (struct scmi_pin_control_attributes_p2a *)payload;

    TEST_ASSERT_EQUAL_STRING(expected_name, return_values->name);
    TEST_ASSERT_EQUAL(expected_number_of_elements, return_values->attributes);
    TEST_ASSERT_EQUAL(SCMI_SUCCESS, return_values->status);

    return FWK_SUCCESS;
}

void utest_attributes_group(void)
{
    int status = SCMI_GENERIC_ERROR;
    char name[] = "grp_i2c_a";

    struct mod_pinctrl_attributes pinctrl_attributes = {
        .number_of_elements = 2,
        .is_gpio_function = false,
        .is_pin_only_function = false,
        .name = name,
    };

    struct scmi_pin_control_attributes_a2p payload = {
        /* Any random value can be chosen */
        .identifier = 3,
        .flags = MOD_PINCTRL_SELECTOR_GROUP,
    };

    mod_scmi_from_protocol_api_scmi_frame_validation_ExpectAnyArgsAndReturn(
        SCMI_SUCCESS);

    get_attributes_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    get_attributes_ReturnThruPtr_attributes(&pinctrl_attributes);

    respond_StubWithCallback(attributes_respond_callback_group_check);

    status = scmi_pin_control_message_handler(
        protocol_id,
        service_id,
        (void *)&payload,
        sizeof(payload),
        MOD_SCMI_PIN_CONTROL_ATTRIBUTES);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

int attributes_respond_callback_function_check(
    fwk_id_t service_id,
    const void *payload,
    size_t size,
    int cmock_num_calls)
{
    const char expected_name[] = "function_uart_a";
    const uint16_t expected_number_of_elements = 2;

    struct scmi_pin_control_attributes_p2a *return_values =
        (struct scmi_pin_control_attributes_p2a *)payload;

    TEST_ASSERT_EQUAL_STRING(expected_name, return_values->name);
    TEST_ASSERT_EQUAL(
        expected_number_of_elements,
        GET_LOW_BYTES_OF_32_BIT_NUMBER(return_values->attributes));
    TEST_ASSERT_EQUAL(
        true,
        BIT_CHECK(
            return_values->attributes, GPIO_FUNCTION_DESCRIPTOR_POSITION));
    TEST_ASSERT_EQUAL(
        true,
        BIT_CHECK(
            return_values->attributes, PIN_ONLY_FUNCTION_DESCRIPTOR_POSITION));
    TEST_ASSERT_EQUAL(
        true, BIT_CHECK(return_values->attributes, EXTENDED_NAME_POSITION));
    TEST_ASSERT_EQUAL(SCMI_SUCCESS, return_values->status);

    return FWK_SUCCESS;
}

void utest_attributes_function(void)
{
    int status = SCMI_GENERIC_ERROR;
    char name[] = "function_uart_abcd";

    struct mod_pinctrl_attributes pinctrl_attributes = {
        .number_of_elements = 2,
        .is_gpio_function = true,
        .is_pin_only_function = true,
        .name = name,
    };

    struct scmi_pin_control_attributes_a2p payload = {
        /* Any random value can be chosen */
        .identifier = 3,
        .flags = MOD_PINCTRL_SELECTOR_FUNCTION,
    };

    mod_scmi_from_protocol_api_scmi_frame_validation_ExpectAnyArgsAndReturn(
        SCMI_SUCCESS);

    get_attributes_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    get_attributes_ReturnThruPtr_attributes(&pinctrl_attributes);

    respond_StubWithCallback(attributes_respond_callback_function_check);

    status = scmi_pin_control_message_handler(
        protocol_id,
        service_id,
        (void *)&payload,
        sizeof(payload),
        MOD_SCMI_PIN_CONTROL_ATTRIBUTES);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

int attributes_respond_callback_pin_check(
    fwk_id_t service_id,
    const void *payload,
    size_t size,
    int cmock_num_calls)
{
    const char expected_name[] = "0123456789ABCDE";
    const uint16_t expected_number_of_elements = 1;

    struct scmi_pin_control_attributes_p2a *return_values =
        (struct scmi_pin_control_attributes_p2a *)payload;

    TEST_ASSERT_EQUAL_STRING(expected_name, return_values->name);
    TEST_ASSERT_EQUAL(
        expected_number_of_elements,
        GET_LOW_BYTES_OF_32_BIT_NUMBER(return_values->attributes));
    TEST_ASSERT_EQUAL(
        false,
        BIT_CHECK(
            return_values->attributes, GPIO_FUNCTION_DESCRIPTOR_POSITION));
    TEST_ASSERT_EQUAL(
        false,
        BIT_CHECK(
            return_values->attributes, PIN_ONLY_FUNCTION_DESCRIPTOR_POSITION));
    TEST_ASSERT_EQUAL(
        true, BIT_CHECK(return_values->attributes, EXTENDED_NAME_POSITION));
    TEST_ASSERT_EQUAL(SCMI_SUCCESS, return_values->status);

    return FWK_SUCCESS;
}

void utest_attributes_pin(void)
{
    int status = SCMI_GENERIC_ERROR;
    char name[] = "0123456789ABCDEFGH";

    struct mod_pinctrl_attributes pinctrl_attributes = {
        .number_of_elements = 1,
        .is_gpio_function = false,
        .is_pin_only_function = false,
        .name = name,
    };

    struct scmi_pin_control_attributes_a2p payload = {
        /* Any random value can be chosen */
        .identifier = 3,
        .flags = MOD_PINCTRL_SELECTOR_PIN,
    };

    mod_scmi_from_protocol_api_scmi_frame_validation_ExpectAnyArgsAndReturn(
        SCMI_SUCCESS);

    get_attributes_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    get_attributes_ReturnThruPtr_attributes(&pinctrl_attributes);

    respond_StubWithCallback(attributes_respond_callback_pin_check);

    status = scmi_pin_control_message_handler(
        protocol_id,
        service_id,
        (void *)&payload,
        sizeof(payload),
        MOD_SCMI_PIN_CONTROL_ATTRIBUTES);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

int attributes_respond_callback_id_out_of_range(
    fwk_id_t service_id,
    const void *payload,
    size_t size,
    int cmock_num_calls)
{
    struct scmi_pin_control_attributes_p2a *return_values =
        (struct scmi_pin_control_attributes_p2a *)payload;

    TEST_ASSERT_EQUAL(SCMI_NOT_FOUND, return_values->status);
    return FWK_E_RANGE;
}

void utest_attributes_id_out_of_range(void)
{
    int status = SCMI_GENERIC_ERROR;
    char name[] = "0123456789ABCDEFGH";

    struct mod_pinctrl_attributes pinctrl_attributes = {
        .number_of_elements = 1,
        .is_gpio_function = false,
        .is_pin_only_function = false,
        .name = name,
    };

    struct scmi_pin_control_attributes_a2p payload = {
        /* Any random value can be chosen */
        .identifier = 7,
        .flags = MOD_PINCTRL_SELECTOR_PIN,
    };

    mod_scmi_from_protocol_api_scmi_frame_validation_ExpectAnyArgsAndReturn(
        SCMI_SUCCESS);

    get_attributes_ExpectAndReturn(
        payload.identifier, payload.flags, &pinctrl_attributes, FWK_E_RANGE);
    get_attributes_IgnoreArg_attributes();
    get_attributes_ReturnThruPtr_attributes(&pinctrl_attributes);

    respond_StubWithCallback(attributes_respond_callback_id_out_of_range);

    status = scmi_pin_control_message_handler(
        protocol_id,
        service_id,
        (void *)&payload,
        sizeof(payload),
        MOD_SCMI_PIN_CONTROL_ATTRIBUTES);

    TEST_ASSERT_EQUAL(FWK_E_RANGE, status);
}

int list_association_pins_in_groups_respond_callback(
    fwk_id_t service_id,
    const void *payload,
    size_t size,
    int cmock_num_calls)
{
    const uint32_t expected_payload_size =
        sizeof(struct scmi_pin_control_list_associations_p2a) +
        EXPECTED_NUMBER_OF_IDENTIFIERS * sizeof(uint16_t);

    TEST_ASSERT_EQUAL(NULL, payload);
    TEST_ASSERT_EQUAL(expected_payload_size, size);

    return FWK_SUCCESS;
}

void utest_list_association_with_valid_identifier(void)
{
    int status = SCMI_GENERIC_ERROR;
    uint32_t payload_size =
        (uint32_t)sizeof(struct scmi_pin_control_list_associations_p2a);
    struct scmi_pin_control_list_associations_a2p payload = {
        /* Any random value can be chosen */
        .identifier = 3,
        .flags = MOD_PINCTRL_SELECTOR_GROUP,
        .index = 0,
    };

    const uint32_t expected_return_flags =
        SHIFT_LEFT_BY_POS(
            (TOTAL_NUMBER_OF_ASSOCIATED_PINS - EXPECTED_NUMBER_OF_IDENTIFIERS),
            SCMI_PIN_CONTROL_REMAINING_CONFIGS_POS) |
        SHIFT_LEFT_BY_POS(EXPECTED_NUMBER_OF_IDENTIFIERS, 0);

    struct scmi_pin_control_list_associations_p2a expected_return_value = {
        .status = FWK_SUCCESS,
        .flags = expected_return_flags,
    };

    size_t max_payload_size = MAX_ALLOWED_RESPOND_BUFFER_SIZE;

    mod_scmi_from_protocol_api_scmi_frame_validation_ExpectAnyArgsAndReturn(
        SCMI_SUCCESS);

    uint16_t total_number_of_associated_pins = TOTAL_NUMBER_OF_ASSOCIATED_PINS;
    get_total_number_of_associations_ExpectAndReturn(
        payload.identifier,
        payload.flags,
        &total_number_of_associated_pins,
        FWK_SUCCESS);
    get_total_number_of_associations_IgnoreArg_total_count();
    get_total_number_of_associations_ReturnThruPtr_total_count(
        &total_number_of_associated_pins);

    get_max_payload_size_ExpectAndReturn(
        service_id, &max_payload_size, FWK_SUCCESS);
    get_max_payload_size_IgnoreArg_size();
    get_max_payload_size_ReturnThruPtr_size(&max_payload_size);

    for (size_t i = 0; i < EXPECTED_NUMBER_OF_IDENTIFIERS; ++i) {
        uint16_t object_id;

        get_list_associations_ExpectAndReturn(
            payload.identifier,
            payload.flags,
            payload.index + i,
            &object_id,
            FWK_SUCCESS);
        get_list_associations_IgnoreArg_object_id();
        get_list_associations_ReturnThruPtr_object_id(&object_id);

        scmi_write_payload_ExpectAndReturn(
            service_id,
            payload_size,
            &object_id,
            sizeof(object_id),
            FWK_SUCCESS);
        scmi_write_payload_IgnoreArg_payload();
        payload_size += (uint32_t)sizeof(uint16_t);
    }

    scmi_write_payload_ExpectAndReturn(
        service_id,
        0,
        &expected_return_value,
        sizeof(expected_return_value),
        FWK_SUCCESS);

    respond_StubWithCallback(list_association_pins_in_groups_respond_callback);

    status = scmi_pin_control_message_handler(
        protocol_id,
        service_id,
        (void *)&payload,
        sizeof(payload),
        MOD_SCMI_PIN_CONTROL_LIST_ASSOCIATIONS);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

int list_association_respond_callback_invalid_identifier(
    fwk_id_t service_id,
    const void *payload,
    size_t size,
    int cmock_num_calls)
{
    const uint32_t expected_payload_size = sizeof(int32_t);
    int32_t *acctual_return_values = (int32_t *)payload;

    TEST_ASSERT_EQUAL(SCMI_NOT_FOUND, *acctual_return_values);
    TEST_ASSERT_EQUAL(expected_payload_size, size);

    return FWK_SUCCESS;
}

void utest_list_association_with_invalid_identifier(void)
{
    int status = SCMI_GENERIC_ERROR;
    struct scmi_pin_control_list_associations_a2p payload = {
        /* Any random value can be chosen */
        .identifier = 3,
        .flags = MOD_PINCTRL_SELECTOR_GROUP,
        .index = 0,
    };

    mod_scmi_from_protocol_api_scmi_frame_validation_ExpectAnyArgsAndReturn(
        SCMI_SUCCESS);

    uint16_t total_number_of_associated_pins = 5;
    get_total_number_of_associations_ExpectAndReturn(
        payload.identifier,
        payload.flags,
        &total_number_of_associated_pins,
        FWK_E_RANGE);
    get_total_number_of_associations_IgnoreArg_total_count();
    get_total_number_of_associations_ReturnThruPtr_total_count(
        &total_number_of_associated_pins);

    respond_StubWithCallback(
        list_association_respond_callback_invalid_identifier);

    status = scmi_pin_control_message_handler(
        protocol_id,
        service_id,
        (void *)&payload,
        sizeof(payload),
        MOD_SCMI_PIN_CONTROL_LIST_ASSOCIATIONS);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

int attributes_respond_callback_name_get(
    fwk_id_t service_id,
    const void *payload,
    size_t size,
    int cmock_num_calls)
{
    char expected_name[] = "0123456789ABCDEF";
    struct scmi_pin_control_name_get_p2a *return_values =
        (struct scmi_pin_control_name_get_p2a *)payload;

    TEST_ASSERT_EQUAL_STRING(expected_name, return_values->name);
    TEST_ASSERT_EQUAL(SCMI_SUCCESS, return_values->status);

    return FWK_SUCCESS;
}

void utest_name_getting_extended_name(void)
{
    int status = SCMI_GENERIC_ERROR;
    char name[] = "0123456789ABCDEF";

    struct scmi_pin_control_name_get_a2p payload = {
        /* Any random value can be chosen */
        .identifier = 6,
        .flags = MOD_PINCTRL_SELECTOR_PIN,
    };

    struct mod_pinctrl_attributes pinctrl_attributes = {
        .name = name,
    };

    mod_scmi_from_protocol_api_scmi_frame_validation_ExpectAnyArgsAndReturn(
        SCMI_SUCCESS);

    get_attributes_ExpectAndReturn(
        payload.identifier, payload.flags, &pinctrl_attributes, FWK_SUCCESS);
    get_attributes_IgnoreArg_attributes();
    get_attributes_ReturnThruPtr_attributes(&pinctrl_attributes);

    respond_StubWithCallback(attributes_respond_callback_name_get);

    status = scmi_pin_control_message_handler(
        protocol_id,
        service_id,
        (void *)&payload,
        sizeof(payload),
        MOD_SCMI_PIN_CONTROL_NAME_GET);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

int attributes_respond_callback_none_extended_name_getting(
    fwk_id_t service_id,
    const void *payload,
    size_t size,
    int cmock_num_calls)
{
    struct scmi_pin_control_name_get_p2a *return_values =
        (struct scmi_pin_control_name_get_p2a *)payload;

    TEST_ASSERT_EQUAL(SCMI_NOT_FOUND, return_values->status);
    TEST_ASSERT_EQUAL(sizeof(return_values->status), size);

    return FWK_E_PARAM;
}

void utest_name_getting_none_extended_name(void)
{
    int status = SCMI_GENERIC_ERROR;
    char name[] = "0123456789A";

    struct scmi_pin_control_name_get_a2p payload = {
        /* Any random value can be chosen */
        .identifier = 6,
        .flags = MOD_PINCTRL_SELECTOR_PIN,
    };

    struct mod_pinctrl_attributes pinctrl_attributes = {
        .name = name,
    };

    mod_scmi_from_protocol_api_scmi_frame_validation_ExpectAnyArgsAndReturn(
        SCMI_SUCCESS);

    get_attributes_ExpectAndReturn(
        payload.identifier, payload.flags, &pinctrl_attributes, FWK_SUCCESS);
    get_attributes_IgnoreArg_attributes();
    get_attributes_ReturnThruPtr_attributes(&pinctrl_attributes);

    respond_StubWithCallback(
        attributes_respond_callback_none_extended_name_getting);

    status = scmi_pin_control_message_handler(
        protocol_id,
        service_id,
        (void *)&payload,
        sizeof(payload),
        MOD_SCMI_PIN_CONTROL_NAME_GET);

    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
}

int setting_get_respond_callback_specific_configuration_value(
    fwk_id_t service_id,
    const void *payload,
    size_t size,
    int cmock_num_calls)
{
    const uint32_t payload_size =
        sizeof(struct scmi_pin_control_settings_get_p2a) +
        sizeof(struct mod_pinctrl_drv_pin_configuration);

    TEST_ASSERT_EQUAL(NULL, payload);
    TEST_ASSERT_EQUAL(payload_size, size);

    return FWK_SUCCESS;
}

void utest_setting_get_specific_configuration_value(void)
{
    int status = SCMI_GENERIC_ERROR;
    const uint32_t config_flag = 0;
    const uint32_t flag_selector = 0;
    enum mod_pinctrl_drv_configuration_type config_type = SHIFT_LEFT_BY_POS(
        MOD_PINCTRL_DRV_TYPE_BIAS_PULL_UP, SETTING_GET_CONFIG_TYPE_POS);
    uint32_t config_value = 1;

    struct scmi_pin_control_settings_get_a2p payload = {
        /* Any random value can be chosen */
        .identifier = 2,
        .attributes = config_flag | flag_selector | config_type,
    };

    struct mod_pinctrl_drv_pin_configuration config_pair = {
        .config_type = MOD_PINCTRL_DRV_TYPE_BIAS_PULL_UP,
        .config_value = config_value,
    };

    struct scmi_pin_control_settings_get_p2a expected_return_values = {
        .status = (int32_t)SCMI_SUCCESS,
        .function_selected = 0,
        .num_configs = 1,
    };

    mod_scmi_from_protocol_api_scmi_frame_validation_ExpectAnyArgsAndReturn(
        SCMI_SUCCESS);

    get_configuration_value_from_type_ExpectAndReturn(
        payload.identifier,
        flag_selector,
        (uint32_t)config_type,
        &config_value,
        FWK_SUCCESS);
    get_configuration_value_from_type_IgnoreArg_config_value();
    get_configuration_value_from_type_ReturnThruPtr_config_value(&config_value);

    scmi_write_payload_ExpectAndReturn(
        service_id,
        sizeof(struct scmi_pin_control_settings_get_p2a),
        &config_pair,
        sizeof(config_pair),
        FWK_SUCCESS);

    scmi_write_payload_ExpectAndReturn(
        service_id,
        0,
        &expected_return_values,
        sizeof(expected_return_values),
        FWK_SUCCESS);

    respond_StubWithCallback(
        setting_get_respond_callback_specific_configuration_value);

    status = scmi_pin_control_message_handler(
        protocol_id,
        service_id,
        (void *)&payload,
        sizeof(payload),
        MOD_SCMI_PIN_CONTROL_SETTINGS_GET);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

int setting_get_all_configurations_respond_callback(
    fwk_id_t service_id,
    const void *payload,
    size_t size,
    int cmock_num_calls)
{
    const uint32_t number_of_configurations = 2;
    uint32_t expected_respond_size =
        sizeof(struct scmi_pin_control_settings_get_p2a) +
        number_of_configurations *
            sizeof(struct mod_pinctrl_drv_pin_configuration);

    TEST_ASSERT_EQUAL(expected_respond_size, size);
    TEST_ASSERT_EQUAL(NULL, payload);

    return FWK_SUCCESS;
}

void utest_setting_get_all_confgurations(void)
{
    int status = SCMI_GENERIC_ERROR;
    const uint32_t config_flag =
        SHIFT_LEFT_BY_POS(1, SETTING_GET_CONFIG_FLAG_POS);
    const uint32_t flag_selector = 0;
    uint16_t config_counts = 2;
    size_t max_payload_size = 50;

    struct scmi_pin_control_settings_get_a2p payload = {
        /* Any random value can be chosen */
        .identifier = 2,
        .attributes = config_flag | flag_selector,
    };

    uint32_t payload_offset =
        (uint32_t)sizeof(struct scmi_pin_control_settings_get_p2a);

    /* num_config should be matched with the total number of configs */
    struct scmi_pin_control_settings_get_p2a expected_return_values = {
        .status = (int32_t)SCMI_SUCCESS,
        .function_selected = 0,
        .num_configs = 2,
    };

    mod_scmi_from_protocol_api_scmi_frame_validation_ExpectAnyArgsAndReturn(
        SCMI_SUCCESS);

    get_total_number_of_configurations_ExpectAndReturn(
        payload.identifier, flag_selector, &config_counts, FWK_SUCCESS);
    get_total_number_of_configurations_IgnoreArg_number_of_configurations();
    get_total_number_of_configurations_ReturnThruPtr_number_of_configurations(
        &config_counts);

    get_max_payload_size_ExpectAndReturn(
        service_id, &max_payload_size, FWK_SUCCESS);
    get_max_payload_size_IgnoreArg_size();
    get_max_payload_size_ReturnThruPtr_size(&max_payload_size);

    struct mod_pinctrl_drv_pin_configuration config_pair[2] = {
        {
            .config_type = MOD_PINCTRL_DRV_TYPE_BIAS_PULL_UP,
            /* Any random value can be chosen */
            .config_value = 1,
        },
        {
            .config_type = MOD_PINCTRL_DRV_TYPE_LOW_POWER_MODE,
            /* Any random value can be chosen */
            .config_value = 1,
        }
    };

    for (int i = 0; i < config_counts; ++i) {
        get_configuration_ExpectAndReturn(
            payload.identifier, flag_selector, i, &config_pair[i], FWK_SUCCESS);
        get_configuration_IgnoreArg_config();
        get_configuration_ReturnThruPtr_config(&config_pair[i]);

        scmi_write_payload_ExpectAndReturn(
            service_id,
            payload_offset,
            &config_pair[i],
            sizeof(config_pair[i]),
            FWK_SUCCESS);
        scmi_write_payload_IgnoreArg_payload();

        payload_offset += sizeof(struct mod_pinctrl_drv_pin_configuration);
    }

    scmi_write_payload_ExpectAndReturn(
        service_id,
        0,
        &expected_return_values,
        sizeof(expected_return_values),
        FWK_SUCCESS);
    scmi_write_payload_IgnoreArg_payload();

    respond_StubWithCallback(setting_get_all_configurations_respond_callback);

    status = scmi_pin_control_message_handler(
        protocol_id,
        service_id,
        (void *)&payload,
        sizeof(payload),
        MOD_SCMI_PIN_CONTROL_SETTINGS_GET);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

int setting_get_function_respond_callback(
    fwk_id_t service_id,
    const void *payload,
    size_t size,
    int cmock_num_calls)
{
    const uint32_t expected_respond_size =
        sizeof(struct scmi_pin_control_settings_get_p2a);

    TEST_ASSERT_EQUAL(NULL, payload);
    TEST_ASSERT_EQUAL(expected_respond_size, size);

    return FWK_SUCCESS;
}

void utest_setting_get_function_selected(void)
{
    int status = SCMI_GENERIC_ERROR;
    uint32_t config_flag = SHIFT_LEFT_BY_POS(
        SCMI_PIN_CONTROL_FUNCTION_SELECTED, SETTING_GET_CONFIG_FLAG_POS);
    uint32_t selector_flag = MOD_PINCTRL_SELECTOR_PIN;
    /* Any chosen random value between 0x00 and 0xFFFF */
    uint32_t function_id = 4;
    struct scmi_pin_control_settings_get_a2p payload = {
        /* Any random value can be chosen */
        .identifier = 2,
        .attributes = config_flag | selector_flag,
    };

    struct scmi_pin_control_settings_get_p2a expected_return_values = {
        .status = (int32_t)SCMI_SUCCESS,
        .function_selected = function_id,
        .num_configs = 0,
    };

    mod_scmi_from_protocol_api_scmi_frame_validation_ExpectAnyArgsAndReturn(
        SCMI_SUCCESS);

    get_current_associated_function_ExpectAndReturn(
        payload.identifier, selector_flag, &function_id, FWK_SUCCESS);
    get_current_associated_function_IgnoreArg_function_id();
    get_current_associated_function_ReturnThruPtr_function_id(&function_id);

    scmi_write_payload_ExpectAndReturn(
        service_id,
        0,
        &expected_return_values,
        sizeof(expected_return_values),
        FWK_SUCCESS);

    respond_StubWithCallback(setting_get_function_respond_callback);

    status = scmi_pin_control_message_handler(
        protocol_id,
        service_id,
        (void *)&payload,
        sizeof(payload),
        MOD_SCMI_PIN_CONTROL_SETTINGS_GET);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

int setting_get_no_function_selected_respond_callback(
    fwk_id_t service_id,
    const void *payload,
    size_t size,
    int cmock_num_calls)
{
    const uint32_t expected_respond_size =
        sizeof(struct scmi_pin_control_settings_get_p2a);

    TEST_ASSERT_EQUAL(NULL, payload);
    TEST_ASSERT_EQUAL(expected_respond_size, size);

    return FWK_SUCCESS;
}

void utest_setting_get_no_function_is_selected(void)
{
    int status = SCMI_GENERIC_ERROR;
    uint32_t config_flag = SHIFT_LEFT_BY_POS(
        SCMI_PIN_CONTROL_FUNCTION_SELECTED, SETTING_GET_CONFIG_FLAG_POS);
    uint32_t selector_flag = MOD_PINCTRL_SELECTOR_PIN;
    uint32_t function_id = NO_FUNCTION_IS_SELECTED;
    struct scmi_pin_control_settings_get_a2p payload = {
        /* Any random value can be chosen */
        .identifier = 2,
        .attributes = config_flag | selector_flag,
    };

    struct scmi_pin_control_settings_get_p2a expected_return_values = {
        .status = (int32_t)SCMI_SUCCESS,
        .function_selected = NO_FUNCTION_IS_SELECTED,
        .num_configs = 0,
    };

    mod_scmi_from_protocol_api_scmi_frame_validation_ExpectAnyArgsAndReturn(
        SCMI_SUCCESS);

    get_current_associated_function_ExpectAndReturn(
        payload.identifier, selector_flag, &function_id, FWK_SUCCESS);
    get_current_associated_function_IgnoreArg_function_id();
    get_current_associated_function_ReturnThruPtr_function_id(&function_id);

    scmi_write_payload_ExpectAndReturn(
        service_id,
        0,
        &expected_return_values,
        sizeof(expected_return_values),
        FWK_SUCCESS);

    respond_StubWithCallback(setting_get_no_function_selected_respond_callback);

    status = scmi_pin_control_message_handler(
        protocol_id,
        service_id,
        (void *)&payload,
        sizeof(payload),
        MOD_SCMI_PIN_CONTROL_SETTINGS_GET);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

int setting_configuration_respond_callback(
    fwk_id_t service_id,
    const void *payload,
    size_t size,
    int cmock_num_calls)
{
    struct scmi_pin_control_settings_configure_p2a *respond =
        (struct scmi_pin_control_settings_configure_p2a *)payload;

    TEST_ASSERT_EQUAL(SCMI_SUCCESS, respond->status);
    TEST_ASSERT_EQUAL(
        sizeof(struct scmi_pin_control_settings_configure_p2a), size);

    return FWK_SUCCESS;
}

void utest_setting_configuration(void)
{
    int status = SCMI_GENERIC_ERROR;

    struct mod_pinctrl_drv_pin_configuration configs[2] = {
        {
            .config_type = MOD_PINCTRL_DRV_TYPE_BIAS_BUS_HOLD,
            .config_value = 1,
        },
        {
            .config_type = MOD_PINCTRL_DRV_TYPE_PULL_MODE,
            .config_value = 1,
        }
    };

    const uint32_t number_of_config_to_set = SHIFT_LEFT_BY_POS(
        FWK_ARRAY_SIZE(configs), SETTING_NUMBER_OF_CONFIG_POS);
    const uint32_t selector_flag = SHIFT_LEFT_BY_POS(1, SETTING_FLAG_POS);

    struct scmi_pin_control_settings_configure_a2p payload = {
        /* Any random value can be chosen */
        .identifier = 2,
        .function_id = NO_FUNCTION_IS_SELECTED,
        .attributes = number_of_config_to_set | selector_flag,
    };

    uint32_t raw_payload[PINCTRL_SETTING_CONFIG_SIZE_OF_PAYLOAD] = {
        payload.identifier,      payload.function_id,
        payload.attributes,      configs[0].config_type,
        configs[0].config_value, configs[1].config_type,
        configs[1].config_value,
    };

    mod_scmi_from_protocol_api_scmi_frame_validation_ExpectAnyArgsAndReturn(
        SCMI_SUCCESS);

    for (int i = 0; i < (int)FWK_ARRAY_SIZE(configs); ++i) {
        const struct mod_pinctrl_drv_pin_configuration *config_pair =
            &configs[i];
        set_configuration_ExpectAndReturn(
            payload.identifier, selector_flag, config_pair, FWK_SUCCESS);
        set_configuration_IgnoreArg_config();
    }

    respond_StubWithCallback(setting_configuration_respond_callback);

    status = scmi_pin_control_message_handler(
        protocol_id,
        service_id,
        (void *)&raw_payload,
        FWK_ARRAY_SIZE(raw_payload) * sizeof(uint32_t),
        MOD_SCMI_PIN_CONTROL_SETTINGS_CONFIGURE);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void utest_setting_function(void)
{
    int status = SCMI_GENERIC_ERROR;
    const uint32_t function_id = SHIFT_LEFT_BY_POS(1, SETTING_FUNC_POS);
    const uint32_t number_of_config_to_set =
        SHIFT_LEFT_BY_POS(0, SETTING_NUMBER_OF_CONFIG_POS);
    const uint32_t selector_flag = SHIFT_LEFT_BY_POS(1, SETTING_FLAG_POS);

    struct scmi_pin_control_settings_configure_a2p payload = {
        /* Any random value can be chosen */
        .identifier = 2,
        .function_id = NO_FUNCTION_IS_SELECTED,
        .attributes = function_id | number_of_config_to_set | selector_flag,
    };

    struct scmi_pin_control_settings_configure_p2a return_values = {
        .status = (int32_t)SCMI_SUCCESS,
    };

    mod_scmi_from_protocol_api_scmi_frame_validation_ExpectAnyArgsAndReturn(
        SCMI_SUCCESS);

    set_function_ExpectAndReturn(
        payload.identifier,
        selector_flag,
        (uint16_t)payload.function_id,
        FWK_SUCCESS);

    respond_ExpectWithArrayAndReturn(
        service_id,
        (void *)&return_values,
        sizeof(return_values),
        sizeof(return_values),
        FWK_SUCCESS);

    status = scmi_pin_control_message_handler(
        protocol_id,
        service_id,
        (void *)&payload,
        sizeof(payload),
        MOD_SCMI_PIN_CONTROL_SETTINGS_CONFIGURE);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void utest_setting_configuration_and_function(void)
{
    int status = SCMI_GENERIC_ERROR;
    const uint32_t function_id = SHIFT_LEFT_BY_POS(1, SETTING_FUNC_POS);
    const uint32_t number_of_config_to_set =
        SHIFT_LEFT_BY_POS(2, SETTING_NUMBER_OF_CONFIG_POS);
    const uint32_t selector_flag = SHIFT_LEFT_BY_POS(1, SETTING_FLAG_POS);

    struct scmi_pin_control_settings_configure_a2p payload = {
        /* Any random value can be chosen */
        .identifier = 2,
        .function_id = NO_FUNCTION_IS_SELECTED,
        .attributes = function_id | number_of_config_to_set | selector_flag,
    };

    struct mod_pinctrl_drv_pin_configuration configs[] = {
        {
            .config_type = MOD_PINCTRL_DRV_TYPE_BIAS_BUS_HOLD,
            .config_value = 1,
        },
        {
            .config_type = MOD_PINCTRL_DRV_TYPE_PULL_MODE,
            .config_value = 1,
        }
    };

    uint32_t raw_payload[PINCTRL_SETTING_CONFIG_SIZE_OF_PAYLOAD] = {
        payload.identifier,      payload.function_id,
        payload.attributes,      configs[0].config_type,
        configs[0].config_value, configs[1].config_type,
        configs[1].config_value,
    };

    struct scmi_pin_control_settings_configure_p2a return_values = {
        .status = (int32_t)SCMI_SUCCESS,
    };

    mod_scmi_from_protocol_api_scmi_frame_validation_ExpectAnyArgsAndReturn(
        SCMI_SUCCESS);

    set_function_ExpectAndReturn(
        payload.identifier,
        selector_flag,
        (uint16_t)payload.function_id,
        FWK_SUCCESS);

    for (int i = 0; i < 2; ++i) {
        set_configuration_ExpectAndReturn(
            payload.identifier, selector_flag, &configs[i], FWK_SUCCESS);
        set_configuration_IgnoreArg_config();
    }

    respond_ExpectWithArrayAndReturn(
        service_id,
        (void *)&return_values,
        sizeof(return_values),
        sizeof(return_values),
        FWK_SUCCESS);

    status = scmi_pin_control_message_handler(
        protocol_id,
        service_id,
        (void *)&raw_payload,
        PINCTRL_SETTING_CONFIG_SIZE_OF_PAYLOAD * sizeof(uint32_t),
        MOD_SCMI_PIN_CONTROL_SETTINGS_CONFIGURE);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

int scmi_pin_control_protocol_test_main(void)
{
    UNITY_BEGIN();

    RUN_TEST(utest_protocol_version);

    RUN_TEST(utest_protocol_message_attributes);
    RUN_TEST(utest_protocol_message_attributes_message_id_out_of_range);

    RUN_TEST(utest_protocol_attributes);

    RUN_TEST(utest_attributes_group);
    RUN_TEST(utest_attributes_function);
    RUN_TEST(utest_attributes_pin);
    RUN_TEST(utest_attributes_id_out_of_range);

    RUN_TEST(utest_list_association_with_valid_identifier);
    RUN_TEST(utest_list_association_with_invalid_identifier);

    RUN_TEST(utest_name_getting_extended_name);
    RUN_TEST(utest_name_getting_none_extended_name);

    RUN_TEST(utest_setting_get_specific_configuration_value);
    RUN_TEST(utest_setting_get_all_confgurations);
    RUN_TEST(utest_setting_get_function_selected);
    RUN_TEST(utest_setting_get_no_function_is_selected);

    RUN_TEST(utest_setting_configuration);
    RUN_TEST(utest_setting_function);
    RUN_TEST(utest_setting_configuration_and_function);

    return UNITY_END();
}

#if !defined(TEST_ON_TARGET)
int main(void)
{
    return scmi_pin_control_protocol_test_main();
}
#endif
