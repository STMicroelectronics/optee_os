/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *      SCMI pin control protocol HAL support.
 */

#ifndef MOD_PINCTRL_H
#define MOD_PINCTRL_H

#include <mod_pinctrl_drv.h>

#include <fwk_id.h>

#include <stdint.h>

/*!
 * \addtogroup GroupModules Modules
 * \{
 */

/*!
 * \defgroup GroupModulePinctrl pinctrl
 * \{
 */

/*!
 * \brief Represents that no function is selected for the pin control module
 */
#define NO_FUNCTION_SELECTED_ID UINT32_MAX

/*!
 * \brief Selector to differentiate between pin, group or function.
 */
enum mod_pinctrl_selector {
    /*! Pin selector id */
    MOD_PINCTRL_SELECTOR_PIN = 0,

    /*! Group selector id */
    MOD_PINCTRL_SELECTOR_GROUP,

    /*! function selector id */
    MOD_PINCTRL_SELECTOR_FUNCTION,
};

/*!
 * \brief Function type to define the functins table.
 */
struct mod_pinctrl_function {
    /*! Pointer to NULL terminated string for name of function */
    char *name;

    /*! Attribute to mark this function as GPIO */
    bool is_gpio;

    /*! Attribute to mark this function as used only for pins */
    bool is_pin_only;
};

/*!
 * \brief Group type to define the Groups table.
 */
struct mod_pinctrl_group {
    /*! Pointer to NULL terminated string for name of function */
    char *name;

    /*! Attribute to mark this group as GPIO */
    bool is_gpio;

    /*! Group function */
    const uint16_t function;

    /*! Pointer to the pins associated to the group */
    const uint16_t *pins;

    /*! Total number of pins associated to the group */
    uint8_t num_pins;
};

/*!
 * \brief Pin type to define all pin characteristics and pins table.
 */
struct mod_pinctrl_pin {
    /*! Pointer to a NULL terminated string for name of the pin */
    char *name;

    /*! Allowed pin functions to choose from */
    const uint16_t *allowed_functions;

    /*! Number of allowed pin functions to choose from */
    const uint16_t num_allowed_functions;

    /*! Table of pin configurations */
    const enum mod_pinctrl_drv_configuration_type *configuration;

    /*! Number of pin configurations */
    const uint8_t num_configuration;

    /*! Table of read only pin configurations */
    const enum mod_pinctrl_drv_configuration_type *read_only_configuration;

    /*! Number of read only pin configurations */
    const uint8_t num_of_read_only_configurations;
};

/*!
 * \brief Attributes type to define the pin, group or function
 *      characteristics to be returned to SCMI.
 */
struct mod_pinctrl_attributes {
    /*! Pointer to NULL terminated string for the name*/
    char *name;

    /*! Number of elements to be returned to SCMI*/
    uint16_t number_of_elements;

    /*! Attribute to mark this function as used only for pins */
    bool is_pin_only_function;

    /*! Attribute to mark this function as GPIO */
    bool is_gpio_function;
};

/*!
 * \brief Attributes type to define the total numbers of pin, and groups and
 * functions counts.
 */
struct mod_pinctrl_info {
    /*! Number of pinctrl pins */
    uint16_t number_of_pins;

    /*! Number of pinctrl groups */
    uint16_t number_of_groups;

    /*! Number of pinctrl functions */
    uint16_t number_of_functions;
};

/*!
 * \brief Driver domain configuration.
 */
struct mod_pinctrl_domain_config {
    /*! Driver identifier */
    fwk_id_t driver_id;

    /*! Driver API identifier */
    fwk_id_t driver_api_id;

    /*! Driver pin base IDs*/
    uint16_t pin_base_id;

    /*! Driver pin ranges IDs*/
    uint16_t pin_range;
};

/*!
 * \brief Module configuration.
 */
struct mod_pinctrl_config {
    /*! Pointer to the table of function descriptors. */
    struct mod_pinctrl_function *functions_table;

    /*! Number of functions */
    uint16_t function_table_count;

    /*! Pointer to the table of group descriptors. */
    struct mod_pinctrl_group *groups_table;

    /*! Number of groups that will be associated to pins */
    uint16_t group_table_count;

    /*! Pointer to the table of pins descriptors. */
    struct mod_pinctrl_pin *pins_table;

    /*! Number of pins */
    uint16_t pin_table_count;
};

/*!
 * \brief mod pin control APIs.
 *
 * \details APIs exported by pin control.
 */
enum mod_pinctrl_api_idx {

    /*! Index for the pin control API */
    MOD_PINCTRL_API_IDX,

    /*! Number of APIs */
    MOD_PINCTRL_API_IDX_COUNT
};

/*!
 * \brief Pin control API.
 */
struct mod_pinctrl_api {
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
    int (*get_attributes)(
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
    int (*get_info)(struct mod_pinctrl_info *info);

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
    int (*get_list_associations)(
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
    int (*get_total_number_of_associations)(
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
    int (*get_configuration_value_from_type)(
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
    int (*get_configuration)(
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
    int (*get_total_number_of_configurations)(
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
    int (*get_current_associated_function)(
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
    int (*set_configuration)(
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
     * \param[in] function_id the fucntionality to be set.
     * \retval ::FWK_SUCCESS The operation succeeded.
     * \retval ::FWK_E_RANGE pin_id >= max number of pins or the
     *      function_id is not allowed for this pin_id.
     * \retval ::FWK_E_PARAM the flag isn't pin or group.
     */
    int (*set_function)(
        uint16_t index,
        enum mod_pinctrl_selector flag,
        uint32_t function_id);
};

/*!
 * \}
 */

/*!
 * \}
 */

#endif /* MOD_PINCTRL_H */
