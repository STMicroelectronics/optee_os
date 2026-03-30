/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "ccsm_drv.h"

#include <fwk_id.h>
#include <fwk_macros.h>
#include <fwk_mm.h>
#include <fwk_module.h>
#include <fwk_status.h>

#include <stddef.h>
#include <stdint.h>

/*
 * CCSM driver functions
 */
int ccsm_drv_get_pll_static_settings(
    struct ccsm_reg *reg,
    enum mod_ccsm_pll_static_select pll_select,
    uint32_t *config)
{
    if (pll_select == MOD_CCSM_PLL_0) {
        *config = reg->PLL_0_STATIC;
    } else if (pll_select == MOD_CCSM_PLL_1) {
        *config = reg->PLL_1_STATIC;
    } else {
        return FWK_E_PARAM;
    }

    return FWK_SUCCESS;
}

int ccsm_drv_get_pll_dynamic_settings(
    struct ccsm_reg *reg,
    enum mod_ccsm_pll_dynamic_select pll_select,
    uint32_t *config0,
    uint32_t *config1)
{
    if (pll_select == MOD_CCSM_PLL_NOMINAL) {
        *config0 = reg->PLL_NOMINAL_SETTINGS_0;
        *config1 = reg->PLL_NOMINAL_SETTINGS_1;
    } else if (pll_select == MOD_CCSM_PLL_FALLBACK) {
        *config0 = reg->PLL_FALLBACK_SETTINGS_0;
        *config1 = reg->PLL_FALLBACK_SETTINGS_1;
    } else {
        return FWK_E_PARAM;
    }

    return FWK_SUCCESS;
}

int ccsm_drv_set_pll_static_settings(
    struct ccsm_reg *reg,
    enum mod_ccsm_pll_static_select pll_select,
    uint32_t config)
{
    if (pll_select == MOD_CCSM_PLL_0) {
        reg->PLL_0_STATIC = config;
    } else if (pll_select == MOD_CCSM_PLL_1) {
        reg->PLL_1_STATIC = config;
    } else {
        return FWK_E_PARAM;
    }

    return FWK_SUCCESS;
}

int ccsm_drv_set_pll_dynamic_settings(
    struct ccsm_reg *reg,
    enum mod_ccsm_pll_dynamic_select pll_select,
    uint32_t config0,
    uint32_t config1)
{
    if (pll_select == MOD_CCSM_PLL_NOMINAL) {
        reg->PLL_NOMINAL_SETTINGS_0 = config0;
        reg->PLL_NOMINAL_SETTINGS_1 = config1;
    } else if (pll_select == MOD_CCSM_PLL_FALLBACK) {
        reg->PLL_FALLBACK_SETTINGS_0 = config0;
        reg->PLL_FALLBACK_SETTINGS_1 = config1;
    } else {
        return FWK_E_PARAM;
    }

    return FWK_SUCCESS;
}

int ccsm_drv_get_dm_configuration(struct ccsm_reg *reg, uint32_t *config)
{
    *config = reg->DROOP_MITIGATION_STRATEGY;

    return FWK_SUCCESS;
}

int ccsm_drv_set_dm_configuration(struct ccsm_reg *reg, uint32_t config)
{
    reg->DROOP_MITIGATION_STRATEGY = config;

    return FWK_SUCCESS;
}

int ccsm_drv_get_dm_telemetry(
    struct ccsm_reg *reg,
    uint32_t *telemetry1,
    uint32_t *telemetry2,
    uint32_t *telemetry3)
{
    *telemetry1 = reg->DROOP_MITIGATION_TELEMETRY1;
    *telemetry2 = reg->DROOP_MITIGATION_TELEMETRY2;
    *telemetry3 = reg->DROOP_MITIGATION_TELEMETRY3;

    return FWK_SUCCESS;
}

int ccsm_drv_set_mod_configuration(
    struct ccsm_reg *reg,
    enum mod_ccsm_mod_select core_id,
    uint32_t config)
{
    switch (core_id) {
    case MOD_CCSM_MOD_CORE0:
        reg->MODULATOR_SETTINGS_CORE0 = config;
        break;
    case MOD_CCSM_MOD_CORE1:
        reg->MODULATOR_SETTINGS_CORE1 = config;
        break;
    case MOD_CCSM_MOD_CORE2:
        reg->MODULATOR_SETTINGS_CORE2 = config;
        break;
    case MOD_CCSM_MOD_CORE3:
        reg->MODULATOR_SETTINGS_CORE3 = config;
        break;
    default:
        return FWK_E_PARAM;
    }

    return FWK_SUCCESS;
}

int ccsm_drv_get_mod_configuration(
    struct ccsm_reg *reg,
    enum mod_ccsm_mod_select core_id,
    uint32_t *config)
{
    switch (core_id) {
    case MOD_CCSM_MOD_CORE0:
        *config = reg->INT_MODULATOR_SETTINGS_CORE0;
        break;
    case MOD_CCSM_MOD_CORE1:
        *config = reg->INT_MODULATOR_SETTINGS_CORE1;
        break;
    case MOD_CCSM_MOD_CORE2:
        *config = reg->INT_MODULATOR_SETTINGS_CORE2;
        break;
    case MOD_CCSM_MOD_CORE3:
        *config = reg->INT_MODULATOR_SETTINGS_CORE3;
        break;
    default:
        return FWK_E_PARAM;
    }

    return FWK_SUCCESS;
}

bool ccsm_drv_is_irq_error_clear(void *reg)
{
    return (((struct ccsm_reg *)reg)->IRQ_ERROR == 0);
}

int ccsm_drv_get_clear_irq_error(struct ccsm_reg *reg, uint32_t *error)
{
    *error = reg->IRQ_ERROR;

    if (*error != 0) {
        /* IRQ error is not already clear. */
        reg->IRQ_ERROR_CLEAR = *error;
    }

    return FWK_SUCCESS;
}

bool ccsm_drv_is_irq_status_clear(void *reg)
{
    return (((struct ccsm_reg *)reg)->IRQ_STATUS == 0);
}

int ccsm_drv_get_clear_irq_status(struct ccsm_reg *reg, uint32_t *status)
{
    *status = reg->IRQ_STATUS;

    if (*status != 0) {
        /* IRQ status is not already clear. */
        reg->IRQ_STATUS_CLEAR = *status;
    }

    return FWK_SUCCESS;
}

bool ccsm_drv_is_request_complete(void *reg)
{
    return (
        (((struct ccsm_reg *)reg)->REQUEST_STATUS & CCSM_REQUEST_STATUS_OG) ==
        0);
}

int ccsm_drv_set_request(
    struct ccsm_reg *reg,
    enum mod_ccsm_request_type req_type)
{
    if (reg->INT_REQUEST_TYPE != 0) {
        return FWK_E_DEVICE;
    }

    if ((reg->REQUEST_STATUS & CCSM_REQUEST_STATUS_OG) != 0) {
        return FWK_E_BUSY;
    }

    reg->REQUEST_TYPE = req_type;
    reg->REQUEST_ENABLE = CCSM_REQUEST_ENABLE_SET;

    return FWK_SUCCESS;
}

int ccsm_drv_clear_request(struct ccsm_reg *reg)
{
    reg->REQUEST_TYPE = CCSM_REQUEST_TYPE_CLR;
    reg->REQUEST_ENABLE = CCSM_REQUEST_ENABLE_CLR;

    return FWK_SUCCESS;
}
