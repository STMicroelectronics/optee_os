/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef CONFIG_PINCTRL_H
#define CONFIG_PINCTRL_H

#include <mod_pinctrl.h>

#include <stdlib.h>

#define DRIVE_OPEN_SOURCE_VALUE 7
#define LOW_POWER_MODE_VALUE    1
#define PIN_WITH_GPIO_FUNC      (PIN_COUNT - 2)
#define ENABLE_VALUE            1
#define DISABLE_VALUE           0

typedef enum {
    PIN_0_IDX = 0,
    PIN_1_IDX,
    PIN_2_IDX,
    PIN_3_IDX,
    PIN_4_IDX,
    PIN_5_IDX,
    PIN_6_IDX,
    PIN_COUNT,
} pin_idx;

typedef enum {
    GPIO_FUNC_IDX = 0,
    UART_FUNC_IDX,
    SPI_FUNC_IDX,
    I2C_FUNC_IDX,
    FUNC_COUNT,
} function_idx;

typedef enum {
    UART_GROUP_IDX = 0,
    SPI_GROUP_IDX,
    I2C_GROUP_IDX,
    GPIO_GROUP_IDX,
    UART_1_GROUP_IDX,
    GROUP_COUNT,
} group_idx;

const enum mod_pinctrl_drv_configuration_type pin_configuration[] = {
    MOD_PINCTRL_DRV_TYPE_DRIVE_OPEN_SOURCE,
    MOD_PINCTRL_DRV_TYPE_LOW_POWER_MODE,
    MOD_PINCTRL_DRV_TYPE_INPUT_VALUE
};

const enum mod_pinctrl_drv_configuration_type read_only_pin_configuration[] = {
    MOD_PINCTRL_DRV_TYPE_INPUT_VALUE
};

uint16_t pin_0_allowed_func[] = { GPIO_FUNC_IDX, UART_FUNC_IDX };
uint16_t pin_1_allowed_func[] = { GPIO_FUNC_IDX, UART_FUNC_IDX };
uint16_t pin_2_allowed_func[] = { GPIO_FUNC_IDX, SPI_FUNC_IDX };
uint16_t pin_3_allowed_func[] = { GPIO_FUNC_IDX, SPI_FUNC_IDX };
uint16_t pin_4_allowed_func[] = { GPIO_FUNC_IDX, SPI_FUNC_IDX };
uint16_t pin_5_allowed_func[] = { UART_FUNC_IDX, I2C_FUNC_IDX };
uint16_t pin_6_allowed_func[] = { UART_FUNC_IDX, I2C_FUNC_IDX };

struct mod_pinctrl_pin pins[] =
{
    [PIN_0_IDX] =
    {
        .name = "PIN_0",
        .allowed_functions = pin_0_allowed_func,
        .num_allowed_functions = FWK_ARRAY_SIZE(pin_0_allowed_func),
        .configuration = pin_configuration,
        .num_configuration = FWK_ARRAY_SIZE(pin_configuration),
        .read_only_configuration = read_only_pin_configuration,
        .num_of_read_only_configurations = FWK_ARRAY_SIZE(read_only_pin_configuration),
    },
    [PIN_1_IDX] =
    {
        .name = "PIN_1",
        .allowed_functions = pin_1_allowed_func,
        .num_allowed_functions = FWK_ARRAY_SIZE(pin_1_allowed_func),
        .configuration = pin_configuration,
        .num_configuration = FWK_ARRAY_SIZE(pin_configuration),
        .read_only_configuration = read_only_pin_configuration,
        .num_of_read_only_configurations = FWK_ARRAY_SIZE(read_only_pin_configuration),
    },
    [PIN_2_IDX] =
    {
        .name = "PIN_2",
        .allowed_functions = pin_2_allowed_func,
        .num_allowed_functions = FWK_ARRAY_SIZE(pin_2_allowed_func),
        .configuration = pin_configuration,
        .num_configuration = FWK_ARRAY_SIZE(pin_configuration),
    },
    [PIN_3_IDX] =
    {
        .name = "PIN_3",
        .allowed_functions = pin_3_allowed_func,
        .num_allowed_functions = FWK_ARRAY_SIZE(pin_3_allowed_func),
        .configuration = pin_configuration,
        .num_configuration = FWK_ARRAY_SIZE(pin_configuration),
        .read_only_configuration = read_only_pin_configuration,
        .num_of_read_only_configurations = FWK_ARRAY_SIZE(read_only_pin_configuration),
    },
    [PIN_4_IDX] =
    {
        .name = "PIN_4",
        .allowed_functions = pin_4_allowed_func,
        .num_allowed_functions = FWK_ARRAY_SIZE(pin_4_allowed_func),
        .configuration = pin_configuration,
        .num_configuration = FWK_ARRAY_SIZE(pin_configuration),
        .read_only_configuration = read_only_pin_configuration,
        .num_of_read_only_configurations = FWK_ARRAY_SIZE(read_only_pin_configuration),
    },
    [PIN_5_IDX] =
    {
        .name = "PIN_5",
        .allowed_functions = pin_5_allowed_func,
        .num_allowed_functions = FWK_ARRAY_SIZE(pin_5_allowed_func),
        .configuration = pin_configuration,
        .num_configuration = FWK_ARRAY_SIZE(pin_configuration),
        .read_only_configuration = read_only_pin_configuration,
        .num_of_read_only_configurations = FWK_ARRAY_SIZE(read_only_pin_configuration),
    },
    [PIN_6_IDX] =
    {
        .name = "PIN_6",
        .allowed_functions = pin_6_allowed_func,
        .num_allowed_functions = FWK_ARRAY_SIZE(pin_6_allowed_func),
        .configuration = pin_configuration,
        .num_configuration = FWK_ARRAY_SIZE(pin_configuration),
        .read_only_configuration = read_only_pin_configuration,
        .num_of_read_only_configurations = FWK_ARRAY_SIZE(read_only_pin_configuration),
    },
};

struct mod_pinctrl_function functions[] =
{
    [GPIO_FUNC_IDX] =
    {
        .name = "f_gpio_a",
        .is_gpio = false,
        .is_pin_only = true,
    },
    [UART_FUNC_IDX] =
    {
        .name = "f_uart_a",
        .is_gpio = false,
        .is_pin_only = false,
    },
    [SPI_FUNC_IDX] =
    {
        .name = "f_spi_a",
        .is_gpio = false,
        .is_pin_only = false,
    },
    [I2C_FUNC_IDX] =
    {
        .name = "f_i2c_a",
        .is_gpio = false,
        .is_pin_only = false,
    },
};

const uint16_t grp_uart_pin_idx[] = { PIN_0_IDX, PIN_1_IDX };
const uint16_t grp_uart_1_pin_idx[] = { PIN_5_IDX, PIN_6_IDX };
const uint16_t grp_spi_pin_idx[] = { PIN_2_IDX, PIN_3_IDX, PIN_4_IDX };
const uint16_t grp_i2c_pin_idx[] = { PIN_5_IDX, PIN_6_IDX };
const uint16_t grp_gpio_pin_idx[] = { PIN_0_IDX, PIN_1_IDX, PIN_2_IDX,
                                      PIN_3_IDX, PIN_4_IDX, PIN_5_IDX };

struct mod_pinctrl_group groups[] =
{
    [UART_GROUP_IDX] =
    {
        .name = "grp_uart_a",
        .is_gpio = false,
        .function = UART_FUNC_IDX,
        .pins = grp_uart_pin_idx,
        .num_pins = FWK_ARRAY_SIZE(grp_uart_pin_idx),
    },
    [SPI_GROUP_IDX] =
    {
        .name = "grp_spi_a",
        .is_gpio = false,
        .function = SPI_FUNC_IDX,
        .pins = grp_spi_pin_idx,
        .num_pins = FWK_ARRAY_SIZE(grp_spi_pin_idx),
    },
    [I2C_GROUP_IDX] =
    {
        .name = "grp_i2c_a",
        .is_gpio = false,
        .function = I2C_FUNC_IDX,
        .pins = grp_i2c_pin_idx,
        .num_pins = FWK_ARRAY_SIZE(grp_i2c_pin_idx),
    },
    [GPIO_GROUP_IDX] =
    {
        .name = "grp_gpio",
        .is_gpio = true,
        .function = GPIO_FUNC_IDX,
        .pins = grp_gpio_pin_idx,
        .num_pins = FWK_ARRAY_SIZE(grp_gpio_pin_idx),
    },
    [UART_1_GROUP_IDX] =
    {
        .name = "grp_uart_1",
        .is_gpio = false,
        .function = UART_FUNC_IDX,
        .pins = grp_uart_1_pin_idx,
        .num_pins = FWK_ARRAY_SIZE(grp_uart_1_pin_idx),
    },
};

struct mod_pinctrl_config config = {
    .functions_table = functions,
    .function_table_count = FWK_ARRAY_SIZE(functions),
    .groups_table = groups,
    .group_table_count = FWK_ARRAY_SIZE(groups),
    .pins_table = pins,
    .pin_table_count = FWK_ARRAY_SIZE(pins),
};

#endif /* CONFIG_PINCTRL_H */
