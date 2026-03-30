/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *      SCMI Pin Control Protocol.
 */

#ifndef INTERNAL_SCMI_PIN_CONTROL_H
#define INTERNAL_SCMI_PIN_CONTROL_H

#include <mod_pinctrl.h>

#include <fwk_macros.h>

#include <stdint.h>

#define SCMI_PROTOCOL_VERSION_PIN_CONTROL UINT32_C(0x10000)

#define SCMI_PIN_CONTROL_EXTENDED_NAME_LENGTH_MAX 64
#define SCMI_PIN_CONTROL_NAME_LENGTH_MAX          16

#define SCMI_PIN_CONTROL_MAX_IDENTIFIER 0xFFFF
#define SCMI_PIN_CONTROL_MIN_IDENTIFIER 0

#define NUM_OF_PIN_GROUPS_POS         16
#define NUM_OF_REMAINING_ELEMENTS_POS 16

#define SHIFT_LEFT_BY_POS(X, POS)       (X << POS)
#define GET_VALUE_FROM_POS(X, POS, MSB) ((X & FWK_GEN_MASK(MSB, POS)) >> POS)
#define NO_FUNCTION_IS_SELECTED         0xFFFFFFFFU

#define SCMI_PIN_CONTROL_PIN_ONLY_FUNC_DESCRIPTOR_POS 16
#define SCMI_PIN_CONTROL_GPIO_FUNC_ONLY_POS           17
#define SCMI_PIN_CONTROL_EXTENDED_NAME_POS            31
#define SCMI_PIN_CONTROL_NAME_IS_EXTENDED             1

#define SCMI_PIN_CONTROL_CONFIG_FLAG_POS      18
#define SCMI_PIN_CONTROL_CONFIG_FLAG_POS_MSB  19
#define SCMI_PIN_CONTROL_SELECTOR_POS         16
#define SCMI_PIN_CONTROL_SELECTOR_POS_MSB     17
#define SCMI_PIN_CONTROL_SKIP_CONFIGS_POS     8
#define SCMI_PIN_CONTROL_SKIP_CONFIGS_POS_MSB 15
#define SCMI_PIN_CONTROL_CONFIG_TYPE_POS      0
#define SCMI_PIN_CONTROL_CONFIG_TYPE_POS_MSB  7

#define SCMI_PIN_CONTROL_REMAINING_CONFIGS_POS 16

#define SCMI_PIN_CONTROL_CONFIG_VALUE_SELECTED_TYPE 0
#define SCMI_PIN_CONTROL_ALL_CONFIG_VALUES          1
#define SCMI_PIN_CONTROL_FUNCTION_SELECTED          2

#define SCMI_PIN_CONTROL_FUNCTION_ID_VALID_POS     10
#define SCMI_PIN_CONTROL_NUM_OF_CONFIGS_POS        2
#define SCMI_PIN_CONTROL_NUM_OF_CONFIGS_POS_MSB    9
#define SCMI_PIN_CONTROL_SET_CONF_SELECTOR_POS     0
#define SCMI_PIN_CONTROL_SET_CONF_SELECTOR_POS_MSB 1

#define SCMI_PIN_CONTROL_FUNCTION_ID_INVALID 0
#define SCMI_PIN_CONTROL_FUNCTION_ID_VALID   1

#define SCMI_PIN_CONTROL_IDENTIFIER_POS_HIGH 15
#define SCMI_PIN_CONTROL_IDENTIFIER_POS_LOW  0

struct scmi_pin_control_protocol_attributes_p2a {
    int32_t status;
    uint32_t attributes_low;
    uint32_t attributes_high;
};

struct scmi_pin_control_attributes_a2p {
    uint32_t identifier;
    uint32_t flags;
};

struct scmi_pin_control_attributes_p2a {
    int32_t status;
    uint32_t attributes;
    char name[SCMI_PIN_CONTROL_NAME_LENGTH_MAX];
};

struct scmi_pin_control_list_associations_a2p {
    uint32_t identifier;
    uint32_t flags;
    uint32_t index;
};

struct scmi_pin_control_list_associations_p2a {
    int32_t status;
    uint32_t flags;
    uint16_t payload[];
};

struct scmi_pin_control_settings_get_a2p {
    uint32_t identifier;
    uint32_t attributes;
};

struct scmi_pin_control_settings_get_p2a {
    int32_t status;
    uint32_t function_selected;
    uint32_t num_configs;
    struct mod_pinctrl_drv_pin_configuration config_pair[];
};

struct scmi_pin_control_settings_configure_a2p {
    uint32_t identifier;
    uint32_t function_id;
    uint32_t attributes;
};

struct scmi_pin_control_settings_configure_p2a {
    int32_t status;
};

struct scmi_pin_control_request_a2p {
    uint32_t identifier;
    uint32_t flags;
};

struct scmi_pin_control_request_p2a {
    int32_t status;
};

struct scmi_pin_control_release_a2p {
    uint32_t identifier;
    uint32_t flags;
};

struct scmi_pin_control_release_p2a {
    int32_t status;
};

struct scmi_pin_control_name_get_a2p {
    uint32_t identifier;
    uint32_t flags;
};

struct scmi_pin_control_name_get_p2a {
    int32_t status;
    uint32_t flags;
    char name[SCMI_PIN_CONTROL_EXTENDED_NAME_LENGTH_MAX];
};

struct scmi_pin_control_set_permissions_a2p {
    uint32_t agent_id;
    uint32_t identifier;
    uint32_t flags;
};

struct scmi_pin_control_set_permissions_p2a {
    int32_t status;
};

struct scmi_pin_control_setting_get_attributes {
    uint8_t config_flag;
    uint8_t selector;
    uint8_t skip_configs;
    uint8_t config_type;
};

#endif /* INTERNAL_SCMI_PIN_CONTROL_H */
