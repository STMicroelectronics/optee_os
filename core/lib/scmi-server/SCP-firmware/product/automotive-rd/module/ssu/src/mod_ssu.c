/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *    Implementation of SSU module.
 */

#include "internal/ssu_reg.h"

#include <mod_fmu.h>
#include <mod_ssu.h>

#include <fwk_assert.h>
#include <fwk_event.h>
#include <fwk_id.h>
#include <fwk_log.h>
#include <fwk_macros.h>
#include <fwk_mm.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>
#include <fwk_notification.h>
#include <fwk_status.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define MOD_NAME "[SSU] "

/* SSU device context (element) */
struct ssu_dev_ctx {
    /* Pointer to the device's configuration */
    const struct mod_ssu_device_config *config;
};

/* Module context */
struct ssu_ip_ctx {
    /* Table of device contexts */
    struct ssu_dev_ctx *device_ctx_table;
};

static struct ssu_ip_ctx ssu_ctx;

/*
 * SSU driver sys register API functions
 */

const char *ssu_safety_status_to_str(enum mod_ssu_fsm_safety_status val)
{
    switch (val) {
    case MOD_SSU_SAFETY_STATUS_TEST:
        return "TEST";
    case MOD_SSU_SAFETY_STATUS_SAFE:
        return "SAFE";
    case MOD_SSU_SAFETY_STATUS_ERRN:
        return "ERRN";
    case MOD_SSU_SAFETY_STATUS_ERRC:
        return "ERRC";
    default:
        FWK_LOG_ERR(MOD_NAME "Invalid SSU safety state: 0x%x", val);
        return "UNKNOWN";
    }
}

static int ssu_get_sys_status(fwk_id_t dev_id, uint32_t *value)
{
    struct ssu_dev_ctx *device_ctx;

    if (!fwk_module_is_valid_element_id(dev_id) || value == NULL) {
        return FWK_E_PARAM;
    }

    device_ctx = &ssu_ctx.device_ctx_table[fwk_id_get_element_idx(dev_id)];

    *value = ssu_reg_read32(device_ctx->config->reg_base, SSU_SYS_STATUS);

    FWK_LOG_INFO(
        MOD_NAME "SSU FSM status: %s (0x%x)",
        ssu_safety_status_to_str(*value),
        *value);

    return FWK_SUCCESS;
}

const char *ssu_safety_ctrl_to_str(enum mod_ssu_fsm_safety_signal val)
{
    switch (val) {
    case MOD_SSU_FSM_SAFE_STATE:
        return "SAFE";
    case MOD_SSU_FSM_NCE_STATE:
        return "ERRN";
    case MOD_SSU_FSM_CE_STATE:
        return "ERRC";
    default:
        FWK_LOG_ERR(MOD_NAME "Invalid SSU safety state transition: 0x%x", val);
        return "UNKNOWN";
    }
}

static int ssu_set_sys_ctrl(
    fwk_id_t dev_id,
    enum mod_ssu_fsm_safety_signal signal)
{
    struct ssu_dev_ctx *device_ctx;

    if (!fwk_module_is_valid_element_id(dev_id)) {
        return FWK_E_PARAM;
    }
    if (signal >= MOD_SSU_FSM_STATE_COUNT) {
        FWK_LOG_ERR(MOD_NAME "Invalid SSU FSM signal: %d", signal);
        return FWK_E_PARAM;
    }

    device_ctx = &ssu_ctx.device_ctx_table[fwk_id_get_element_idx(dev_id)];

    ssu_reg_write32(
        device_ctx->config->reg_base,
        SSU_SYS_CTRL,
        ((signal << SSU_SYS_CTRL_SHIFT) & SSU_SYS_CTRL_MASK));

    FWK_LOG_INFO(
        MOD_NAME "Setting SSU FSM to: %s (0x%x)",
        ssu_safety_ctrl_to_str(signal),
        signal);

    return FWK_SUCCESS;
}

static const struct mod_ssu_sys_register_api ssu_sys_register_api = {
    .get_sys_status = ssu_get_sys_status,
    .set_sys_ctrl = ssu_set_sys_ctrl,
};

/*
 * SSU driver Error Control and Status API functions
 */

static int ssu_get_err_feature(fwk_id_t dev_id, uint8_t *feature_value)
{
    struct ssu_dev_ctx *device_ctx;
    uint32_t reg;

    if (!fwk_module_is_valid_element_id(dev_id) || feature_value == NULL) {
        return FWK_E_PARAM;
    }

    device_ctx = &ssu_ctx.device_ctx_table[fwk_id_get_element_idx(dev_id)];

    reg = ssu_reg_read32(device_ctx->config->reg_base, SSU_ERR_FR);
    *feature_value = ((reg & SSU_ERR_FR_ED_MASK) >> SSU_ERR_FR_ED_SHIFT);

    FWK_LOG_DEBUG(MOD_NAME "Error feature value:0x%x", *feature_value);

    return FWK_SUCCESS;
}

static int ssu_set_err_ctrl_enable(fwk_id_t dev_id, bool enable)
{
    struct ssu_dev_ctx *device_ctx;

    if (!fwk_module_is_valid_element_id(dev_id)) {
        return FWK_E_PARAM;
    }

    device_ctx = &ssu_ctx.device_ctx_table[fwk_id_get_element_idx(dev_id)];

    if (enable) {
        ssu_set_mask(
            device_ctx->config->reg_base, SSU_ERR_CTRL, SSU_ERR_CTRL_ED_MASK);
    } else {
        ssu_clear_mask(
            device_ctx->config->reg_base, SSU_ERR_CTRL, SSU_ERR_CTRL_ED_MASK);
    }

    FWK_LOG_DEBUG(MOD_NAME "Error control %s", enable ? "enabled" : "disabled");
    FWK_LOG_DEBUG(
        MOD_NAME "Error control: 0x%x",
        ssu_reg_read32(device_ctx->config->reg_base, SSU_ERR_CTRL));
    return FWK_SUCCESS;
}

struct ssu_err_field {
    uint32_t mask;
    uint32_t shift;
};

/* Mapping for error status fields */
static const struct ssu_err_field ssu_err_status_fields[] = {
    [MOD_SSU_ERR_SERR] = { SSU_ERR_STATUS_SERR_MASK,
                           SSU_ERR_STATUS_SERR_SHIFT },
    [MOD_SSU_ERR_IERR] = { SSU_ERR_STATUS_IERR_MASK,
                           SSU_ERR_STATUS_IERR_SHIFT },
    [MOD_SSU_ERR_APB_SW] = { SSU_ERR_STATUS_IERR_APB_SW_MASK,
                             SSU_ERR_STATUS_IERR_APB_SW_SHIFT },
    [MOD_SSU_ERR_APB_INC_SEQ] = { SSU_ERR_STATUS_IERR_INC_SEQ_MASK,
                                  SSU_ERR_STATUS_IERR_INC_SEQ_SHIFT },
    [MOD_SSU_ERR_APB_PARITY] = { SSU_ERR_STATUS_IERR_APB_PARITY_MASK,
                                 SSU_ERR_STATUS_IERR_APB_PARITY_SHIFT },
    [MOD_SSU_ERR_IN_FMU] = { SSU_ERR_STATUS_IERR_ERR_IN_MASK,
                             SSU_ERR_STATUS_IERR_ERR_IN_SHIFT },
    [MOD_SSU_ERR_OF] = { SSU_ERR_STATUS_OF_MASK, SSU_ERR_STATUS_OF_SHIFT },
    [MOD_SSU_ERR_VALID] = { SSU_ERR_STATUS_VALID_MASK,
                            SSU_ERR_STATUS_VALID_SHIFT },
};

static int ssu_get_err_status(
    fwk_id_t dev_id,
    enum mod_ssu_error_signal signal,
    uint32_t *value)
{
    struct ssu_dev_ctx *device_ctx;
    uint32_t reg;

    if (!fwk_module_is_valid_element_id(dev_id) || value == NULL ||
        signal >= FWK_ARRAY_SIZE(ssu_err_status_fields)) {
        return FWK_E_PARAM;
    }

    device_ctx = &ssu_ctx.device_ctx_table[fwk_id_get_element_idx(dev_id)];
    reg = ssu_reg_read32(device_ctx->config->reg_base, SSU_ERR_STATUS);

    assert(ssu_err_status_fields[signal].shift <= 31);

    *value = (reg & ssu_err_status_fields[signal].mask) >>
        ssu_err_status_fields[signal].shift;

    return FWK_SUCCESS;
}

/* Mapping for error detection control fields */
static const struct ssu_err_field ssu_err_detect_fields[] = {
    [MOD_SSU_ERR_CR_EN] = { SSU_ERR_IMPDEF_CR_EN_MASK,
                            SSU_ERR_IMPDEF_CR_EN_SHIFT },
    [MOD_SSU_ERR_NCR_EN] = { SSU_ERR_IMPDEF_NCR_EN_MASK,
                             SSU_ERR_IMPDEF_NCR_EN_SHIFT },
    [MOD_SSU_ERR_APB_SW] = { SSU_ERR_IMPDEF_APB_SW_EN_MASK,
                             SSU_ERR_IMPDEF_APB_SW_EN_SHIFT },
    [MOD_SSU_ERR_APB_INC_SEQ] = { SSU_ERR_IMPDEF_INC_SEQ_EN_MASK,
                                  SSU_ERR_IMPDEF_INC_SEQ_EN_SHIFT },
    [MOD_SSU_ERR_APB_PARITY] = { SSU_ERR_IMPDEF_APB_PROT_EN_MASK,
                                 SSU_ERR_IMPDEF_APB_PROT_EN_SHIFT },
};

static int ssu_err_detect_control(
    fwk_id_t dev_id,
    enum mod_ssu_error_signal signal,
    uint32_t value)
{
    struct ssu_dev_ctx *device_ctx;

    if (!fwk_module_is_valid_element_id(dev_id) ||
        signal >= FWK_ARRAY_SIZE(ssu_err_detect_fields)) {
        return FWK_E_PARAM;
    }

    device_ctx = &ssu_ctx.device_ctx_table[fwk_id_get_element_idx(dev_id)];

    ssu_clear_set_mask(
        device_ctx->config->reg_base,
        SSU_ERR_IMPDEF,
        ssu_err_detect_fields[signal].mask,
        ssu_err_detect_fields[signal].shift,
        value);

    FWK_LOG_DEBUG(
        MOD_NAME "Error detect control: 0x%x",
        ssu_reg_read32(device_ctx->config->reg_base, SSU_ERR_IMPDEF));

    return FWK_SUCCESS;
}

static const struct mod_ssu_error_control_status_api
    ssu_error_control_status_api = {
        .get_err_feature = ssu_get_err_feature,
        .set_err_ctrl_enable = ssu_set_err_ctrl_enable,
        .get_err_status = ssu_get_err_status,
        .err_detect_control = ssu_err_detect_control,
    };

/*
 * Framework handler functions
 */
static int ssu_init(
    fwk_id_t module_id,
    unsigned int element_count,
    const void *data)
{
    if (element_count == 0) {
        return FWK_E_PARAM;
    }

    ssu_ctx.device_ctx_table =
        fwk_mm_calloc(element_count, sizeof(struct ssu_dev_ctx));

    FWK_LOG_INFO(MOD_NAME "SSU initialized");

    return FWK_SUCCESS;
}

static int ssu_element_init(
    fwk_id_t element_id,
    unsigned int unused,
    const void *data)
{
    struct mod_ssu_device_config *config;
    struct ssu_dev_ctx *device_ctx;

    if (!fwk_module_is_valid_element_id(element_id) || data == NULL) {
        return FWK_E_PARAM;
    }

    config = (struct mod_ssu_device_config *)data;

    device_ctx = &ssu_ctx.device_ctx_table[fwk_id_get_element_idx(element_id)];

    device_ctx->config = config;

    return FWK_SUCCESS;
}

static int ssu_start(fwk_id_t id)
{
    int status;

    if (!fwk_id_is_type(id, FWK_ID_TYPE_ELEMENT)) {
        return FWK_SUCCESS;
    }

    /* Register the module for fmu notifications */
    status = fwk_notification_subscribe(
        mod_fmu_notification_id_fault, FWK_ID_MODULE(FWK_MODULE_IDX_FMU), id);
    if (status != FWK_SUCCESS) {
        fwk_unexpected();
    }

    return FWK_SUCCESS;
}

#ifdef BUILD_HAS_NOTIFICATION
static int ssu_process_notification(
    const struct fwk_event *event,
    struct fwk_event *resp_event)
{
    struct mod_fmu_fault_notification_params *params;
    uint32_t state = 0;

    fwk_assert(fwk_id_is_type(event->target_id, FWK_ID_TYPE_ELEMENT));
    if (fwk_id_is_equal(event->id, mod_fmu_notification_id_fault)) {
        params = (struct mod_fmu_fault_notification_params *)event->params;
        if (params->critical) {
            ssu_get_sys_status(event->target_id, &state);
            if (state == MOD_SSU_SAFETY_STATUS_ERRC) {
                FWK_LOG_CRIT(MOD_NAME
                             "Critical Error received "
                             "from FMU module.");
            } else {
                FWK_LOG_CRIT(MOD_NAME
                             "Critical Error received from FMU module.");
                FWK_LOG_CRIT(MOD_NAME "But SSU FSM failed to change state");
            }
        } else {
            ssu_get_sys_status(event->target_id, &state);
            if (state == MOD_SSU_SAFETY_STATUS_ERRN) {
                FWK_LOG_CRIT(MOD_NAME
                             "Non-Critical Error received "
                             "from FMU module");
            } else {
                FWK_LOG_INFO(MOD_NAME
                             "By default, SSU FSM state is not changed for "
                             "non-critical errors");
            }
        }
    }

    return FWK_SUCCESS;
}
#endif

static int ssu_process_bind_request(
    fwk_id_t requester_id,
    fwk_id_t target_id,
    fwk_id_t api_id,
    const void **api)
{
    switch (fwk_id_get_api_idx(api_id)) {
    case MOD_SSU_SYS_API_IDX:
        *api = &ssu_sys_register_api;
        break;
    case MOD_SSU_ERR_API_IDX:
        *api = &ssu_error_control_status_api;
        break;
    default:
        return FWK_E_PARAM;
    }

    return FWK_SUCCESS;
}

const struct fwk_module module_ssu = {
    .type = FWK_MODULE_TYPE_DRIVER,
    .api_count = MOD_SSU_API_COUNT,
    .init = ssu_init,
    .element_init = ssu_element_init,
    .start = ssu_start,
    .process_bind_request = ssu_process_bind_request,
#ifdef BUILD_HAS_NOTIFICATION
    .process_notification = ssu_process_notification,
#endif
};
