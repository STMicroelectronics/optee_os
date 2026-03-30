/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config_pinctrl.h"
#include "fwk_module_idx.h"
#include "mod_pinctrl.h"
#include "scp_unity.h"
#include "unity.h"

#include <Mockmod_pinctrl_drv_extra.h>

#include UNIT_TEST_SRC

struct mod_pinctrl_drv_api pinctrl_driver_api[] = {
    {
        .set_pin_function = set_pin_function,
        .set_pin_configuration = set_pin_configuration,
        .get_pin_configuration = get_pin_configuration,
        .get_pin_function = get_pin_function,
    },
};

struct mod_pinctrl_domain_config drv_domain_config[] = {
    {
        .driver_id = pinctrl_drv_id,
        .driver_api_id = pinctrl_drv_api_id,
        .pin_base_id = PIN_0_IDX,
        .pin_range = PIN_COUNT,
    },
};

struct mod_pinctrl_domain_ctx pinctrl_drv_domain[] = {
    {
        .drv_domain_config = drv_domain_config,
        .pinctrl_driver_api = pinctrl_driver_api,
    },
};

void setUp(void)
{
    pinctrl_ctx.config = &config;
    pinctrl_ctx.pinctrl_drv_domain_count = FWK_ARRAY_SIZE(drv_domain_config);
    pinctrl_ctx.pinctrl_drv_domain_ctx = pinctrl_drv_domain;
}
void tearDown(void)
{
}

void utest_pinctrl_get_group_attributes(void)
{
    int status = FWK_SUCCESS;

    uint16_t identifier = I2C_GROUP_IDX;
    enum mod_pinctrl_selector flags = MOD_PINCTRL_SELECTOR_GROUP;
    struct mod_pinctrl_attributes pinctrl_attributes;

    status = get_attributes(identifier, flags, &pinctrl_attributes);

    TEST_ASSERT_EQUAL_STRING(groups[identifier].name, pinctrl_attributes.name);
    TEST_ASSERT_EQUAL(
        FWK_ARRAY_SIZE(grp_i2c_pin_idx), pinctrl_attributes.number_of_elements);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void utest_pinctrl_get_function_attributes(void)
{
    int status = FWK_SUCCESS;
    uint16_t identifier = UART_FUNC_IDX;
    enum mod_pinctrl_selector flags = MOD_PINCTRL_SELECTOR_FUNCTION;
    struct mod_pinctrl_attributes pinctrl_attributes;
    const uint16_t expected_number_of_groups = 2;

    status = get_attributes(identifier, flags, &pinctrl_attributes);

    TEST_ASSERT_EQUAL_STRING(
        functions[identifier].name, pinctrl_attributes.name);
    TEST_ASSERT_EQUAL(
        expected_number_of_groups, pinctrl_attributes.number_of_elements);
    TEST_ASSERT_EQUAL(false, pinctrl_attributes.is_gpio_function);
    TEST_ASSERT_EQUAL(false, pinctrl_attributes.is_pin_only_function);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void utest_pinctrl_get_pin_only_fucntion_attributes(void)
{
    int status = FWK_SUCCESS;
    uint16_t identifier = GPIO_FUNC_IDX;
    enum mod_pinctrl_selector flags = MOD_PINCTRL_SELECTOR_FUNCTION;
    struct mod_pinctrl_attributes pinctrl_attributes;
    uint16_t expected_number_of_pins = 5;

    status = get_attributes(identifier, flags, &pinctrl_attributes);

    TEST_ASSERT_EQUAL_STRING(
        functions[identifier].name, pinctrl_attributes.name);
    TEST_ASSERT_EQUAL(
        expected_number_of_pins, pinctrl_attributes.number_of_elements);
    TEST_ASSERT_EQUAL(false, pinctrl_attributes.is_gpio_function);
    TEST_ASSERT_EQUAL(true, pinctrl_attributes.is_pin_only_function);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void utest_pinctrl_get_pin_attributes(void)
{
    int status = FWK_SUCCESS;
    uint16_t identifier = PIN_3_IDX;
    enum mod_pinctrl_selector flags = MOD_PINCTRL_SELECTOR_PIN;
    struct mod_pinctrl_attributes pinctrl_attributes;
    const uint16_t expected_number_of_pins = 1;
    char *expected_pin_name = "PIN_3";

    status = get_attributes(identifier, flags, &pinctrl_attributes);

    TEST_ASSERT_EQUAL_STRING(expected_pin_name, pinctrl_attributes.name);
    TEST_ASSERT_EQUAL(
        expected_number_of_pins, pinctrl_attributes.number_of_elements);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void utest_pinctrl_get_attribute_invalid_flags(void)
{
    int status = FWK_SUCCESS;

    uint16_t identifier = I2C_GROUP_IDX;
    enum mod_pinctrl_selector flags = 3;
    struct mod_pinctrl_attributes pinctrl_attributes;

    status = get_attributes(identifier, flags, &pinctrl_attributes);

    TEST_ASSERT_EQUAL(FWK_E_RANGE, status);
}

void utest_pinctrl_get_attribute_invalid_group_id(void)
{
    int status = FWK_SUCCESS;

    uint16_t identifier = GROUP_COUNT;
    enum mod_pinctrl_selector flags = MOD_PINCTRL_SELECTOR_GROUP;
    struct mod_pinctrl_attributes pinctrl_attributes;

    status = get_attributes(identifier, flags, &pinctrl_attributes);

    TEST_ASSERT_EQUAL(FWK_E_RANGE, status);
}

void utest_pinctrl_get_attribute_invalid_function_id(void)
{
    int status = FWK_SUCCESS;

    uint16_t identifier = FUNC_COUNT;
    enum mod_pinctrl_selector flags = MOD_PINCTRL_SELECTOR_FUNCTION;
    struct mod_pinctrl_attributes pinctrl_attributes;

    status = get_attributes(identifier, flags, &pinctrl_attributes);

    TEST_ASSERT_EQUAL(FWK_E_RANGE, status);
}

void utest_pinctrl_get_attribute_invalid_pin_id(void)
{
    int status = FWK_SUCCESS;
    uint16_t identifier = PIN_COUNT;
    enum mod_pinctrl_selector flags = MOD_PINCTRL_SELECTOR_PIN;
    struct mod_pinctrl_attributes pinctrl_attributes;

    status = get_attributes(identifier, flags, &pinctrl_attributes);

    TEST_ASSERT_EQUAL(FWK_E_RANGE, status);
}

void utest_get_info(void)
{
    int status = FWK_SUCCESS;
    struct mod_pinctrl_info info;

    status = get_info(&info);

    TEST_ASSERT_EQUAL(PIN_COUNT, info.number_of_pins);
    TEST_ASSERT_EQUAL(GROUP_COUNT, info.number_of_groups);
    TEST_ASSERT_EQUAL(FUNC_COUNT, info.number_of_functions);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void utest_list_pins_associated_with_group(void)
{
    int status = FWK_SUCCESS;
    uint16_t pin_id;

    uint16_t group_index = SPI_GROUP_IDX;
    enum mod_pinctrl_selector flags = MOD_PINCTRL_SELECTOR_GROUP;
    uint16_t index = 1;

    status = get_list_associations(group_index, flags, index, &pin_id);

    TEST_ASSERT_EQUAL(PIN_3_IDX, pin_id);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void utest_list_pins_associated_with_group_out_of_range_indx(void)
{
    int status = FWK_SUCCESS;
    uint16_t pin_id;

    uint16_t group_index = SPI_GROUP_IDX;
    enum mod_pinctrl_selector flags = MOD_PINCTRL_SELECTOR_GROUP;
    /* Try pin index out of range while the SPI group have only 3 pins */
    uint16_t index = 3;

    status = get_list_associations(group_index, flags, index, &pin_id);

    TEST_ASSERT_EQUAL(FWK_E_RANGE, status);
}

void utest_list_groups_associated_with_function(void)
{
    int status = FWK_SUCCESS;
    uint16_t group_id;

    uint16_t function_index = SPI_FUNC_IDX;
    enum mod_pinctrl_selector flags = MOD_PINCTRL_SELECTOR_FUNCTION;
    uint16_t index = 0;

    status = get_list_associations(function_index, flags, index, &group_id);

    TEST_ASSERT_EQUAL(SPI_GROUP_IDX, group_id);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void utest_list_multiple_groups_associated_with_the_same_function(void)
{
    int status = FWK_SUCCESS;
    /* array to contain the group_id's got from asking to list all groups
     * associated with certain functionality which is 2 in out configuration */
    uint16_t group_id[2];

    uint16_t function_index = UART_FUNC_IDX;
    enum mod_pinctrl_selector flags = MOD_PINCTRL_SELECTOR_FUNCTION;
    uint16_t index = 0;

    uint16_t group_counts = 0;
    status =
        get_total_number_of_associations(function_index, flags, &group_counts);

    for (index = 0; index < group_counts; ++index) {
        status = get_list_associations(
            function_index, flags, index, &group_id[index]);
    }

    TEST_ASSERT_EQUAL(UART_GROUP_IDX, group_id[0]);
    TEST_ASSERT_EQUAL(UART_1_GROUP_IDX, group_id[1]);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void utest_list_groups_associated_with_function_out_of_range_indx(void)
{
    int status = FWK_SUCCESS;
    uint16_t group_id;

    uint16_t function_index = SPI_FUNC_IDX;
    enum mod_pinctrl_selector flags = MOD_PINCTRL_SELECTOR_FUNCTION;
    /* Try index value out of range where we have only one group has SPI func */
    uint16_t index = 1;

    status = get_list_associations(function_index, flags, index, &group_id);

    TEST_ASSERT_EQUAL(FWK_E_RANGE, status);
}

void utest_list_pins_associated_with_function(void)
{
    int status = FWK_SUCCESS;
    uint16_t pin_id;

    uint16_t function_idx = GPIO_FUNC_IDX;
    enum mod_pinctrl_selector flags = MOD_PINCTRL_SELECTOR_FUNCTION;
    /* Try index value in range where GPIO func has 5 pins */
    uint16_t index = 2;

    status = get_list_associations(function_idx, flags, index, &pin_id);

    TEST_ASSERT_EQUAL(PIN_2_IDX, pin_id);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void utest_list_pins_associated_with_function_invalid_index(void)
{
    int status = FWK_SUCCESS;
    uint16_t pin_id = PIN_3_IDX;

    uint16_t function_idx = GPIO_FUNC_IDX;
    enum mod_pinctrl_selector flags = MOD_PINCTRL_SELECTOR_FUNCTION;
    /* Try out of range index where the GPIO func is only 5 pins */
    uint16_t index = 5;

    status = get_list_associations(function_idx, flags, index, &pin_id);

    TEST_ASSERT_EQUAL(PIN_3_IDX, pin_id);
    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
}

void utest_get_total_number_of_pins_associated_with_group(void)
{
    int status;
    enum mod_pinctrl_selector flags = MOD_PINCTRL_SELECTOR_GROUP;
    uint16_t group_index = I2C_GROUP_IDX;
    uint16_t expected_total_pins;

    status = get_total_number_of_associations(
        group_index, flags, &expected_total_pins);

    TEST_ASSERT_EQUAL(FWK_ARRAY_SIZE(grp_i2c_pin_idx), expected_total_pins);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void utest_get_total_number_of_groups_associated_with_function(void)
{
    int status;
    enum mod_pinctrl_selector flags = MOD_PINCTRL_SELECTOR_FUNCTION;
    uint16_t function_idx = UART_FUNC_IDX;
    uint16_t expected_total_count;

    status = get_total_number_of_associations(
        function_idx, flags, &expected_total_count);

    /* Expected total number of groups associated with the UART_FUNC to be 2*/
    TEST_ASSERT_EQUAL(2, expected_total_count);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void utest_get_total_number_of_pins_associated_with_function(void)
{
    int status;
    enum mod_pinctrl_selector flags = MOD_PINCTRL_SELECTOR_FUNCTION;
    uint16_t function_idx = GPIO_FUNC_IDX;
    uint16_t total_count;

    status =
        get_total_number_of_associations(function_idx, flags, &total_count);

    TEST_ASSERT_EQUAL(PIN_WITH_GPIO_FUNC, total_count);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void utest_get_pin_configuration_value_from_type(void)
{
    int status;
    uint16_t pin_idx = PIN_6_IDX;
    enum mod_pinctrl_selector flags = MOD_PINCTRL_SELECTOR_PIN;
    struct mod_pinctrl_drv_pin_configuration pin_config = {
        .config_type = MOD_PINCTRL_DRV_TYPE_DRIVE_OPEN_SOURCE,
        .config_value = DRIVE_OPEN_SOURCE_VALUE,
    };

    get_pin_configuration_ExpectAndReturn(PIN_6_IDX, &pin_config, FWK_SUCCESS);
    get_pin_configuration_IgnoreArg_pin_config();
    get_pin_configuration_ReturnThruPtr_pin_config(&pin_config);

    status = get_configuration_value_from_type(
        pin_idx, flags, pin_config.config_type, &pin_config.config_value);

    TEST_ASSERT_EQUAL(DRIVE_OPEN_SOURCE_VALUE, pin_config.config_value);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void utest_get_group_configuration_value_from_type(void)
{
    int status;
    uint16_t group_idx = GPIO_GROUP_IDX;
    enum mod_pinctrl_selector flags = MOD_PINCTRL_SELECTOR_GROUP;

    struct mod_pinctrl_drv_pin_configuration config = {
        .config_type = MOD_PINCTRL_DRV_TYPE_LOW_POWER_MODE,
        .config_value = LOW_POWER_MODE_VALUE,
    };

    get_pin_configuration_ExpectAndReturn(PIN_0_IDX, &config, FWK_SUCCESS);
    get_pin_configuration_IgnoreArg_pin_config();
    get_pin_configuration_ReturnThruPtr_pin_config(&config);

    status = get_configuration_value_from_type(
        group_idx, flags, config.config_type, &config.config_value);

    TEST_ASSERT_EQUAL(LOW_POWER_MODE_VALUE, config.config_value);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void utest_get_configuration_value_from_type_invalid_flag(void)
{
    int status;
    uint16_t group_idx = I2C_GROUP_IDX;
    enum mod_pinctrl_selector flags = 3;
    enum mod_pinctrl_drv_configuration_type config_type =
        MOD_PINCTRL_DRV_TYPE_INPUT_DEBOUNCE;
    uint32_t config_value;

    status = get_configuration_value_from_type(
        group_idx, flags, config_type, &config_value);

    TEST_ASSERT_EQUAL(FWK_E_RANGE, status);
}

void utest_get_configuration_value_from_type_invalid_group_id(void)
{
    int status;
    uint16_t group_idx = GROUP_COUNT;
    enum mod_pinctrl_selector flags = MOD_PINCTRL_SELECTOR_GROUP;
    enum mod_pinctrl_drv_configuration_type config_type =
        MOD_PINCTRL_DRV_TYPE_INPUT_DEBOUNCE;
    uint32_t config_value;

    status = get_configuration_value_from_type(
        group_idx, flags, config_type, &config_value);

    TEST_ASSERT_EQUAL(FWK_E_RANGE, status);
}

void utest_get_configuration_value_from_type_invalid_group_type(void)
{
    int status;
    uint16_t group_idx = I2C_GROUP_IDX;
    enum mod_pinctrl_selector flags = MOD_PINCTRL_SELECTOR_GROUP;
    enum mod_pinctrl_drv_configuration_type config_type =
        MOD_PINCTRL_DRV_TYPE_INPUT_DEBOUNCE;
    uint32_t config_value;

    status = get_configuration_value_from_type(
        group_idx, flags, config_type, &config_value);

    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
}

void utest_get_group_configurations(void)
{
    int status;

    uint16_t group_idx = GPIO_GROUP_IDX;
    enum mod_pinctrl_selector flags = MOD_PINCTRL_SELECTOR_GROUP;
    uint16_t configration_index = 1;
    struct mod_pinctrl_drv_pin_configuration config = {
        .config_type = MOD_PINCTRL_DRV_TYPE_LOW_POWER_MODE,
        .config_value = LOW_POWER_MODE_VALUE,
    };

    get_pin_configuration_ExpectAndReturn(PIN_0_IDX, &config, FWK_SUCCESS);
    get_pin_configuration_IgnoreArg_pin_config();
    get_pin_configuration_ReturnThruPtr_pin_config(&config);

    status = get_configuration(group_idx, flags, configration_index, &config);

    TEST_ASSERT_EQUAL(MOD_PINCTRL_DRV_TYPE_LOW_POWER_MODE, config.config_type);
    TEST_ASSERT_EQUAL(LOW_POWER_MODE_VALUE, config.config_value);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void utest_get_pin_configurations(void)
{
    int status;

    uint16_t pin_idx = PIN_4_IDX;
    enum mod_pinctrl_selector flags = MOD_PINCTRL_SELECTOR_PIN;
    uint16_t configration_index = 0;
    struct mod_pinctrl_drv_pin_configuration config = {
        .config_type = MOD_PINCTRL_DRV_TYPE_DRIVE_OPEN_SOURCE,
        .config_value = DRIVE_OPEN_SOURCE_VALUE,
    };

    get_pin_configuration_ExpectAndReturn(PIN_4_IDX, &config, FWK_SUCCESS);
    get_pin_configuration_IgnoreArg_pin_config();
    get_pin_configuration_ReturnThruPtr_pin_config(&config);

    status = get_configuration(pin_idx, flags, configration_index, &config);

    TEST_ASSERT_EQUAL(
        MOD_PINCTRL_DRV_TYPE_DRIVE_OPEN_SOURCE, config.config_type);
    TEST_ASSERT_EQUAL(DRIVE_OPEN_SOURCE_VALUE, config.config_value);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void utest_get_configurations_invalid_flags(void)
{
    int status;

    uint16_t pin_idx = PIN_4_IDX;
    enum mod_pinctrl_selector flags = MOD_PINCTRL_SELECTOR_FUNCTION;
    uint16_t configration_index = 4;

    struct mod_pinctrl_drv_pin_configuration config;

    status = get_configuration(pin_idx, flags, configration_index, &config);

    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
}

void utest_get_total_number_of_group_configurations(void)
{
    int status;

    uint16_t group_idx = I2C_GROUP_IDX;
    enum mod_pinctrl_selector flags = MOD_PINCTRL_SELECTOR_GROUP;
    uint16_t number_of_configurations;
    uint16_t expected_number_of_configurations = 3;

    status = get_total_number_of_configurations(
        group_idx, flags, &number_of_configurations);

    TEST_ASSERT_EQUAL(
        expected_number_of_configurations, number_of_configurations);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void utest_get_total_number_of_configurations_invalid_group_id(void)
{
    int status;

    uint16_t group_idx = GROUP_COUNT;
    enum mod_pinctrl_selector flags = MOD_PINCTRL_SELECTOR_GROUP;
    uint16_t number_of_configurations;

    status = get_total_number_of_configurations(
        group_idx, flags, &number_of_configurations);

    TEST_ASSERT_EQUAL(FWK_E_RANGE, status);
}

void utest_get_total_number_of_pin_configurations(void)
{
    int status;

    uint16_t pin_idx = PIN_2_IDX;
    enum mod_pinctrl_selector flags = MOD_PINCTRL_SELECTOR_PIN;
    uint16_t number_of_configurations;
    uint16_t expected_number_of_configurations = 3;

    status = get_total_number_of_configurations(
        pin_idx, flags, &number_of_configurations);

    TEST_ASSERT_EQUAL(
        expected_number_of_configurations, number_of_configurations);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void utest_get_total_number_of_configurations_invalid_flags(void)
{
    int status;

    uint16_t pin_idx = PIN_2_IDX;
    enum mod_pinctrl_selector flags = MOD_PINCTRL_SELECTOR_FUNCTION;
    uint16_t number_of_configurations;

    status = get_total_number_of_configurations(
        pin_idx, flags, &number_of_configurations);

    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
}

void utest_get_pin_associated_function(void)
{
    int status;

    uint16_t pin_index = PIN_0_IDX;
    enum mod_pinctrl_selector flags = MOD_PINCTRL_SELECTOR_PIN;
    uint32_t function_id = UART_FUNC_IDX;
    uint32_t expected_function_id = GPIO_FUNC_IDX;

    get_pin_function_ExpectAndReturn(
        PIN_0_IDX, &expected_function_id, FWK_SUCCESS);
    get_pin_function_IgnoreArg_function_id();
    get_pin_function_ReturnThruPtr_function_id(&expected_function_id);

    status = get_current_associated_function(pin_index, flags, &function_id);

    TEST_ASSERT_EQUAL(expected_function_id, function_id);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void utest_get_group_associated_function(void)
{
    int status;

    uint16_t group_index = SPI_GROUP_IDX;
    enum mod_pinctrl_selector flags = MOD_PINCTRL_SELECTOR_GROUP;
    uint32_t function_id = SPI_FUNC_IDX;
    uint32_t expected_function_id = I2C_FUNC_IDX;

    get_pin_function_ExpectAndReturn(
        PIN_2_IDX, &expected_function_id, FWK_SUCCESS);
    get_pin_function_IgnoreArg_function_id();
    get_pin_function_ReturnThruPtr_function_id(&expected_function_id);

    status = get_current_associated_function(group_index, flags, &function_id);

    TEST_ASSERT_EQUAL(expected_function_id, function_id);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void utest_get_group_associated_function_invalid_flag(void)
{
    int status;

    uint16_t group_index = SPI_GROUP_IDX;
    enum mod_pinctrl_selector flags = MOD_PINCTRL_SELECTOR_FUNCTION;
    uint32_t function_id;

    status = get_current_associated_function(group_index, flags, &function_id);

    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
}

void utest_set_pin_configurations(void)
{
    int status;

    uint16_t pin_index = PIN_1_IDX;
    enum mod_pinctrl_selector flags = MOD_PINCTRL_SELECTOR_PIN;
    const struct mod_pinctrl_drv_pin_configuration config = {
        .config_type = MOD_PINCTRL_DRV_TYPE_LOW_POWER_MODE,
        .config_value = 4,
    };

    set_pin_configuration_ExpectAndReturn(PIN_1_IDX, &config, FWK_SUCCESS);

    status = set_configuration(pin_index, flags, &config);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void utest_set_group_configuration(void)
{
    int status;

    uint16_t group_id = I2C_GROUP_IDX;
    enum mod_pinctrl_selector flags = MOD_PINCTRL_SELECTOR_GROUP;

    const struct mod_pinctrl_drv_pin_configuration configs = {
        .config_type = MOD_PINCTRL_DRV_TYPE_DRIVE_OPEN_SOURCE,
        .config_value = 0,
    };

    set_pin_configuration_ExpectAndReturn(PIN_5_IDX, &configs, FWK_SUCCESS);
    set_pin_configuration_ExpectAndReturn(PIN_6_IDX, &configs, FWK_SUCCESS);

    status = set_configuration(group_id, flags, &configs);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void utest_set_group_configuration_group_id_invalid(void)
{
    int status;

    uint16_t group_id = GROUP_COUNT;
    enum mod_pinctrl_selector flags = MOD_PINCTRL_SELECTOR_GROUP;

    const struct mod_pinctrl_drv_pin_configuration configs = {
        .config_type = MOD_PINCTRL_DRV_TYPE_BIAS_DISABLE,
        .config_value = 1,
    };

    status = set_configuration(group_id, flags, &configs);

    TEST_ASSERT_EQUAL(FWK_E_RANGE, status);
}

void utest_set_group_configuration_invalid_flag(void)
{
    int status;

    uint16_t group_id = I2C_GROUP_IDX;
    enum mod_pinctrl_selector flags = MOD_PINCTRL_SELECTOR_FUNCTION;

    const struct mod_pinctrl_drv_pin_configuration configs = {
        .config_type = MOD_PINCTRL_DRV_TYPE_BIAS_BUS_HOLD,
        .config_value = 1,
    };

    status = set_configuration(group_id, flags, &configs);

    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
}

void utest_set_pin_read_only_configuration(void)
{
    int status;

    uint16_t pin_index = PIN_1_IDX;
    enum mod_pinctrl_selector flags = MOD_PINCTRL_SELECTOR_PIN;
    const struct mod_pinctrl_drv_pin_configuration config = {
        .config_type = MOD_PINCTRL_DRV_TYPE_INPUT_VALUE,
        .config_value = DISABLE_VALUE,
    };

    status = set_configuration(pin_index, flags, &config);

    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
}

void utest_set_group_read_only_configuration(void)
{
    int status;

    uint16_t group_id = I2C_GROUP_IDX;
    enum mod_pinctrl_selector flags = MOD_PINCTRL_SELECTOR_GROUP;

    const struct mod_pinctrl_drv_pin_configuration configs = {
        .config_type = MOD_PINCTRL_DRV_TYPE_INPUT_VALUE,
        .config_value = ENABLE_VALUE,
    };

    status = set_configuration(group_id, flags, &configs);

    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
}

void utest_set_pin_function(void)
{
    int status;

    uint16_t pin_index = PIN_1_IDX;
    enum mod_pinctrl_selector flags = MOD_PINCTRL_SELECTOR_PIN;
    uint16_t function_id = GPIO_FUNC_IDX;

    set_pin_function_ExpectAndReturn(PIN_1_IDX, GPIO_FUNC_IDX, FWK_SUCCESS);

    status = set_function(pin_index, flags, function_id);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void utest_set_pin_function_is_not_pin_only_function(void)
{
    int status;

    uint16_t pin_index = PIN_1_IDX;
    enum mod_pinctrl_selector flags = MOD_PINCTRL_SELECTOR_PIN;
    uint16_t function_id = I2C_FUNC_IDX;

    status = set_function(pin_index, flags, function_id);

    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
}

void utest_set_group_function(void)
{
    int status;

    uint16_t group_index = I2C_GROUP_IDX;
    enum mod_pinctrl_selector flags = MOD_PINCTRL_SELECTOR_GROUP;
    uint16_t function_id = I2C_FUNC_IDX;

    set_pin_function_ExpectAndReturn(PIN_5_IDX, function_id, FWK_SUCCESS);
    set_pin_function_ExpectAndReturn(PIN_6_IDX, function_id, FWK_SUCCESS);

    status = set_function(group_index, flags, function_id);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void utest_set_group_function_pin_only_function_id(void)
{
    int status;

    uint16_t group_index = I2C_GROUP_IDX;
    enum mod_pinctrl_selector flags = MOD_PINCTRL_SELECTOR_GROUP;
    uint16_t function_id = GPIO_FUNC_IDX;

    status = set_function(group_index, flags, function_id);

    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
}

void utest_set_group_function_group_out_of_range(void)
{
    int status;

    uint16_t group_index = GROUP_COUNT;
    enum mod_pinctrl_selector flags = MOD_PINCTRL_SELECTOR_GROUP;
    uint16_t function_id = I2C_FUNC_IDX;

    status = set_function(group_index, flags, function_id);

    TEST_ASSERT_EQUAL(FWK_E_RANGE, status);
}

void utest_set_pin_function_with_not_allowed_function(void)
{
    int status;

    uint16_t pin_index = PIN_5_IDX;
    enum mod_pinctrl_selector flags = MOD_PINCTRL_SELECTOR_PIN;
    uint16_t function_id = GPIO_FUNC_IDX;

    status = set_function(pin_index, flags, function_id);

    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
}

int pinctrl_test_main(void)
{
    UNITY_BEGIN();

    RUN_TEST(utest_pinctrl_get_group_attributes);
    RUN_TEST(utest_pinctrl_get_function_attributes);
    RUN_TEST(utest_pinctrl_get_pin_only_fucntion_attributes);
    RUN_TEST(utest_pinctrl_get_pin_attributes);
    RUN_TEST(utest_pinctrl_get_attribute_invalid_flags);
    RUN_TEST(utest_pinctrl_get_attribute_invalid_group_id);
    RUN_TEST(utest_pinctrl_get_attribute_invalid_function_id);
    RUN_TEST(utest_pinctrl_get_attribute_invalid_pin_id);

    RUN_TEST(utest_get_info);

    RUN_TEST(utest_list_pins_associated_with_group);
    RUN_TEST(utest_list_pins_associated_with_group_out_of_range_indx);
    RUN_TEST(utest_list_groups_associated_with_function);
    RUN_TEST(utest_list_multiple_groups_associated_with_the_same_function);
    RUN_TEST(utest_list_groups_associated_with_function_out_of_range_indx);
    RUN_TEST(utest_list_pins_associated_with_function);
    RUN_TEST(utest_list_pins_associated_with_function_invalid_index);

    RUN_TEST(utest_get_total_number_of_pins_associated_with_group);
    RUN_TEST(utest_get_total_number_of_groups_associated_with_function);
    RUN_TEST(utest_get_total_number_of_pins_associated_with_function);

    RUN_TEST(utest_get_pin_configuration_value_from_type);
    RUN_TEST(utest_get_group_configuration_value_from_type);
    RUN_TEST(utest_get_configuration_value_from_type_invalid_flag);
    RUN_TEST(utest_get_configuration_value_from_type_invalid_group_id);
    RUN_TEST(utest_get_configuration_value_from_type_invalid_group_type);

    RUN_TEST(utest_get_group_configurations);
    RUN_TEST(utest_get_pin_configurations);
    RUN_TEST(utest_get_configurations_invalid_flags);

    RUN_TEST(utest_get_total_number_of_group_configurations);
    RUN_TEST(utest_get_total_number_of_configurations_invalid_group_id);
    RUN_TEST(utest_get_total_number_of_pin_configurations);
    RUN_TEST(utest_get_total_number_of_configurations_invalid_flags);

    RUN_TEST(utest_get_pin_associated_function);
    RUN_TEST(utest_get_group_associated_function);
    RUN_TEST(utest_get_group_associated_function_invalid_flag);

    RUN_TEST(utest_set_pin_configurations);
    RUN_TEST(utest_set_group_configuration);
    RUN_TEST(utest_set_group_configuration_group_id_invalid);
    RUN_TEST(utest_set_group_configuration_invalid_flag);
    RUN_TEST(utest_set_pin_read_only_configuration);
    RUN_TEST(utest_set_group_read_only_configuration);

    RUN_TEST(utest_set_pin_function);
    RUN_TEST(utest_set_pin_function_is_not_pin_only_function);
    RUN_TEST(utest_set_group_function);
    RUN_TEST(utest_set_group_function_pin_only_function_id);
    RUN_TEST(utest_set_group_function_group_out_of_range);
    RUN_TEST(utest_set_pin_function_with_not_allowed_function);

    return UNITY_END();
}

#if !defined(TEST_ON_TARGET)
int main(void)
{
    return pinctrl_test_main();
}
#endif
