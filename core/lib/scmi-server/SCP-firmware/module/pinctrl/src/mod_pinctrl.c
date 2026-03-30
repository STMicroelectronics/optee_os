/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     SCMI pin control protocol support.
 */

#include <mod_pinctrl.h>

#include <fwk_id.h>
#include <fwk_mm.h>
#include <fwk_module.h>
#include <fwk_notification.h>
#include <fwk_status.h>
#include <fwk_string.h>

#include <stdbool.h>

struct mod_pinctrl_domain_ctx {
    /*! Bound driver domain configurations */
    const struct mod_pinctrl_domain_config *drv_domain_config;

    /*! Bound Driver APIs */
    struct mod_pinctrl_drv_api *pinctrl_driver_api;
};

struct mod_pinctrl_ctx {
    /*! Pin Control Module Configuration */
    struct mod_pinctrl_config *config;

    /*! Bound driver module ctx */
    struct mod_pinctrl_domain_ctx *pinctrl_drv_domain_ctx;

    /*! Driver module domain counts */
    unsigned int pinctrl_drv_domain_count;
};

static struct mod_pinctrl_ctx pinctrl_ctx;

/*
 * PINCTRL mapper function
 */

static int get_drv_api_from_pin_id(
    uint16_t pin_id,
    struct mod_pinctrl_drv_api **pinctrl_driver_api)
{
    uint16_t pin_base_id;
    uint16_t pin_range;

    for (uint32_t i = 0; i < pinctrl_ctx.pinctrl_drv_domain_count; ++i) {
        pin_base_id = pinctrl_ctx.pinctrl_drv_domain_ctx[i]
                          .drv_domain_config->pin_base_id;
        pin_range =
            pinctrl_ctx.pinctrl_drv_domain_ctx[i].drv_domain_config->pin_range;

        if ((pin_id >= pin_base_id) && (pin_id < pin_range)) {
            *pinctrl_driver_api =
                pinctrl_ctx.pinctrl_drv_domain_ctx[i].pinctrl_driver_api;
            return FWK_SUCCESS;
        }
    }

    return FWK_E_RANGE;
}

/*
 * PINCTRL_ATTRIBUTES
 */

static int get_pin_attributes(
    uint16_t pin_id,
    struct mod_pinctrl_attributes *pinctrl_attributes)
{
    if (pin_id >= pinctrl_ctx.config->pin_table_count) {
        return FWK_E_RANGE;
    }

    pinctrl_attributes->name = pinctrl_ctx.config->pins_table[pin_id].name;
    /* One to number_of_elements where we just returning one pin attributes */
    pinctrl_attributes->number_of_elements = 1;

    return FWK_SUCCESS;
}

static int get_group_attributes(
    uint16_t group_id,
    struct mod_pinctrl_attributes *pinctrl_attributes)
{
    if (group_id >= pinctrl_ctx.config->group_table_count) {
        return FWK_E_RANGE;
    }

    const struct mod_pinctrl_group *group =
        &pinctrl_ctx.config->groups_table[group_id];

    pinctrl_attributes->name = group->name;
    pinctrl_attributes->number_of_elements = group->num_pins;

    return FWK_SUCCESS;
}

static int get_function_attributes(
    uint16_t function_id,
    struct mod_pinctrl_attributes *pinctrl_attributes)
{
    if (function_id >= (size_t)pinctrl_ctx.config->function_table_count) {
        return FWK_E_RANGE;
    }

    const struct mod_pinctrl_function *function =
        &pinctrl_ctx.config->functions_table[function_id];

    pinctrl_attributes->name = function->name;

    pinctrl_attributes->is_gpio_function = function->is_gpio;
    pinctrl_attributes->is_pin_only_function = function->is_pin_only;

    pinctrl_attributes->number_of_elements = 0;
    if (function->is_pin_only) {
        struct mod_pinctrl_pin *pins = pinctrl_ctx.config->pins_table;
        for (uint32_t index = 0; index < pinctrl_ctx.config->pin_table_count;
             ++index) {
            for (uint32_t i = 0; i < pins[index].num_allowed_functions; ++i) {
                if (function_id == pins[index].allowed_functions[i]) {
                    pinctrl_attributes->number_of_elements++;
                    break;
                }
            }
        }
    } else {
        struct mod_pinctrl_group *groups = pinctrl_ctx.config->groups_table;
        for (uint32_t index = 0; index < pinctrl_ctx.config->group_table_count;
             ++index) {
            if (function_id == groups[index].function) {
                pinctrl_attributes->number_of_elements++;
            }
        }
    }

    return FWK_SUCCESS;
}

int get_attributes(
    uint16_t index,
    enum mod_pinctrl_selector flags,
    struct mod_pinctrl_attributes *attributes)
{
    int status = FWK_SUCCESS;

    switch (flags) {
    case MOD_PINCTRL_SELECTOR_PIN:
        status = get_pin_attributes(index, attributes);
        break;
    case MOD_PINCTRL_SELECTOR_GROUP:
        status = get_group_attributes(index, attributes);
        break;

    case MOD_PINCTRL_SELECTOR_FUNCTION:
        status = get_function_attributes(index, attributes);
        break;

    default:
        status = FWK_E_RANGE;
        break;
    }

    return status;
}

/*
 * INFO
 */

int get_info(struct mod_pinctrl_info *info)
{
    if (info == NULL) {
        return FWK_E_PARAM;
    }

    info->number_of_pins = pinctrl_ctx.config->pin_table_count;
    info->number_of_functions = pinctrl_ctx.config->function_table_count;
    info->number_of_groups = pinctrl_ctx.config->group_table_count;

    return FWK_SUCCESS;
}

/*
 * PINCTRL_LIST_ASSOCIATIONS
 */

static int get_pin_id_associated_with_group(
    uint16_t group_id,
    uint16_t index,
    uint16_t *pin_id)
{
    if (index >= pinctrl_ctx.config->groups_table[group_id].num_pins) {
        return FWK_E_RANGE;
    }

    *pin_id = pinctrl_ctx.config->groups_table[group_id].pins[index];

    return FWK_SUCCESS;
}

static int get_group_id_associated_with_function(
    uint16_t function_id,
    uint16_t index,
    uint16_t *group_id)
{
    uint16_t group_count = pinctrl_ctx.config->group_table_count;
    uint16_t search_index = 0;

    if (index >= group_count) {
        return FWK_E_RANGE;
    }

    for (uint32_t group_index = 0; group_index < group_count; ++group_index) {
        if (function_id ==
            pinctrl_ctx.config->groups_table[group_index].function) {
            if (search_index == index) {
                *group_id = group_index;
                return FWK_SUCCESS;
            } else {
                ++search_index;
            }
        }
    }

    return FWK_E_RANGE;
}

static int get_pin_id_associated_with_function(
    uint16_t function_id,
    uint16_t index,
    uint16_t *pin_id)
{
    uint16_t pin_count = pinctrl_ctx.config->pin_table_count;
    uint16_t search_index = 0;

    if (index >= pin_count) {
        return FWK_E_RANGE;
    }

    for (uint32_t pin_index = 0; pin_index < pin_count; ++pin_index) {
        for (uint32_t func_index = 0; func_index <
             pinctrl_ctx.config->pins_table[pin_index].num_allowed_functions;
             ++func_index) {
            if (function_id ==
                pinctrl_ctx.config->pins_table[pin_index]
                    .allowed_functions[func_index]) {
                if (search_index == index) {
                    *pin_id = pin_index;
                    return FWK_SUCCESS;
                } else {
                    ++search_index;
                    break;
                }
            }
        }
    }

    return FWK_E_PARAM;
}

int get_list_associations(
    uint16_t id,
    enum mod_pinctrl_selector flag,
    uint16_t index,
    uint16_t *element)
{
    int status = FWK_SUCCESS;
    switch (flag) {
    case MOD_PINCTRL_SELECTOR_GROUP:
        status = get_pin_id_associated_with_group(id, index, element);
        break;
    case MOD_PINCTRL_SELECTOR_FUNCTION:
        if (id >= pinctrl_ctx.config->function_table_count) {
            return FWK_E_RANGE;
        }
        if (pinctrl_ctx.config->functions_table[id].is_pin_only) {
            status = get_pin_id_associated_with_function(id, index, element);
        } else {
            status = get_group_id_associated_with_function(id, index, element);
        }
        break;

    default:
        return FWK_E_PARAM;
        break;
    }

    return status;
}

static void get_number_of_groups_can_support_function(
    uint16_t function_id,
    uint16_t *number_of_groups)
{
    *number_of_groups = 0;
    for (uint32_t group_index = 0;
         group_index < pinctrl_ctx.config->group_table_count;
         ++group_index) {
        if (pinctrl_ctx.config->groups_table[group_index].function ==
            function_id) {
            ++(*number_of_groups);
        }
    }
}

static void get_number_pins_can_support_function(
    uint16_t function_id,
    uint16_t *number_of_pins)
{
    *number_of_pins = 0;
    for (uint32_t i = 0; i < pinctrl_ctx.config->pin_table_count; ++i) {
        for (uint32_t j = 0;
             j < pinctrl_ctx.config->pins_table[i].num_allowed_functions;
             ++j) {
            if (pinctrl_ctx.config->pins_table[i].allowed_functions[j] ==
                function_id) {
                ++(*number_of_pins);
                break;
            }
        }
    }
}

int get_total_number_of_associations(
    uint16_t index,
    enum mod_pinctrl_selector flags,
    uint16_t *total_count)
{
    switch (flags) {
    case MOD_PINCTRL_SELECTOR_GROUP:
        if (index >= pinctrl_ctx.config->group_table_count) {
            return FWK_E_RANGE;
        }
        *total_count = pinctrl_ctx.config->groups_table[index].num_pins;
        return FWK_SUCCESS;
        break;

    case MOD_PINCTRL_SELECTOR_FUNCTION:
        if (index >= pinctrl_ctx.config->function_table_count) {
            return FWK_E_RANGE;
        }

        if (pinctrl_ctx.config->functions_table[index].is_pin_only) {
            get_number_pins_can_support_function(index, total_count);
        } else {
            get_number_of_groups_can_support_function(index, total_count);
        }
        return FWK_SUCCESS;
        break;
    default:
        return FWK_E_PARAM;
        break;
    }
}

/*
 * PINCTRL_SETTINGS_GET
 */

static int get_pin_configuration_value_from_type(
    uint16_t pin_index,
    enum mod_pinctrl_drv_configuration_type config_type,
    uint32_t *config_value)
{
    struct mod_pinctrl_drv_api *drv_api;
    struct mod_pinctrl_drv_pin_configuration config;
    int status;

    if (pin_index >= pinctrl_ctx.config->pin_table_count) {
        return FWK_E_RANGE;
    }

    status = get_drv_api_from_pin_id(pin_index, &drv_api);
    if (status != FWK_SUCCESS) {
        return status;
    }

    struct mod_pinctrl_pin *pin = &pinctrl_ctx.config->pins_table[pin_index];
    for (uint32_t i = 0; i < pin->num_configuration; ++i) {
        if (pin->configuration[i] == config_type) {
            config.config_type = config_type;

            status = drv_api->get_pin_configuration(pin_index, &config);
            if (status != FWK_SUCCESS) {
                return status;
            }
            *config_value = config.config_value;

            return FWK_SUCCESS;
        }
    }

    return FWK_E_PARAM;
}

static int get_group_configuration_by_value(
    uint16_t group_id,
    enum mod_pinctrl_drv_configuration_type config_type,
    uint32_t *config_value)
{
    const int first_group_pin_index = 0;
    uint16_t pin_id;
    int status;

    if (group_id >= pinctrl_ctx.config->group_table_count) {
        return FWK_E_RANGE;
    }

    if ((pinctrl_ctx.config->groups_table[group_id].num_pins > 1) &&
        (pinctrl_ctx.config->groups_table[group_id].is_gpio == false)) {
        return FWK_E_PARAM;
    }

    status = get_pin_id_associated_with_group(
        group_id, first_group_pin_index, &pin_id);
    if (status != FWK_SUCCESS) {
        return status;
    }

    status = get_pin_configuration_value_from_type(
        pin_id, config_type, config_value);

    return status;
}

int get_configuration_value_from_type(
    uint16_t index,
    enum mod_pinctrl_selector flag,
    enum mod_pinctrl_drv_configuration_type config_type,
    uint32_t *config_value)
{
    int status;

    switch (flag) {
    case MOD_PINCTRL_SELECTOR_PIN:
        status = get_pin_configuration_value_from_type(
            index, config_type, config_value);
        break;

    case MOD_PINCTRL_SELECTOR_GROUP:
        status =
            get_group_configuration_by_value(index, config_type, config_value);
        break;

    default:
        status = FWK_E_RANGE;
        break;
    }

    return status;
}

static int get_pin_configuration_from_config_index(
    uint16_t pin_index,
    uint16_t config_index,
    struct mod_pinctrl_drv_pin_configuration *pin_config)
{
    struct mod_pinctrl_drv_api *drv_api;
    int status;

    if (pin_index >= pinctrl_ctx.config->pin_table_count) {
        return FWK_E_RANGE;
    }

    status = get_drv_api_from_pin_id(pin_index, &drv_api);
    if (status != FWK_SUCCESS) {
        return status;
    }

    if (config_index >=
        pinctrl_ctx.config->pins_table[pin_index].num_configuration) {
        return FWK_E_RANGE;
    }

    pin_config->config_type =
        pinctrl_ctx.config->pins_table[pin_index].configuration[config_index];

    status = drv_api->get_pin_configuration(pin_index, pin_config);
    if (status != FWK_SUCCESS) {
        return status;
    }

    return FWK_SUCCESS;
}

static int get_group_configuration_by_index(
    uint16_t group_id,
    uint16_t configration_index,
    struct mod_pinctrl_drv_pin_configuration *config)
{
    const int first_group_pin_index = 0;
    uint16_t pin_id;
    int status;

    if (group_id >= pinctrl_ctx.config->group_table_count) {
        return FWK_E_RANGE;
    }

    if ((pinctrl_ctx.config->groups_table[group_id].num_pins > 1) &&
        (pinctrl_ctx.config->groups_table[group_id].is_gpio == false)) {
        return FWK_E_PARAM;
    }

    status = get_pin_id_associated_with_group(
        group_id, first_group_pin_index, &pin_id);
    if (status != FWK_SUCCESS) {
        return status;
    }

    status = get_pin_configuration_from_config_index(
        pin_id, configration_index, config);

    return status;
}

int get_configuration(
    uint16_t index,
    enum mod_pinctrl_selector flag,
    uint16_t configration_index,
    struct mod_pinctrl_drv_pin_configuration *config)
{
    int status;

    switch (flag) {
    case MOD_PINCTRL_SELECTOR_PIN:
        status = get_pin_configuration_from_config_index(
            index, configration_index, config);
        break;
    case MOD_PINCTRL_SELECTOR_GROUP:
        status =
            get_group_configuration_by_index(index, configration_index, config);
        break;
    default:
        status = FWK_E_PARAM;
        break;
    }

    return status;
}

static int get_pin_total_number_of_configurations(
    uint16_t pin_index,
    uint16_t *number_of_configurations)
{
    if ((pin_index >= pinctrl_ctx.config->pin_table_count)) {
        return FWK_E_RANGE;
    }

    *number_of_configurations =
        pinctrl_ctx.config->pins_table[pin_index].num_configuration;

    return FWK_SUCCESS;
}

static int get_number_of_group_configurations(
    uint16_t group_id,
    uint16_t *number_of_configurations)
{
    const int first_group_pin_index = 0;
    uint16_t pin_id;
    int status;

    if (number_of_configurations == NULL) {
        return FWK_E_PARAM;
    }

    if (group_id >= pinctrl_ctx.config->group_table_count) {
        return FWK_E_RANGE;
    }

    status = get_pin_id_associated_with_group(
        group_id, first_group_pin_index, &pin_id);
    if (status != FWK_SUCCESS) {
        return status;
    }

    status = get_pin_total_number_of_configurations(
        pin_id, number_of_configurations);

    return status;
}

int get_total_number_of_configurations(
    uint16_t index,
    enum mod_pinctrl_selector flag,
    uint16_t *number_of_configurations)
{
    int status;

    switch (flag) {
    case MOD_PINCTRL_SELECTOR_PIN:
        status = get_pin_total_number_of_configurations(
            index, number_of_configurations);
        break;
    case MOD_PINCTRL_SELECTOR_GROUP:
        status =
            get_number_of_group_configurations(index, number_of_configurations);
        break;
    default:
        return FWK_E_PARAM;
        break;
    }

    return status;
}

int get_current_associated_function(
    uint16_t index,
    enum mod_pinctrl_selector flag,
    uint32_t *function_id)
{
    int status;
    struct mod_pinctrl_drv_api *drv_api;
    uint16_t first_pin_id;

    switch (flag) {
    case MOD_PINCTRL_SELECTOR_PIN:
        if (index >= pinctrl_ctx.config->pin_table_count) {
            status = FWK_E_RANGE;
        } else {
            status = get_drv_api_from_pin_id(index, &drv_api);
            if (status != FWK_SUCCESS) {
                return status;
            }

            status = drv_api->get_pin_function(index, function_id);
        }
        break;

    case MOD_PINCTRL_SELECTOR_GROUP:
        if (index >= pinctrl_ctx.config->group_table_count) {
            status = FWK_E_RANGE;
        } else {
            first_pin_id = pinctrl_ctx.config->groups_table[index].pins[0];

            status = get_drv_api_from_pin_id(index, &drv_api);
            if (status != FWK_SUCCESS) {
                return status;
            }

            status = drv_api->get_pin_function(first_pin_id, function_id);
        }
        break;
    default:
        status = FWK_E_PARAM;
        break;
    }

    return status;
}

/*
 * PINCTRL_SETTINGS_CONFIGURE
 */

static bool is_config_read_only(
    uint16_t pin_id,
    const enum mod_pinctrl_drv_configuration_type config)
{
    const enum mod_pinctrl_drv_configuration_type *configs =
        pinctrl_ctx.config->pins_table[pin_id].read_only_configuration;
    const uint8_t num_of_config =
        pinctrl_ctx.config->pins_table[pin_id].num_of_read_only_configurations;

    for (uint32_t i = 0; i < num_of_config; ++i) {
        if (config == configs[i]) {
            return true;
        }
    }

    return false;
}

static bool is_configuration_allowed(
    uint16_t pin_id,
    const enum mod_pinctrl_drv_configuration_type config)
{
    const enum mod_pinctrl_drv_configuration_type *configs =
        pinctrl_ctx.config->pins_table[pin_id].configuration;
    const uint8_t num_of_config =
        pinctrl_ctx.config->pins_table[pin_id].num_configuration;

    for (uint32_t i = 0; i < num_of_config; ++i) {
        if (config == configs[i]) {
            return true;
        }
    }

    return false;
}

int set_configuration(
    uint16_t index,
    enum mod_pinctrl_selector flag,
    const struct mod_pinctrl_drv_pin_configuration *config)
{
    int status = FWK_E_RANGE;

    uint16_t pin_id;
    uint16_t pin_count;
    bool read_only_config = true;
    struct mod_pinctrl_drv_api *drv_api;

    switch (flag) {
    case MOD_PINCTRL_SELECTOR_PIN:
        if (index >= pinctrl_ctx.config->pin_table_count) {
            return FWK_E_RANGE;
        }

        read_only_config = is_config_read_only(index, config->config_type);
        if (read_only_config) {
            return FWK_E_PARAM;
        }

        if (!is_configuration_allowed(index, config->config_type)) {
            return FWK_E_PARAM;
        }

        status = get_drv_api_from_pin_id(index, &drv_api);
        if (status != FWK_SUCCESS) {
            return status;
        }

        status = drv_api->set_pin_configuration(index, config);
        if (status != FWK_SUCCESS) {
            return status;
        }
        break;

    case MOD_PINCTRL_SELECTOR_GROUP:
        if (index >= pinctrl_ctx.config->group_table_count) {
            status = FWK_E_RANGE;
        } else {
            pin_count = pinctrl_ctx.config->groups_table[index].num_pins;
            for (uint32_t i = 0; i < pin_count; ++i) {
                pin_id = pinctrl_ctx.config->groups_table[index].pins[i];
                read_only_config =
                    is_config_read_only(pin_id, config->config_type);
                if (read_only_config) {
                    return FWK_E_PARAM;
                }

                if (!is_configuration_allowed(index, config->config_type)) {
                    return FWK_E_PARAM;
                }

                status = get_drv_api_from_pin_id(pin_id, &drv_api);
                if (status != FWK_SUCCESS) {
                    return status;
                }
                status = drv_api->set_pin_configuration(pin_id, config);
                if (status != FWK_SUCCESS) {
                    return status;
                }
            }
        }
        break;

    default:
        status = FWK_E_PARAM;
        break;
    }

    return status;
}

static int is_pin_function_allowed(uint16_t pin_id, uint32_t function_id)
{
    if (pin_id >= pinctrl_ctx.config->pin_table_count) {
        return FWK_E_RANGE;
    }

    for (uint32_t i = 0;
         i < pinctrl_ctx.config->pins_table[pin_id].num_allowed_functions;
         ++i) {
        if ((function_id ==
             pinctrl_ctx.config->pins_table[pin_id].allowed_functions[i]) ||
            (function_id == NO_FUNCTION_SELECTED_ID)) {
            return FWK_SUCCESS;
        }
    }

    return FWK_E_PARAM;
}

static int set_group_function(uint16_t group_index, uint32_t function)
{
    int status = FWK_SUCCESS;
    uint16_t group_pins_count = 0;
    uint16_t pin_index;
    struct mod_pinctrl_drv_api *drv_api;

    if (group_index >= pinctrl_ctx.config->group_table_count) {
        return FWK_E_RANGE;
    }

    if (pinctrl_ctx.config->groups_table[group_index].function != function) {
        return FWK_E_PARAM;
    }

    group_pins_count = pinctrl_ctx.config->groups_table[group_index].num_pins;

    for (uint32_t i = 0; i < group_pins_count; ++i) {
        pin_index = pinctrl_ctx.config->groups_table[group_index].pins[i];
        status = get_drv_api_from_pin_id(pin_index, &drv_api);
        if (status != FWK_SUCCESS) {
            return status;
        }

        status = is_pin_function_allowed(pin_index, function);
        if (status != FWK_SUCCESS) {
            return status;
        }

        status = drv_api->set_pin_function(pin_index, function);
        if (status != FWK_SUCCESS) {
            return status;
        }
    }

    return status;
}

int set_function(
    uint16_t index,
    enum mod_pinctrl_selector flag,
    uint32_t function_id)
{
    int status;
    struct mod_pinctrl_drv_api *drv_api;

    if ((function_id >= pinctrl_ctx.config->function_table_count) &&
        (function_id != NO_FUNCTION_SELECTED_ID)) {
        return FWK_E_RANGE;
    }

    switch (flag) {
    case MOD_PINCTRL_SELECTOR_PIN:
        if (pinctrl_ctx.config->functions_table[function_id].is_pin_only) {
            status = get_drv_api_from_pin_id(index, &drv_api);
            if (status != FWK_SUCCESS) {
                return status;
            }

            status = is_pin_function_allowed(index, function_id);
            if (status != FWK_SUCCESS) {
                return status;
            }

            status = drv_api->set_pin_function(index, function_id);
        } else {
            status = FWK_E_PARAM;
        }
        break;

    case MOD_PINCTRL_SELECTOR_GROUP:
        if (pinctrl_ctx.config->functions_table[function_id].is_pin_only) {
            status = FWK_E_PARAM;
        } else {
            status = set_group_function(index, function_id);
        }
        break;

    default:
        return FWK_E_PARAM;
        break;
    }

    return status;
}

struct mod_pinctrl_api mod_pinctrl_apis = {
    .get_attributes = get_attributes,
    .get_info = get_info,
    .get_list_associations = get_list_associations,
    .get_total_number_of_associations = get_total_number_of_associations,
    .get_configuration_value_from_type = get_configuration_value_from_type,
    .get_configuration = get_configuration,
    .get_total_number_of_configurations = get_total_number_of_configurations,
    .get_current_associated_function = get_current_associated_function,
    .set_configuration = set_configuration,
    .set_function = set_function,
};

/*
 * Framework handlers
 */

static int pinctrl_init(
    fwk_id_t module_id,
    unsigned int element_count,
    const void *data)
{
    struct mod_pinctrl_config *config = (struct mod_pinctrl_config *)data;

    if (config == NULL) {
        return FWK_E_PARAM;
    }

    pinctrl_ctx.config = config;

    pinctrl_ctx.pinctrl_drv_domain_ctx =
        fwk_mm_alloc(element_count, sizeof(struct mod_pinctrl_domain_ctx));

    pinctrl_ctx.pinctrl_drv_domain_count = element_count;

    return FWK_SUCCESS;
}

static int mod_pinctrl_bind(fwk_id_t id, unsigned int round)
{
    int status;
    if (round > 0)
        return FWK_SUCCESS;

    for (uint32_t i = 0; i < pinctrl_ctx.pinctrl_drv_domain_count; ++i) {
        status = fwk_module_bind(
            pinctrl_ctx.pinctrl_drv_domain_ctx[i].drv_domain_config->driver_id,
            pinctrl_ctx.pinctrl_drv_domain_ctx[i]
                .drv_domain_config->driver_api_id,
            &pinctrl_ctx.pinctrl_drv_domain_ctx[i].pinctrl_driver_api);

        if (status != FWK_SUCCESS) {
            return status;
        }
    }

    return FWK_SUCCESS;
}

static int pinctrl_element_init(
    fwk_id_t element_id,
    unsigned int sub_element_count,
    const void *data)
{
    if (data == NULL) {
        return FWK_E_DATA;
    }

    pinctrl_ctx.pinctrl_drv_domain_ctx[fwk_id_get_element_idx(element_id)]
        .drv_domain_config = (struct mod_pinctrl_domain_config *)data;

    return FWK_SUCCESS;
}

static int pinctrl_process_bind_request(
    fwk_id_t source_id,
    fwk_id_t target_id,
    fwk_id_t api_id,
    const void **api)
{
    enum mod_pinctrl_api_idx api_idx =
        (enum mod_pinctrl_api_idx)fwk_id_get_api_idx(api_id);

    switch (api_idx) {
    case MOD_PINCTRL_API_IDX:
        *api = &mod_pinctrl_apis;
        break;

    default:
        return FWK_E_ACCESS;
        break;
    }

    return FWK_SUCCESS;
}

const struct fwk_module module_pinctrl = {
    .api_count = (unsigned int)MOD_PINCTRL_API_IDX_COUNT,
    .type = FWK_MODULE_TYPE_HAL,
    .init = pinctrl_init,
    .element_init = pinctrl_element_init,
    .bind = mod_pinctrl_bind,
    .process_bind_request = pinctrl_process_bind_request,
};
