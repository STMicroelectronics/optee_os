/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Safety Island SSU module.
 */

#ifndef MOD_SSU_H
#define MOD_SSU_H

#include <internal/ssu_reg.h>

#include <fwk_id.h>
#include <fwk_macros.h>
#include <fwk_module_idx.h>

#include <stdbool.h>
#include <stdint.h>

/*!
 * \addtogroup GroupSSUModule SSU Product Modules
 * \{
 */

/*!
 * \defgroup Safety Island SSU Driver
 *
 * \brief Driver support for Safety Island SSU.
 *
 * \details This module provides APIs for configuring and monitoring the
 * Safety State Unit (SSU), including control register access, state
 * transitions, and error handling. It also receives fault events from the
 * Fault Management Unit (FMU).
 *
 * \{
 */

/*!
 * \brief SSU module API indices.
 */

enum mod_ssu_api_idx {
    /*! SSU SYS Register API index */
    MOD_SSU_SYS_API_IDX,

    /*! SSU Error Control and Status Register API index */
    MOD_SSU_ERR_API_IDX,

    /*! Number of APIs */
    MOD_SSU_API_COUNT,
};

/*!
 * \brief SSU device configuration.
 */
struct mod_ssu_device_config {
    /*! SSU device Base address */
    uintptr_t reg_base;
};

enum mod_ssu_fsm_safety_signal {
    /** @brief Transition FSM to SAFE state */
    MOD_SSU_FSM_SAFE_STATE = 0x0,

    /** @brief Transition FSM to ERRN (non-critical fault) state */
    MOD_SSU_FSM_NCE_STATE = 0x1,

    /** @brief Transition FSM to ERRC (critical fault) state */
    MOD_SSU_FSM_CE_STATE = 0x2,
};
#define MOD_SSU_FSM_STATE_COUNT 3

enum mod_ssu_fsm_safety_status {
    /** @brief TEST state: self-test on boot */
    MOD_SSU_SAFETY_STATUS_TEST = 0x1,

    /** @brief SAFE state: safe operation */
    MOD_SSU_SAFETY_STATUS_SAFE = 0x2,

    /** @brief ERRN state: Non-critical fault detected */
    MOD_SSU_SAFETY_STATUS_ERRN = 0x4,

    /** @brief ERRC state: Critical fault detected */
    MOD_SSU_SAFETY_STATUS_ERRC = 0x8,
};

/*!
 * \brief API to access and change the transition of SSU state.
 */
struct mod_ssu_sys_register_api {
    /*!
     * \brief Provides current FSM state of SSU.
     *
     * \param dev_id Element identifier.
     * \param[out] status Pointer to the variable that receives SSU FSM state.
     *
     * \retval ::FWK_SUCCESS Operation succeeded.
     * \return One of the other specific error codes described by the framework.
     */
    int (*get_sys_status)(fwk_id_t dev_id, uint32_t *status);

    /*!
     * \brief Triggers a state transition in the SSU finite state machine (FSM).
     *
     * \param dev_id Element identifier.
     * \param signal SSU FSM (Finite State Machine) state value.
     *
     * \retval ::FWK_SUCCESS Operation succeeded.
     * \return One of the other specific error codes described by the framework.
     */
    int (*set_sys_ctrl)(fwk_id_t dev_id, enum mod_ssu_fsm_safety_signal signal);
};

/*!
 * \brief Error control and status types.
 */
enum mod_ssu_error_signal {
    /** @brief Enable error detection and forward critical error report from
     *  FMU to SSU. */
    MOD_SSU_ERR_CR_EN,

    /** @brief Enable error detection and forward non-critical error report
     *  from FMU to SSU. */
    MOD_SSU_ERR_NCR_EN,

    /** @brief Enable detection of software-related APB errors. */
    MOD_SSU_ERR_APB_SW,

    /** @brief Enable detection of APB sequence errors. */
    MOD_SSU_ERR_APB_INC_SEQ,

    /** @brief Enable detection of APB parity faults. */
    MOD_SSU_ERR_APB_PARITY,

    /** @brief RAS architecture defined error. */
    MOD_SSU_ERR_SERR,

    /** @brief RAS architecture defined error. */
    MOD_SSU_ERR_IERR,

    /** @brief Error originated from FMU input signals. */
    MOD_SSU_ERR_IN_FMU,

    /** @brief Indicates the error syndrome status. */
    MOD_SSU_ERR_OF,

    /** @brief Indicates whether the error status bits are valid. */
    MOD_SSU_ERR_VALID,
};

/*!
 * \brief API for controlling and reading SSU internal error status bits.
 */
struct mod_ssu_error_control_status_api {
    /*!
     * \brief Get SSU error detection capabilities.
     *
     * \param dev_id Element identifier.
     * \param[out] value Pointer to the variable that stores the value.
     *
     * \retval ::FWK_SUCCESS Operation succeeded.
     * \return One of the other specific error codes described by the framework.
     */
    int (*get_err_feature)(fwk_id_t dev_id, uint8_t *value);

    /*!
     * \brief Error reporting enable/disable function.
     *
     * \param dev_id Element identifier.
     * \param enable When set to true indicates error reporting enable.
     *
     * \retval ::FWK_SUCCESS Operation succeeded.
     * \return One of the other specific error codes described by the framework.
     */
    int (*set_err_ctrl_enable)(fwk_id_t dev_id, bool enable);

    /*!
     * \brief Enables or disables a specific SSU error detection signal.
     *
     * \param dev_id Element identifier.
     * \param signal Error signal value.
     *
     * \retval ::FWK_SUCCESS Operation succeeded.
     * \return One of the other specific error codes described by the framework.
     */
    int (*err_detect_control)(
        fwk_id_t dev_id,
        enum mod_ssu_error_signal signal,
        uint32_t value);
    /*!
     * \brief Retrieves the current status of a specific SSU error signal.
     *
     * \param dev_id Element identifier.
     * \param signal Error signal value.
     * \param[out] status Pointer to the variable that receives the error
     * status.
     *
     * \retval ::FWK_SUCCESS Operation succeeded.
     * \return One of the other specific error codes described by the framework.
     */
    int (*get_err_status)(
        fwk_id_t dev_id,
        enum mod_ssu_error_signal signal,
        uint32_t *status);
};

/*!
 * \}
 */

/*!
 * \}
 */
#endif /* MOD_SSU_H */
