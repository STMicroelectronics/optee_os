/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *      SCMI Pin Control Protocol.
 */
#include <internal/scmi_pin_control.h>

#include <mod_scmi.h>
#include <mod_scmi_pin_control.h>

#include <fwk_event.h>
#include <fwk_log.h>
#include <fwk_module.h>

#include <stdint.h>
#include <string.h>

struct mod_scmi_pin_control_ctx {
    /* SCMI Configuration Domain */
    const struct mod_scmi_pinctrl_domain_config *config_domain_table;

    /* SCMI module API */
    const struct mod_scmi_from_protocol_api *scmi_api;

    /* Pin Control module API */
    const struct mod_pinctrl_api *pinctrl_api;
};

/*
 * SCMI Pin Control Message Handlers
 */
static int scmi_pin_control_protocol_version_handler(
    fwk_id_t service_id,
    const uint32_t *payload);
static int scmi_pin_control_protocol_attributes_handler(
    fwk_id_t service_id,
    const uint32_t *payload);
static int scmi_pin_control_protocol_message_attributes_handler(
    fwk_id_t service_id,
    const uint32_t *payload);
static int scmi_pin_control_attributes_handler(
    fwk_id_t service_id,
    const uint32_t *payload);
static int scmi_pin_control_list_associations_handler(
    fwk_id_t service_id,
    const uint32_t *payload);
static int scmi_pin_control_settings_get_handler(
    fwk_id_t service_id,
    const uint32_t *payload);
static int scmi_pin_control_settings_configure_handler(
    fwk_id_t service_id,
    const uint32_t *payload);
static int scmi_pin_control_name_get_handler(
    fwk_id_t service_id,
    const uint32_t *payload);
static int scmi_pin_control_req_message_not_supported(
    fwk_id_t service_id,
    const uint32_t *payload);

/*
 * Internal variables.
 */

static struct mod_scmi_pin_control_ctx scmi_pin_control_ctx;

static handler_table_t handler_table[MOD_SCMI_PIN_CONTROL_COMMAND_COUNT] = {
    [MOD_SCMI_PROTOCOL_VERSION] = scmi_pin_control_protocol_version_handler,
    [MOD_SCMI_PROTOCOL_ATTRIBUTES] =
        scmi_pin_control_protocol_attributes_handler,
    [MOD_SCMI_PROTOCOL_MESSAGE_ATTRIBUTES] =
        scmi_pin_control_protocol_message_attributes_handler,
    [MOD_SCMI_PIN_CONTROL_ATTRIBUTES] = scmi_pin_control_attributes_handler,
    [MOD_SCMI_PIN_CONTROL_LIST_ASSOCIATIONS] =
        scmi_pin_control_list_associations_handler,
    [MOD_SCMI_PIN_CONTROL_SETTINGS_GET] = scmi_pin_control_settings_get_handler,
    [MOD_SCMI_PIN_CONTROL_SETTINGS_CONFIGURE] =
        scmi_pin_control_settings_configure_handler,
    [MOD_SCMI_PIN_CONTROL_REQUEST] = scmi_pin_control_req_message_not_supported,
    [MOD_SCMI_PIN_CONTROL_RELEASE] = scmi_pin_control_req_message_not_supported,
    [MOD_SCMI_PIN_CONTROL_NAME_GET] = scmi_pin_control_name_get_handler,
    [MOD_SCMI_PIN_CONTROL_SET_PERMISSIONS] =
        scmi_pin_control_req_message_not_supported,
};

static const size_t payload_size_table[MOD_SCMI_PIN_CONTROL_COMMAND_COUNT] = {
    [MOD_SCMI_PROTOCOL_VERSION] = 0,
    [MOD_SCMI_PROTOCOL_ATTRIBUTES] = 0,
    [MOD_SCMI_PROTOCOL_MESSAGE_ATTRIBUTES] =
        (unsigned int)sizeof(struct scmi_protocol_message_attributes_a2p),
    [MOD_SCMI_PIN_CONTROL_ATTRIBUTES] =
        (unsigned int)sizeof(struct scmi_pin_control_attributes_a2p),
    [MOD_SCMI_PIN_CONTROL_LIST_ASSOCIATIONS] =
        (unsigned int)sizeof(struct scmi_pin_control_list_associations_a2p),
    [MOD_SCMI_PIN_CONTROL_SETTINGS_GET] =
        (unsigned int)sizeof(struct scmi_pin_control_settings_get_a2p),
    [MOD_SCMI_PIN_CONTROL_SETTINGS_CONFIGURE] =
        MOD_SCMI_PROTOCOL_DAYNAMIC_PAYLOAD_SIZE,
    [MOD_SCMI_PIN_CONTROL_REQUEST] =
        (unsigned int)sizeof(struct scmi_pin_control_request_a2p),
    [MOD_SCMI_PIN_CONTROL_RELEASE] =
        (unsigned int)sizeof(struct scmi_pin_control_release_a2p),
    [MOD_SCMI_PIN_CONTROL_NAME_GET] =
        (unsigned int)sizeof(struct scmi_pin_control_name_get_a2p),
    [MOD_SCMI_PIN_CONTROL_SET_PERMISSIONS] =
        (unsigned int)sizeof(struct scmi_pin_control_set_permissions_a2p),
};

static int scmi_pin_control_req_message_not_supported(
    fwk_id_t service_id,
    const uint32_t *payload)
{
    int32_t not_supported_message = (int32_t)SCMI_NOT_SUPPORTED;
    return scmi_pin_control_ctx.scmi_api->respond(
        service_id, &not_supported_message, sizeof(not_supported_message));
}

static int validate_setting_config_payload_size(
    size_t payload_size,
    const uint32_t *payload)
{
    const struct scmi_pin_control_settings_configure_a2p *parameters;
    uint8_t num_of_configs;
    uint32_t attributes;
    size_t calculated_payload_size;

    parameters =
        (const struct scmi_pin_control_settings_configure_a2p *)payload;
    attributes = parameters->attributes;

    num_of_configs = (uint8_t)GET_VALUE_FROM_POS(
        attributes,
        SCMI_PIN_CONTROL_NUM_OF_CONFIGS_POS,
        SCMI_PIN_CONTROL_NUM_OF_CONFIGS_POS_MSB);

    calculated_payload_size =
        sizeof(struct scmi_pin_control_settings_configure_a2p) +
        (num_of_configs * sizeof(uint64_t));

    if (calculated_payload_size != payload_size) {
        return SCMI_PROTOCOL_ERROR;
    }

    return SCMI_SUCCESS;
}

static int map_identifier(int32_t identifier, uint16_t *mapped_identifier)
{
    size_t i;
    const struct mod_scmi_pinctrl_domain_config *domains =
        scmi_pin_control_ctx.config_domain_table;
    int32_t calculated_identifier;

    identifier &= UINT16_MAX;

    for (i = 0; i < domains->config_domain_count; ++i) {
        if ((identifier >= domains->config_domains[i].identifier_begin) &&
            (identifier <= domains->config_domains[i].identifier_end)) {
            calculated_identifier =
                identifier + domains->config_domains[i].shift_factor;

            if ((calculated_identifier > SCMI_PIN_CONTROL_MAX_IDENTIFIER) ||
                (calculated_identifier < SCMI_PIN_CONTROL_MIN_IDENTIFIER)) {
                return FWK_E_RANGE;
            }
            *mapped_identifier = calculated_identifier;
            return FWK_SUCCESS;
        }
    }

    return FWK_E_RANGE;
}

static size_t number_of_elements_allowed_in_payload(
    fwk_id_t service_id,
    size_t respond_header_size,
    size_t size_of_payload_element,
    size_t *number_of_elements)
{
    size_t max_payload_size;
    int status;
    /*
     * Get the maximum payload size to determine how many associations
     * entries can be returned in one response.
     */
    status = scmi_pin_control_ctx.scmi_api->get_max_payload_size(
        service_id, &max_payload_size);
    if (status != FWK_SUCCESS) {
        return status;
    }

    if (respond_header_size > max_payload_size) {
        *number_of_elements = 0;
        return FWK_E_SIZE;
    }

    *number_of_elements =
        (max_payload_size - respond_header_size) / size_of_payload_element;
    return FWK_SUCCESS;
}

static int scmi_pin_control_protocol_version_handler(
    fwk_id_t service_id,
    const uint32_t *payload)
{
    struct scmi_protocol_version_p2a return_values = {
        .status = (int32_t)SCMI_SUCCESS,
        .version = SCMI_PROTOCOL_VERSION_PIN_CONTROL,
    };

    return scmi_pin_control_ctx.scmi_api->respond(
        service_id, &return_values, sizeof(return_values));
}

static int scmi_pin_control_protocol_attributes_handler(
    fwk_id_t service_id,
    const uint32_t *payload)
{
    size_t response_size;
    struct scmi_pin_control_protocol_attributes_p2a return_values = {
        .status = SCMI_GENERIC_ERROR,
        .attributes_low = 0,
        .attributes_high = 0,
    };
    struct mod_pinctrl_info protocol_attributes;

    int status =
        scmi_pin_control_ctx.pinctrl_api->get_info(&protocol_attributes);

    if (status == FWK_SUCCESS) {
        return_values.status = (int32_t)SCMI_SUCCESS;
        return_values.attributes_low = protocol_attributes.number_of_pins;
        return_values.attributes_low |= (uint32_t)SHIFT_LEFT_BY_POS(
            protocol_attributes.number_of_groups, NUM_OF_PIN_GROUPS_POS);
        return_values.attributes_high = protocol_attributes.number_of_functions;
    } else {
        return_values.status = (int32_t)SCMI_GENERIC_ERROR;
    }

    response_size = (return_values.status == SCMI_SUCCESS) ?
        sizeof(return_values) :
        sizeof(return_values.status);
    return scmi_pin_control_ctx.scmi_api->respond(
        service_id, &return_values, response_size);
}

static int scmi_pin_control_protocol_message_attributes_handler(
    fwk_id_t service_id,
    const uint32_t *payload)
{
    size_t response_size;
    const struct scmi_protocol_message_attributes_a2p *parameters;
    uint32_t message_id;
    struct scmi_protocol_message_attributes_p2a return_values = {
        .status = (int32_t)SCMI_SUCCESS,
        .attributes = 0,
    };

    parameters = (const struct scmi_protocol_message_attributes_a2p *)payload;
    message_id = parameters->message_id;

    if ((message_id >= FWK_ARRAY_SIZE(handler_table)) ||
        (handler_table[message_id] ==
         scmi_pin_control_req_message_not_supported)) {
        return_values.status = (int32_t)SCMI_NOT_FOUND;
    }

    response_size = (return_values.status == SCMI_SUCCESS) ?
        sizeof(return_values) :
        sizeof(return_values.status);

    return scmi_pin_control_ctx.scmi_api->respond(
        service_id, &return_values, response_size);
}

static int scmi_pin_control_attributes_handler(
    fwk_id_t service_id,
    const uint32_t *payload)
{
    const struct scmi_pin_control_attributes_a2p *parameters;
    size_t name_length = 1U;
    struct mod_pinctrl_attributes pinctrl_attributes;
    uint16_t mapped_identifier;
    int status;

    struct scmi_pin_control_attributes_p2a return_values = {
        .status = (int32_t)SCMI_GENERIC_ERROR,
        .attributes = 0,
        .name = { 0 },
    };

    parameters = (const struct scmi_pin_control_attributes_a2p *)payload;

    status = map_identifier(parameters->identifier, &mapped_identifier);

    if (status != FWK_SUCCESS) {
        goto exit;
    }

    status = scmi_pin_control_ctx.pinctrl_api->get_attributes(
        mapped_identifier, parameters->flags, &pinctrl_attributes);

    if (status == FWK_SUCCESS) {
        return_values.status = (int32_t)SCMI_SUCCESS;
        return_values.attributes = pinctrl_attributes.number_of_elements;

        return_values.attributes |= (uint32_t)SHIFT_LEFT_BY_POS(
            pinctrl_attributes.is_pin_only_function,
            SCMI_PIN_CONTROL_PIN_ONLY_FUNC_DESCRIPTOR_POS);

        return_values.attributes |= (uint32_t)SHIFT_LEFT_BY_POS(
            pinctrl_attributes.is_gpio_function,
            SCMI_PIN_CONTROL_GPIO_FUNC_ONLY_POS);

        name_length += strnlen(
            pinctrl_attributes.name,
            SCMI_PIN_CONTROL_EXTENDED_NAME_LENGTH_MAX - 1);
        if (name_length > SCMI_PIN_CONTROL_NAME_LENGTH_MAX) {
            return_values.attributes |= (uint32_t)SHIFT_LEFT_BY_POS(
                SCMI_PIN_CONTROL_NAME_IS_EXTENDED,
                SCMI_PIN_CONTROL_EXTENDED_NAME_POS);
            strncpy(
                return_values.name,
                pinctrl_attributes.name,
                SCMI_PIN_CONTROL_NAME_LENGTH_MAX - 1U);
        } else {
            strncpy(return_values.name, pinctrl_attributes.name, name_length);
        }

        return scmi_pin_control_ctx.scmi_api->respond(
            service_id, &return_values, sizeof(return_values));
    }

exit:
    return_values.status = (int32_t)SCMI_NOT_FOUND;
    return scmi_pin_control_ctx.scmi_api->respond(
        service_id, &return_values.status, sizeof(return_values.status));
}

static int scmi_pin_control_list_associations_handler(
    fwk_id_t service_id,
    const uint32_t *payload)
{
    const struct scmi_pin_control_list_associations_a2p *parameters;
    uint32_t payload_size;
    uint16_t identifiers_count;
    uint16_t total_number_of_associations;
    uint16_t identifier_index;
    size_t buffer_allowed_identifiers;
    uint16_t mapped_identifier;

    int status;

    struct scmi_pin_control_list_associations_p2a return_values = {
        .status = (int32_t)SCMI_GENERIC_ERROR,
        .flags = 0,
    };

    payload_size = (uint32_t)sizeof(return_values);

    parameters = (const struct scmi_pin_control_list_associations_a2p *)payload;

    status = map_identifier(parameters->identifier, &mapped_identifier);

    if (status != FWK_SUCCESS) {
        return_values.status = SCMI_NOT_FOUND;
        goto exit;
    }

    status = scmi_pin_control_ctx.pinctrl_api->get_total_number_of_associations(
        mapped_identifier, parameters->flags, &total_number_of_associations);
    if (status != FWK_SUCCESS) {
        return_values.status = SCMI_NOT_FOUND;
        goto exit;
    }

    status = number_of_elements_allowed_in_payload(
        service_id,
        sizeof(struct scmi_pin_control_list_associations_p2a),
        sizeof(uint16_t),
        (size_t *)&buffer_allowed_identifiers);
    if (status != FWK_SUCCESS) {
        goto exit;
    }

    identifiers_count = (uint16_t)FWK_MIN(
        buffer_allowed_identifiers,
        (uint16_t)(total_number_of_associations - parameters->index));

    return_values.flags = identifiers_count;
    return_values.flags |= SHIFT_LEFT_BY_POS(
        (total_number_of_associations - parameters->index - identifiers_count),
        NUM_OF_REMAINING_ELEMENTS_POS);

    for (identifier_index = 0; identifier_index < identifiers_count;
         identifier_index++) {
        uint16_t object_id;

        status = scmi_pin_control_ctx.pinctrl_api->get_list_associations(
            mapped_identifier,
            parameters->flags,
            (parameters->index + identifier_index),
            &object_id);
        if (status != FWK_SUCCESS) {
            return_values.status = SCMI_NOT_FOUND;
            goto exit;
        }

        status = scmi_pin_control_ctx.scmi_api->write_payload(
            service_id, payload_size, &object_id, sizeof(object_id));
        if (status != FWK_SUCCESS) {
            goto exit;
        }

        payload_size += (uint32_t)sizeof(object_id);
    }

    return_values.status = (int32_t)SCMI_SUCCESS;
    status = scmi_pin_control_ctx.scmi_api->write_payload(
        service_id, 0, &return_values, sizeof(return_values));
    if (status != FWK_SUCCESS) {
        return_values.status = SCMI_GENERIC_ERROR;
    }

exit:
    return scmi_pin_control_ctx.scmi_api->respond(
        service_id,
        (return_values.status == SCMI_SUCCESS) ? NULL :
                                                 (void *)&return_values.status,
        (return_values.status == SCMI_SUCCESS) ? payload_size :
                                                 sizeof(return_values.status));
}

static int get_all_configs(
    fwk_id_t service_id,
    const uint32_t identifier,
    const struct scmi_pin_control_setting_get_attributes *attribute,
    uint32_t *num_configs,
    uint32_t *payload_size)
{
    uint16_t total_no_configurations;
    size_t buffer_allowed_identifiers;
    uint16_t config_count;
    uint16_t config_index;
    struct mod_pinctrl_drv_pin_configuration config_pair;
    int status;

    status =
        scmi_pin_control_ctx.pinctrl_api->get_total_number_of_configurations(
            identifier, attribute->selector, &total_no_configurations);

    if (status != FWK_SUCCESS) {
        return status;
    }

    status = number_of_elements_allowed_in_payload(
        service_id,
        sizeof(struct scmi_pin_control_settings_get_p2a),
        sizeof(struct mod_pinctrl_drv_pin_configuration),
        (size_t *)&buffer_allowed_identifiers);

    if (status != FWK_SUCCESS) {
        return status;
    }

    config_count = (uint16_t)FWK_MIN(
        buffer_allowed_identifiers,
        (uint16_t)(total_no_configurations - attribute->skip_configs));

    *num_configs = config_count;
    *num_configs |= (uint32_t)SHIFT_LEFT_BY_POS(
        (total_no_configurations - attribute->skip_configs - config_count),
        SCMI_PIN_CONTROL_REMAINING_CONFIGS_POS);

    for (config_index = 0; config_index < config_count; config_index++) {
        status = scmi_pin_control_ctx.pinctrl_api->get_configuration(
            identifier,
            attribute->selector,
            (attribute->skip_configs + config_index),
            &config_pair);
        if (status != FWK_SUCCESS) {
            return status;
        }

        status = scmi_pin_control_ctx.scmi_api->write_payload(
            service_id, *payload_size, &config_pair, sizeof(config_pair));
        if (status != FWK_SUCCESS) {
            return status;
        }

        *payload_size +=
            (uint32_t)sizeof(struct mod_pinctrl_drv_pin_configuration);
    }

    return FWK_SUCCESS;
}

static void pinctrl_setting_get_attributes(
    const uint32_t attribute,
    struct scmi_pin_control_setting_get_attributes *attributes)
{
    attributes->config_flag = (uint8_t)GET_VALUE_FROM_POS(
        attribute,
        SCMI_PIN_CONTROL_CONFIG_FLAG_POS,
        SCMI_PIN_CONTROL_CONFIG_FLAG_POS_MSB);

    attributes->selector = (uint8_t)GET_VALUE_FROM_POS(
        attribute,
        SCMI_PIN_CONTROL_SELECTOR_POS,
        SCMI_PIN_CONTROL_SELECTOR_POS_MSB);

    attributes->skip_configs = (uint8_t)GET_VALUE_FROM_POS(
        attribute,
        SCMI_PIN_CONTROL_SKIP_CONFIGS_POS,
        SCMI_PIN_CONTROL_SKIP_CONFIGS_POS_MSB);

    attributes->config_type = (uint8_t)GET_VALUE_FROM_POS(
        attribute,
        SCMI_PIN_CONTROL_CONFIG_TYPE_POS,
        SCMI_PIN_CONTROL_CONFIG_TYPE_POS_MSB);
}

static int scmi_pin_control_settings_get_handler(
    fwk_id_t service_id,
    const uint32_t *payload)
{
    uint32_t payload_size;
    int16_t status;
    uint32_t function_selected;
    struct mod_pinctrl_drv_pin_configuration config_pair;
    const struct scmi_pin_control_settings_get_a2p *parameters;
    struct scmi_pin_control_setting_get_attributes attributes;
    struct scmi_pin_control_settings_get_p2a return_values = {
        .status = (int32_t)SCMI_GENERIC_ERROR,
        .function_selected = 0,
        .num_configs = 0,
    };
    uint16_t mapped_identifier;
    payload_size = (uint32_t)sizeof(return_values);
    parameters = (const struct scmi_pin_control_settings_get_a2p *)payload;

    status = map_identifier(parameters->identifier, &mapped_identifier);
    if (status != FWK_SUCCESS) {
        goto exit;
    }

    pinctrl_setting_get_attributes(parameters->attributes, &attributes);

    switch (attributes.config_flag) {
    case SCMI_PIN_CONTROL_CONFIG_VALUE_SELECTED_TYPE:
        return_values.num_configs = 1;
        config_pair.config_type = attributes.config_type;
        status =
            scmi_pin_control_ctx.pinctrl_api->get_configuration_value_from_type(
                mapped_identifier,
                (enum mod_pinctrl_selector)attributes.selector,
                config_pair.config_type,
                &config_pair.config_value);
        if (status != FWK_SUCCESS) {
            goto exit;
        }
        status = scmi_pin_control_ctx.scmi_api->write_payload(
            service_id, payload_size, &config_pair, sizeof(config_pair));
        if (status != FWK_SUCCESS) {
            goto exit;
        }
        payload_size +=
            (uint32_t)sizeof(struct mod_pinctrl_drv_pin_configuration);
        break;

    case SCMI_PIN_CONTROL_ALL_CONFIG_VALUES:
        status = get_all_configs(
            service_id,
            mapped_identifier,
            &attributes,
            &return_values.num_configs,
            &payload_size);
        break;

    case SCMI_PIN_CONTROL_FUNCTION_SELECTED:
        status =
            scmi_pin_control_ctx.pinctrl_api->get_current_associated_function(
                mapped_identifier, attributes.selector, &function_selected);

        if (status == FWK_SUCCESS) {
            return_values.function_selected = function_selected;
        } else {
            goto exit;
        }
        break;
    }

    return_values.status = (int32_t)SCMI_SUCCESS;
    status = scmi_pin_control_ctx.scmi_api->write_payload(
        service_id, 0, &return_values, sizeof(return_values));
    if (status != FWK_SUCCESS) {
        return_values.status = SCMI_GENERIC_ERROR;
    }

exit:
    if (status == FWK_E_RANGE) {
        return_values.status = (int32_t)SCMI_NOT_FOUND;
    } else if (status == FWK_E_PARAM) {
        return_values.status = (int32_t)SCMI_INVALID_PARAMETERS;
    }

    return scmi_pin_control_ctx.scmi_api->respond(
        service_id,
        (return_values.status == SCMI_SUCCESS) ? NULL :
                                                 (void *)&return_values.status,
        (return_values.status == SCMI_SUCCESS) ? payload_size :
                                                 sizeof(return_values.status));
}

static int scmi_pin_control_settings_configure_handler(
    fwk_id_t service_id,
    const uint32_t *payload)
{
    const struct scmi_pin_control_settings_configure_a2p *parameters;
    uint16_t mapped_identifier;
    uint16_t function_id;
    uint32_t attributes;
    uint8_t function_id_valid;
    uint8_t num_of_configs;
    uint8_t selector;
    int status;
    const struct mod_pinctrl_drv_pin_configuration *config_base;

    struct scmi_pin_control_settings_configure_p2a return_values = {
        .status = (int32_t)SCMI_GENERIC_ERROR,
    };

    parameters =
        (const struct scmi_pin_control_settings_configure_a2p *)payload;

    status = map_identifier(parameters->identifier, &mapped_identifier);
    if (status != FWK_SUCCESS) {
        goto exit;
    }

    function_id = (uint16_t)parameters->function_id;
    attributes = parameters->attributes;

    function_id_valid = (uint8_t)GET_VALUE_FROM_POS(
        attributes,
        SCMI_PIN_CONTROL_FUNCTION_ID_VALID_POS,
        SCMI_PIN_CONTROL_FUNCTION_ID_VALID_POS);
    num_of_configs = (uint8_t)GET_VALUE_FROM_POS(
        attributes,
        SCMI_PIN_CONTROL_NUM_OF_CONFIGS_POS,
        SCMI_PIN_CONTROL_NUM_OF_CONFIGS_POS_MSB);
    selector = (uint8_t)GET_VALUE_FROM_POS(
        attributes,
        SCMI_PIN_CONTROL_SET_CONF_SELECTOR_POS,
        SCMI_PIN_CONTROL_SET_CONF_SELECTOR_POS_MSB);

    if (function_id_valid == SCMI_PIN_CONTROL_FUNCTION_ID_VALID) {
        status = scmi_pin_control_ctx.pinctrl_api->set_function(
            mapped_identifier, selector, function_id);
        if (status != FWK_SUCCESS) {
            goto exit;
        }
    }

    /* Start the pointer at the first configuration */
    config_base =
        (const struct mod_pinctrl_drv_pin_configuration
             *)(payload + (sizeof(struct scmi_pin_control_settings_configure_a2p) / sizeof(*payload)));
    for (size_t config_index = 0U; config_index < num_of_configs;
         ++config_index) {
        /* Access each configuration directly by index */
        const struct mod_pinctrl_drv_pin_configuration config_pair =
            config_base[config_index];
        status = scmi_pin_control_ctx.pinctrl_api->set_configuration(
            mapped_identifier, selector, &config_pair);
        if (status != FWK_SUCCESS) {
            goto exit;
        }
    }
    return_values.status = (int32_t)SCMI_SUCCESS;
exit:
    if (status == FWK_E_RANGE) {
        return_values.status = (int32_t)SCMI_NOT_FOUND;
    } else if (status == FWK_E_PARAM) {
        return_values.status = (int32_t)SCMI_INVALID_PARAMETERS;
    }

    return scmi_pin_control_ctx.scmi_api->respond(
        service_id, &return_values, sizeof(return_values));
}

static int scmi_pin_control_name_get_handler(
    fwk_id_t service_id,
    const uint32_t *payload)
{
    size_t response_size;
    const struct scmi_pin_control_name_get_a2p *parameters;
    struct mod_pinctrl_attributes pinctrl_attributes;
    /* Put an initial value of one to represent the NULL space in the name */
    size_t name_length = 1U;
    int status;
    uint16_t mapped_identifier;

    struct scmi_pin_control_name_get_p2a return_values = {
        .status = (int32_t)SCMI_GENERIC_ERROR,
        .flags = 0,
        .name = { 0 },
    };

    parameters = (const struct scmi_pin_control_name_get_a2p *)payload;

    status = map_identifier(parameters->identifier, &mapped_identifier);
    if (status != FWK_SUCCESS) {
        return_values.status = (int32_t)SCMI_NOT_FOUND;
        goto exit;
    }

    status = scmi_pin_control_ctx.pinctrl_api->get_attributes(
        mapped_identifier, parameters->flags, &pinctrl_attributes);

    if (status != FWK_SUCCESS) {
        goto exit;
    }

    name_length += strnlen(
        pinctrl_attributes.name, SCMI_PIN_CONTROL_EXTENDED_NAME_LENGTH_MAX - 1);
    if (name_length <= SCMI_PIN_CONTROL_NAME_LENGTH_MAX) {
        return_values.status = (int32_t)SCMI_NOT_FOUND;
        goto exit;
    }

    return_values.status = (int32_t)SCMI_SUCCESS;
    strncpy(return_values.name, pinctrl_attributes.name, name_length);

exit:
    response_size = (return_values.status == SCMI_SUCCESS) ?
        sizeof(return_values) :
        sizeof(return_values.status);
    return scmi_pin_control_ctx.scmi_api->respond(
        service_id, &return_values, response_size);
}

/*
 * SCMI module -> SCMI pin control module interface
 */
static int scmi_pin_control_get_scmi_protocol_id(
    fwk_id_t protocol_id,
    uint8_t *scmi_protocol_id)
{
    *scmi_protocol_id = (uint8_t)MOD_SCMI_PROTOCOL_ID_PIN_CONTROL;

    return FWK_SUCCESS;
}

static int scmi_pin_control_message_handler(
    fwk_id_t protocol_id,
    fwk_id_t service_id,
    const uint32_t *payload,
    size_t payload_size,
    unsigned int message_id)
{
    int validation_result;

    static_assert(
        FWK_ARRAY_SIZE(handler_table) == FWK_ARRAY_SIZE(payload_size_table),
        "[SCMI] Pin Control protocol table sizes not consistent");

    validation_result = scmi_pin_control_ctx.scmi_api->scmi_message_validation(
        MOD_SCMI_PROTOCOL_ID_PIN_CONTROL,
        service_id,
        payload,
        payload_size,
        message_id,
        payload_size_table,
        MOD_SCMI_PIN_CONTROL_COMMAND_COUNT,
        handler_table);

    if ((message_id == MOD_SCMI_PIN_CONTROL_SETTINGS_CONFIGURE) &&
        (validation_result == SCMI_SUCCESS)) {
        validation_result =
            validate_setting_config_payload_size(payload_size, payload);
    }

    if (validation_result == SCMI_SUCCESS) {
        return handler_table[message_id](service_id, payload);
    } else {
        return scmi_pin_control_ctx.scmi_api->respond(
            service_id, &validation_result, sizeof(validation_result));
    }
}

static struct mod_scmi_to_protocol_api
    scmi_pin_control_mod_scmi_to_protocol_api = {
        .get_scmi_protocol_id = scmi_pin_control_get_scmi_protocol_id,
        .message_handler = scmi_pin_control_message_handler
    };

static int scmi_pin_control_init(
    fwk_id_t module_id,
    unsigned int element_count,
    const void *data)
{
    size_t i;
    if (data == NULL) {
        return FWK_E_PARAM;
    }

    scmi_pin_control_ctx.config_domain_table =
        (struct mod_scmi_pinctrl_domain_config *)data;

    for (i = 0;
         i < scmi_pin_control_ctx.config_domain_table->config_domain_count;
         ++i) {
        uint16_t domain_begin =
            scmi_pin_control_ctx.config_domain_table->config_domains[i]
                .identifier_begin;
        uint16_t domain_end =
            scmi_pin_control_ctx.config_domain_table->config_domains[i]
                .identifier_end;
        int32_t domain_shift =
            scmi_pin_control_ctx.config_domain_table->config_domains[i]
                .shift_factor;

        if (domain_end < domain_begin) {
            return FWK_E_INIT;
        }

        if ((domain_shift < -UINT16_MAX) || (domain_shift > UINT16_MAX)) {
            return FWK_E_INIT;
        }
    }
    return FWK_SUCCESS;
}

static int scmi_pin_control_bind(fwk_id_t id, unsigned int round)
{
    if (round == 1) {
        return FWK_SUCCESS;
    }

    int status = fwk_module_bind(
        FWK_ID_MODULE(FWK_MODULE_IDX_SCMI),
        FWK_ID_API(FWK_MODULE_IDX_SCMI, MOD_SCMI_API_IDX_PROTOCOL),
        &scmi_pin_control_ctx.scmi_api);

    if (status != FWK_SUCCESS) {
        return status;
    }

    return fwk_module_bind(
        FWK_ID_MODULE(FWK_MODULE_IDX_PINCTRL),
        FWK_ID_API(FWK_MODULE_IDX_PINCTRL, MOD_PINCTRL_API_IDX),
        &scmi_pin_control_ctx.pinctrl_api);
}

static int scmi_pin_control_process_bind_request(
    fwk_id_t source_id,
    fwk_id_t target_id,
    fwk_id_t api_id,
    const void **api)
{
    if (!fwk_id_is_equal(source_id, FWK_ID_MODULE(FWK_MODULE_IDX_SCMI))) {
        return FWK_E_ACCESS;
    }

    *api = &scmi_pin_control_mod_scmi_to_protocol_api;

    return FWK_SUCCESS;
}

/* SCMI Pin Control Protocol Definition */
const struct fwk_module module_scmi_pin_control = {
    .api_count = 1,
    .type = FWK_MODULE_TYPE_PROTOCOL,
    .init = scmi_pin_control_init,
    .bind = scmi_pin_control_bind,
    .process_bind_request = scmi_pin_control_process_bind_request,
};
