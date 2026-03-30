/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Power capping module.
 */

#ifndef MOD_POWER_CAPPING_H
#define MOD_POWER_CAPPING_H

#include <fwk_id.h>

#include <stdint.h>

/*!
 * \addtogroup GroupModules Modules
 * \{
 */

/*!
 * \defgroup GroupPOWER_CAPPING Power Capping
 * \{
 */

/*!
 * \brief Power Measurement driver interface.
 */
struct mod_power_measurement_driver_api {
    /*!
     * \brief Get average power across an averaging interval.
     *
     * \param id power measurements domain ID.
     * \param[out] power average power measured in microwatt.
     *
     * \retval ::FWK_SUCCESS The requested cap was applied successfully.
     * \return One of the standard framework error codes.
     */
    int (*get_average_power)(fwk_id_t id, uint32_t *power);

    /*!
     * \brief Set the power averaging interval.
     *
     * \param id power measurements domain ID.
     * \param pai averaging interval measured in microseconds.
     *
     * \retval ::FWK_SUCCESS The requested cap was applied successfully.
     * \return One of the standard framework error codes.
     */
    int (*set_averaging_interval)(fwk_id_t id, uint32_t pai);

    /*!
     * \brief Get the power averaging interval.
     *
     * \param id power measurements domain ID.
     * \param[out] pai averaging interval measured in microseconds.
     *
     * \retval ::FWK_SUCCESS The requested cap was applied successfully.
     * \return One of the standard framework error codes.
     */
    int (*get_averaging_interval)(fwk_id_t id, uint32_t *pai);

    /*!
     * \brief Get averaging interval step.
     *
     * \param id power measurements domain ID.
     * \param[out] pai_step The step size between two consecutive averaging
     * intervals in microseconds.
     *
     * \retval ::FWK_SUCCESS The requested cap was applied successfully.
     * \return One of the standard framework error codes.
     */
    int (*get_averaging_interval_step)(fwk_id_t id, uint32_t *pai_step);

    /*!
     * \brief Get averaging interval range.
     *
     * \param id power measurements domain ID.
     * \param[out] min_pai min averaging interval measured in microseconds.
     * \param[out] max_pai max averaging interval measured in microseconds.
     *
     * \retval ::FWK_SUCCESS The requested cap was applied successfully.
     * \return One of the standard framework error codes.
     */
    int (*get_averaging_interval_range)(
        fwk_id_t id,
        uint32_t *min_pai,
        uint32_t *max_pai);
};

/*!
 * \brief Power Capping interface.
 */
struct mod_power_capping_api {
    /*!
     * \brief Request power cap.
     *
     * \param domain_id Power Capping domain ID.
     * \param cap Requested cap to the system.
     *
     * \retval ::FWK_SUCCESS The requested cap was applied successfully.
     * \retval ::FWK_PENDING The requested cap was registered but not yet
     * applied. \return One of the standard framework error codes.
     */
    int (*request_cap)(fwk_id_t domain_id, uint32_t cap);

    /*!
     * \brief Get the applied power cap.
     *
     * \param domain_id Power Capping domain ID.
     * \param cap The applied cap to the system. Note that the applied cap
     *     is different from the requested where the applied cap is considered
     *     the real cap considered by the system and the requested cap is the
     *     cap that has been asked by an agent and is not guaranteed to be the
     *     the applied one.
     *
     * \retval ::FWK_SUCCESS The applied cap was returned successfully.
     * \return One of the standard framework error codes.
     */
    int (*get_applied_cap)(fwk_id_t domain_id, uint32_t *cap);

    /*!
     * \brief Get average power across an averaging interval.
     *
     * \param id Power measurements domain ID.
     * \param[out] power Average power measured in microwatt.
     *
     * \retval ::FWK_SUCCESS The average power applied to the system
     * measured over PAI interval was returned successfully.
     * \return One of the standard framework error codes.
     */
    int (*get_average_power)(fwk_id_t id, uint32_t *power);

    /*!
     * \brief Set the power averaging interval.
     *
     * \param id Power measurements domain ID.
     * \param pai Averaging interval measured in microseconds.
     *
     * \retval ::FWK_SUCCESS The PAI was applied successfully.
     * \return One of the standard framework error codes.
     */
    int (*set_averaging_interval)(fwk_id_t id, uint32_t pai);

    /*!
     * \brief Get the power averaging interval.
     *
     * \param id Power measurements domain ID.
     * \param[out] pai Averaging interval measured in microseconds.
     *
     * \retval ::FWK_SUCCESS The PAI was returned successfully.
     * \return One of the standard framework error codes.
     */
    int (*get_averaging_interval)(fwk_id_t id, uint32_t *pai);

    /*!
     * \brief Get averaging interval step.
     *
     * \param id Power measurements domain ID.
     * \param[out] pai_step The step size between two consecutive averaging
     * intervals in microseconds.
     *
     * \retval ::FWK_SUCCESS The PAI step was returned successfully.
     * \return One of the standard framework error codes.
     */
    int (*get_averaging_interval_step)(fwk_id_t id, uint32_t *pai_step);

    /*!
     * \brief Get averaging interval range.
     *
     * \param id Power measurements domain ID.
     * \param[out] min_pai Min averaging interval measured in microseconds.
     * \param[out] max_pai Max averaging interval measured in microseconds.
     *
     * \retval ::FWK_SUCCESS The averaging interval range was returned
     * successfully.
     * \return One of the standard framework error codes.
     */
    int (*get_averaging_interval_range)(
        fwk_id_t id,
        uint32_t *min_pai,
        uint32_t *max_pai);

    /*!
     * \brief Set power thresholds specified in power units which are in use
     *  by this domain. The platform sends a notification to the agent when
     *  the average power consumption of the power capping domain, measured
     *  over PAI, is outside the threshold values.
     *
     * \param domain_id Power Capping domain ID.
     * \param threshold_low The lower threshold specified in power units
     * used by this domain.
     * \param threshold_high The higher threshold specified in power units
     * used by this domain.
     *
     * \retval ::FWK_SUCCESS The power thresholds were applied successfully.
     * \return One of the standard framework error codes.
     */
    int (*set_power_thresholds)(
        fwk_id_t domain_id,
        uint32_t threshold_low,
        uint32_t threshold_high);
};

/*!
 * \brief Power Capping domain configuration.
 */
struct mod_power_capping_domain_config {
    /*!
     * \brief ID of the power_measurement driver.
     */
    fwk_id_t power_measurement_id;
    /*!
     * \brief API ID of the api of the power_measurement driver.
     */
    fwk_id_t power_measurement_api_id;
    /*!
     * \brief ID of the object that sets notification for the power
     *    limit change.
     */
    fwk_id_t power_limit_set_notifier_id;
    /*!
     * \brief Notification ID of the power limit change notification.
     */
    fwk_id_t power_limit_set_notification_id;

    /*! PID controller identifier. */
    fwk_id_t pid_controller_id;

    /*! PID controller API identifier. */
    fwk_id_t pid_controller_api_id;
};

/*!
 * \brief Power Capping API indices.
 */
enum mod_power_capping_api_idx {
    /*! Power Cap API. */
    MOD_POWER_CAPPING_API_IDX_CAP,

    /*! Power Management API. */
    MOD_POWER_CAPPING_API_IDX_POWER_MANAGEMENT,

    /*! Number of defined APIs. */
    MOD_POWER_CAPPING_API_IDX_COUNT,
};

/*!
 * \brief Power Capping module notification indices.
 */
enum mod_power_capping_notification_idx {
    /*! Power cap change notification. */
    MOD_POWER_CAPPING_NOTIFICATION_IDX_CAP_CHANGE,

#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
    /*! PAI changed notification. */
    MOD_POWER_CAPPING_NOTIFICATION_IDX_PAI_CHANGED,

    /*! Measurements changed notification. */
    MOD_POWER_CAPPING_NOTIFICATION_IDX_MEASUREMENTS_CHANGED,
#endif

    /*! Number of defined notifications. */
    MOD_POWER_CAPPING_NOTIFICATION_IDX_COUNT,
};

/*!
 * \}
 */

/*!
 * \}
 */

#endif /* MOD_POWER_CAPPING_H */
