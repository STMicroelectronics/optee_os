/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef MOD_PINCTRL_DRV_EXTRA_H
#define MOD_PINCTRL_DRV_EXTRA_H

#include "mod_pinctrl_drv.h"

#include <stdint.h>

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
int get_pin_function(uint16_t pin_id, uint32_t *function_id);

/*!
 * \brief set pin function.
 *
 * \param[in] pin_id pin identifier.
 * \param[in] function_id function to be set.
 * \retval ::FWK_SUCCESS The operation succeeded.
 * \retval ::FWK_E_RANGE pin_id >= max number of pins or group.
 * \retval ::FWK_E_PARAM function_id is not supported by the pin. is
 *      not allowed for this pin_id.
 */
int set_pin_function(uint16_t pin_id, uint32_t function_id);
/*!
 * \brief set pin configuration.
 *
 * \param[in] pin_id pin identifier.
 * \param[in] pin_config Configuration type and value.
 * \retval ::FWK_SUCCESS The operation succeeded.
 * \retval ::FWK_E_RANGE: if pin_id field does not point to a valid pin.
 * \retval ::FWK_E_PARAM:: if the config_type specify incorrect or
 *      illegal values.
 */
int set_pin_configuration(
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
int get_pin_configuration(
    uint16_t pin_id,
    struct mod_pinctrl_drv_pin_configuration *pin_config);

#endif
