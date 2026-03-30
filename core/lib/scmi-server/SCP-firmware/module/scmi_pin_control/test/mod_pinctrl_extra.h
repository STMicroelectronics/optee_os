/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef MOD_PINCTRL_EXTRA_H
#define MOD_PINCTRL_EXTRA_H

#include <mod_pinctrl.h>

#include <stdint.h>

/*!
 * \brief Get attributes of pin, group or function.
 *
 * \param[in] index Identifier for the pin, group, or function.
 * \param[in] flags Selector: Whether the identifier field selects
 *      a pin, a group, or a function.
 *      0 - pin
 *      1 - group
 *      2 - function
 * \param[out] attributes respond to get attribute request
 *      number_of_elements: total number of elements.
 *      is_pin_only_function: enum group_pin_serving_t.
 *      is_gpio_function: enum gpio_function_t.
 *      name: Null-terminated ASCII string.
 * \retval ::FWK_SUCCESS The operation succeeded.
 * \retval ::FWK_E_RANGE if the identifier field pertains to a
 *      non-existent pin, group, or function.
 */
int get_attributes(
    uint16_t index,
    enum mod_pinctrl_selector flags,
    struct mod_pinctrl_attributes *attributes);
/*!
 * \brief Get info of pin, group or function.
 *
 * \param[out] info return protocol attributes
 *      Number of pin groups.
 *      Number of pins.
 *      Reserved, must be zero.
 *      Number of function.
 * \retval ::FWK_SUCCESS The operation succeeded.
 */
int get_info(struct mod_pinctrl_info *info);
/*!
 * \brief Get pin associated with a group, or
 *      Get group which can enable a function, or
 *      Get pin which can enable a single-pin function.
 * \param[in] index Identifier for the group, or function.
 * \param[in] flags Selector: Whether the identifier field selects
 *      group, or function.
 *      1 - group
 *      2 - function
 * \param[in] first_index the index of the object {pin, group} to be
 *      returned
 * \param[out] object_id the returned object.
 * \retval ::FWK_SUCCESS The operation succeeded.
 * \retval ::FWK_E_RANGE index >= max number of pins associated with this
 *      function or group.
 * \retval ::FWK_E_PARAM the flags is pin.
 */
int get_list_associations(
    uint16_t index,
    enum mod_pinctrl_selector flags,
    uint16_t first_index,
    uint16_t *object_id);
/*!
 * \brief Get the total number of associated pins or groups to index.
 * \param[in] index Identifier for the group, or function.
 * \param[in] flags Selector: Whether the identifier field selects
 *      group, or function.
 *      1 - group
 *      2 - function
 * \param[in] total_count the total number of associations to the index
 * \retval ::FWK_SUCCESS The operation succeeded.
 * \retval ::FWK_E_PARAM index invalid index or flags == pin
 */
int get_total_number_of_associations(
    uint16_t index,
    enum mod_pinctrl_selector flags,
    uint16_t *total_count);
/*!
 * \brief Get pin or group specific configuration value.
 * \param[in] index Identifier for the pin, or group.
 * \param[in] flags Selector: Whether the identifier field selects
 *      a pin or group.
 *      0 - pin
 *      1 - group
 * \param[in] config_type Configuration type to be retun its value.
 * \param[out] config_value Configuration value.
 * \retval ::FWK_SUCCESS The operation succeeded.
 * \retval ::FWK_E_RANGE index >= max number of pins or groups.
 * \retval ::FWK_E_PARAM the flags isn't 0 or 1.
 */
int get_configuration_value_from_type(
    uint16_t index,
    enum mod_pinctrl_selector flag,
    enum mod_pinctrl_drv_configuration_type config_type,
    uint32_t *config_value);
/*!
 * \brief Get pin or group configuration by configuration index.
 * \param[in] index Identifier for the pin, or group.
 * \param[in] flags Selector: Whether the identifier field selects
 *      a pin or group.
 *      0 - pin
 *      1 - group
 * \param[in] configration_index configuration index
 * \param[out] config configuration type and value for specific index.
 * \retval ::FWK_SUCCESS The operation succeeded.
 * \retval ::FWK_E_RANGE configration_index > total number of configurtions.
 * \retval ::FWK_E_PARAM the flags isn't pin or group. or index >= max
 *      number of pins or groups.
 */
int get_configuration(
    uint16_t index,
    enum mod_pinctrl_selector flag,
    uint16_t configration_index,
    struct mod_pinctrl_drv_pin_configuration *config);
/*!
 * \brief Get the total number of configurations of pin or group
 * \param[in] index Identifier for the pin, or group.
 * \param[in] flags Selector: Whether the identifier field selects
 *      a pin or group.
 *      0 - pin
 *      1 - group
 * \param[out] number_of_configurations the umber of configurations
 * \retval ::FWK_SUCCESS The operation succeeded.
 * \retval ::FWK_E_PARAM the flags == function. or index >= max
 *      number of pins or groups.
 */
int get_total_number_of_configurations(
    uint16_t index,
    enum mod_pinctrl_selector flag,
    uint16_t *number_of_configurations);
/*!
 * \brief get current pin or group enabled function.
 *
 * \param[in] index pin or group identifier.
 * \param[in] flag Selector: Whether the identifier field selects
 *      a pin or group.
 *      0 - pin
 *      1 - group
 * \param[out] function_id current enabled function.
 * \retval ::FWK_SUCCESS The operation succeeded.
 * \retval ::FWK_E_RANGE pin_id >= max number of pins or the
 *      function_id is not allowed for this pin_id.
 * \retval ::FWK_E_PARAM the flag isn't pin or group.
 */
int get_current_associated_function(
    uint16_t index,
    enum mod_pinctrl_selector flag,
    uint32_t *function_id);
/*!
 * \brief set pin or group configuration.
 *
 * \param[in] index pin or group identifier.
 * \param[in] flag Selector: Whether the identifier field selects
 *      a pin or group.
 *      0 - pin
 *      1 - group
 * \param[in] config configuration to be set.
 * \retval ::FWK_SUCCESS The operation succeeded.
 * \retval ::FWK_E_RANGE pin_id >= max number of pins or the
 *      function_id is not allowed for this pin_id.
 * \retval ::FWK_E_PARAM the flag isn't pin or group.
 */
int set_configuration(
    uint16_t index,
    enum mod_pinctrl_selector flag,
    const struct mod_pinctrl_drv_pin_configuration *config);
/*!
 * \brief Set pin or group function.
 *
 * \param[in] index pin or group identifier.
 * \param[in] flag Selector: Whether the identifier field selects
 *      a pin or group.
 *      0 - pin
 *      1 - group
 * \param[in]  the fucntionality to be set.
 * \retval ::FWK_SUCCESS The operation succeeded.
 * \retval ::FWK_E_RANGE pin_id >= max number of pins or the
 *      function_id is not allowed for this pin_id.
 * \retval ::FWK_E_PARAM the flag isn't pin or group.
 */
int set_function(
    uint16_t index,
    enum mod_pinctrl_selector flag,
    uint32_t function_id);

#endif /* MOD_PINCTRL_H */
