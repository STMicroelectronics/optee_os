/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     pinctrl driver.
 */

#include <mod_pinctrl_drv.h>

#include <fwk_id.h>
#include <fwk_mm.h>
#include <fwk_module.h>
#include <fwk_status.h>

static int get_pin_function(uint16_t pin_id, uint32_t *function_id)
{
    *function_id = 0xAA;
    return FWK_SUCCESS;
}

static int set_pin_function(uint16_t pin_id, uint32_t function_id)
{
    return FWK_SUCCESS;
}

static int set_pin_configuration(
    uint16_t pin_id,
    const struct mod_pinctrl_drv_pin_configuration *pin_config)
{
    return FWK_SUCCESS;
}

static int get_pin_configuration(
    uint16_t pin_id,
    struct mod_pinctrl_drv_pin_configuration *pin_config)
{
    pin_config->config_value = 0xAA55AA55;
    return FWK_SUCCESS;
}

struct mod_pinctrl_drv_api mod_pinctrl_drv_apis = {
    .get_pin_configuration = get_pin_configuration,
    .set_pin_configuration = set_pin_configuration,
    .get_pin_function = get_pin_function,
    .set_pin_function = set_pin_function,
};

/*
 * Framework handlers
 */

static int pinctrl_drv_init(
    fwk_id_t module_id,
    unsigned int element_count,
    const void *data)
{
    return FWK_SUCCESS;
}

static int pinctrl_process_bind_request(
    fwk_id_t source_id,
    fwk_id_t target_id,
    fwk_id_t api_id,
    const void **api)
{
    enum mod_pinctrl_drv_api_idx api_idx =
        (enum mod_pinctrl_drv_api_idx)fwk_id_get_api_idx(api_id);

    switch (api_idx) {
    case MOD_PINCTRL_DRV_API_IDX:
        *api = &mod_pinctrl_drv_apis;
        break;

    default:
        return FWK_E_ACCESS;
        break;
    }

    return FWK_SUCCESS;
}

const struct fwk_module module_pinctrl_drv = {
    .api_count = (unsigned int)MOD_PINCTRL_DRV_API_COUNT,
    .type = FWK_MODULE_TYPE_DRIVER,
    .init = pinctrl_drv_init,
    .process_bind_request = pinctrl_process_bind_request,
};
