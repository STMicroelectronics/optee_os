/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *      MOC SCMI pin control protocol DRV support APIs.
 */

#ifndef MOD_PINCTRL_DRV_H
#define MOD_PINCTRL_DRV_H

#include <stdint.h>

/*!
 * \addtogroup GroupModules Modules
 * \{
 */

/*!
 * \defgroup GroupModulePinctrl_drv pinctrl_drv
 * \{
 */

/*!
 * \brief Pre-defined pin configurations types from SCMI.
 */
enum mod_pinctrl_drv_configuration_type {
    MOD_PINCTRL_DRV_TYPE_DEFAULT = 0,
    MOD_PINCTRL_DRV_TYPE_BIAS_BUS_HOLD,
    MOD_PINCTRL_DRV_TYPE_BIAS_DISABLE,
    MOD_PINCTRL_DRV_TYPE_BIAS_HIGH_IMPEDANCE,
    MOD_PINCTRL_DRV_TYPE_BIAS_PULL_UP,
    MOD_PINCTRL_DRV_TYPE_BIAS_PULL_DEFAULT,
    MOD_PINCTRL_DRV_TYPE_BIAS_PULL_DOWN,
    MOD_PINCTRL_DRV_TYPE_DRIVE_OPEN_DRAIN,
    MOD_PINCTRL_DRV_TYPE_DRIVE_OPEN_SOURCE,
    MOD_PINCTRL_DRV_TYPE_DRIVE_PUSH_PULL,
    MOD_PINCTRL_DRV_TYPE_DRIVE_STRENGTH,
    MOD_PINCTRL_DRV_TYPE_INPUT_DEBOUNCE,
    MOD_PINCTRL_DRV_TYPE_INPUT_MODE,
    MOD_PINCTRL_DRV_TYPE_PULL_MODE,
    MOD_PINCTRL_DRV_TYPE_INPUT_VALUE,
    MOD_PINCTRL_DRV_TYPE_INPUT_SCHMITT,
    MOD_PINCTRL_DRV_TYPE_LOW_POWER_MODE,
    MOD_PINCTRL_DRV_TYPE_OUTPUT_MODE,
    MOD_PINCTRL_DRV_TYPE_OUTPUT_VALUE,
    MOD_PINCTRL_DRV_TYPE_POWER_SOURCE,
    MOD_PINCTRL_DRV_TYPE_SLEW_RATE,
};

/*!
 * \brief Pin configuration type to link between configurations types and its
 *      value.
 */
struct mod_pinctrl_drv_pin_configuration {
    /*! Configuration type */
    enum mod_pinctrl_drv_configuration_type config_type;

    /*! Configuration value */
    uint32_t config_value;
};

/*!
 * \brief mod pin control attributes APIs.
 *
 * \details APIs exported by pin control driver.
 */
enum mod_pinctrl_drv_api_idx {
    /*! Index for the pin control attributes API */
    MOD_PINCTRL_DRV_API_IDX,

    /*! Number of APIs */
    MOD_PINCTRL_DRV_API_COUNT
};

/*!
 * \brief Pin control DRV API.
 */
struct mod_pinctrl_drv_api {
    /*!
     * \brief Get pin function.
     *
     * \param[in] pin_id pin identifier.
     * \param[in] function_id function to be Get.
     * \retval ::FWK_SUCCESS The operation succeeded.
     * \retval ::FWK_E_RANGE pin_id >= max number of pins or group.
     * \retval ::FWK_E_PARAM function_id is not supported by the pin. is
     *      not allowed for this pin_id.
     */
    int (*get_pin_function)(uint16_t pin_id, uint32_t *function_id);

    /*!
     * \brief Set pin function.
     *
     * \param[in] pin_id pin identifier.
     * \param[in] function_id function to be set.
     * \retval ::FWK_SUCCESS The operation succeeded.
     * \retval ::FWK_E_RANGE pin_id >= max number of pins or group.
     * \retval ::FWK_E_PARAM function_id is not supported by the pin. is
     *      not allowed for this pin_id.
     */
    int (*set_pin_function)(uint16_t pin_id, uint32_t function_id);

    /*!
     * \brief Set pin configuration.
     *
     * \param[in] pin_id pin identifier.
     * \param[in] pin_config Configuration type and value.
     * \retval ::FWK_SUCCESS The operation succeeded.
     * \retval ::FWK_E_RANGE: if pin_id field does not point to a valid pin.
     * \retval ::FWK_E_PARAM:: if the config_type specify incorrect or
     *      illegal values.
     */
    int (*set_pin_configuration)(
        uint16_t pin_id,
        const struct mod_pinctrl_drv_pin_configuration *pin_config);

    /*!
     * \brief Get pin configuration.
     *
     * \param[in] pin_id pin identifier.
     * \param[in] pin_config Configuration type and value.
     * \retval ::FWK_SUCCESS The operation succeeded.
     * \retval ::FWK_E_RANGE: if pin_id field does not point to a valid pin.
     * \retval ::FWK_E_PARAM:: if the config_type specify incorrect or
     *      illegal values.
     */
    int (*get_pin_configuration)(
        uint16_t pin_id,
        struct mod_pinctrl_drv_pin_configuration *pin_config);
};

/*!
 * \}
 */

/*!
 * \}
 */

#endif /* MOD_PINCTRL_DRV_H */
