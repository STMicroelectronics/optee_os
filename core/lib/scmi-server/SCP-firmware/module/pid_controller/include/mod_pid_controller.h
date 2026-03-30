/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     PID Controller.
 */

#ifndef MOD_PID_CONTROLLER_H
#define MOD_PID_CONTROLLER_H

#include <fwk_element.h>
#include <fwk_id.h>
#include <fwk_module_idx.h>

#include <stdint.h>

/*!
 * \ingroup GroupModules
 * \defgroup GroupPIDController PID Controller
 *
 * \details Generic PID Controller module.
 *
 * \{
 */

/*!
 * \brief Constant parameter structure for PID controller.
 *
 * \details This structure defines a PID controller parameter, where the
 *      numerator represents the term's value, and the divisor indicates
 *      a power-of-2 shift for scaling. This allows for fractional precision
 *      through bit-shifting, avoiding floating point operations.
 */
struct mod_pid_controller_k_parameter {
    /*! Numerator value */
    int32_t numerator;

    /*! Power-of-2 shift to scale the term value (2^divisor_shift) */
    uint8_t divisor_shift;
};

/*!
 * \brief PID controller element configuration structure.
 */
struct mod_pid_controller_elem_config {
    /*!
     * \brief Control set point value
     *
     * \details The value that the system will achive once stabilised.
     */
    int64_t set_point;

    /*!
     * \brief Switch-on value threshold.
     *
     * \details The value above which the PID loop runs. Below this
     *      threshold the output is allocated only on bias coefficients.
     */
    int64_t switch_on_value;

    /*!  Desired output when the controller is not active */
    int64_t inactive_state_output;

    /*!
     * \brief Integral cut-off threshold.
     *
     * \details The integral term will accumulate errors only when the
     *  control input is within a specified range. This helps to prevent
     *  the integral wind-up by stopping the accumulation of errors
     *  when the control input is saturated or outside the desired
     *  operating range.
     */
    int64_t integral_cutoff;

    /*!
     * \brief Integral maximum.
     *
     * \details This is the upper limit the accumulated errors.
     */
    int64_t integral_max;

    /* Controller constant parameters */

    /*!
     * \brief Controller constant parameters structure.
     *
     * \details This structure defines the constant parameters for a PID
     *      controller, where each term is represented with both a value and a
     *      corresponding denominator. The denominators are used as
     *      power-of-2 shifts to scale down their respective terms, allowing
     *      fractional precision without using floating point calculations.
     *
     *      The output of the controller is calculated as:
     *      output = (Kp * e) / (2^Kp_d_shift) +
     *               (Ki * ei) / (2^Ki_d_shift) +
     *               (Kd * ed) / (2^Kd_d_shift)
     */
    struct {
        /*! Proportional term when undershooting */
        struct mod_pid_controller_k_parameter proportional_undershoot;

        /*! Proportional term when overshooting */
        struct mod_pid_controller_k_parameter proportional_overshoot;

        /*! Integral term */
        struct mod_pid_controller_k_parameter integral;

        /*! Derivative term */
        struct mod_pid_controller_k_parameter derivative;
    } k;

    /*! Output limits */
    struct {
        /*! Minimum output limit */
        int64_t min;

        /*! Maximum output limit */
        int64_t max;
    } output;
};

/*!
 * \brief PID Controller API
 *
 * \details API for the PID controller to update and set control values.
 */
struct mod_pid_controller_api {
    /*!
     * \brief Update the PID controller with a new input value.
     *
     * \details It is crucial that sampling is performed periodically at the
     *          same time rate (at consistent intervals), because the PID
     *          controller relies on the time rate to compute the proportional,
     *          integral and derivative terms accurately.
     *
     * \param id The identifier of the PID controller.
     * \param input The input value to update the controller with.
     * \param[out] output The output value calculated by the controller.
     *
     * \retval ::FWK_SUCCESS If the call is successful.
     * \retval ::FWK_E_DATA Integral error overflow occurred.
     * \return One of the standard framework error codes.
     */
    int (*update)(fwk_id_t id, int64_t input, int64_t *output);

    /*!
     * \brief Set the control value for the PID controller.
     *
     * \param id The identifier of the PID controller.
     * \param input The control value to be set.
     *
     * \retval ::FWK_SUCCESS If the call is successful.
     * \retval ::FWK_E_PARAM Control value under switch on value.
     * \return One of the standard framework error codes.
     */
    int (*set_point)(fwk_id_t id, int64_t input);

    /*!
     * \brief Reset the PID controller.
     *
     * \details This function resets the internal state of the PID controller,
     *          including the accumulated integral term and any other relevant
     *          state variables.
     *
     * \param id The identifier of the PID controller.
     *
     * \retval ::FWK_SUCCESS If the reset is successful.
     * \return One of the standard framework error codes.
     */
    int (*reset)(fwk_id_t id);
};

/*!
 * \brief API indices.
 */
enum mod_pid_controller_api_idx {
    /*! Index for control API. */
    MOD_PID_CONTROLLER_API_IDX_CONTROL,

    /*! Number of defined APIs. */
    MOD_PID_CONTROLLER_API_IDX_COUNT,
};

/*!
 * \brief Module API identifier.
 */
extern const fwk_id_t mod_pid_controller_api_id_control;

/*!
 * \}
 */

#endif /* MOD_PID_CONTROLLER_H */
