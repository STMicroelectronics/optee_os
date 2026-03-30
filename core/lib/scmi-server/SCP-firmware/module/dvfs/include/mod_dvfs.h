/*
 * Arm SCP/MCP Software
 * Copyright (c) 2017-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOD_DVFS_H
#define MOD_DVFS_H

#include <fwk_id.h>
#include <fwk_macros.h>
#include <fwk_module_idx.h>

#include <stddef.h>
#include <stdint.h>

/*!
 * \brief Domain states for GET_OPP/SET_OPP.
 */
enum mod_dvfs_domain_state {
    DVFS_DOMAIN_STATE_IDLE = 0,

    /* Waiting for GET_OPP request */
    DVFS_DOMAIN_GET_OPP,

    /* Waiting for SET_OPP request */
    DVFS_DOMAIN_SET_OPP,

    /* set_rate() in progress, set_voltage() next */
    DVFS_DOMAIN_SET_VOLTAGE,

    /* set_voltage() in progress, set_frequency() next */
    DVFS_DOMAIN_SET_FREQUENCY,

    /* waiting for SET_OPP to complete */
    DVFS_DOMAIN_SET_OPP_DONE,

    /* waiting for alarm callback to start a retry */
    DVFS_DOMAIN_STATE_RETRY,
};

/*!
 * \ingroup GroupModules
 * \defgroup GroupDvfs Dynamic Voltage and Frequency Scaling (DVFS)
 * \{
 */

/*!
 * \defgroup GroupDvfsTypes Types
 * \{
 */

/*!
 * \brief Operating Performance Point (OPP).
 */
struct mod_dvfs_opp {
    uint32_t level; /*!< Level value of the OPP */
    uint32_t voltage; /*!< Power supply voltage in millivolts (mV) */
    uint32_t frequency; /*!< Clock rate in Hertz (Hz) */
    uint32_t power; /*!< Power draw in milliwatts (mW) */
};

/*!
 * \}
 */

/*!
 * \defgroup GroupDvfsConfig Configuration
 * \{
 */

/*!
 * \brief Domain configuration.
 */
struct mod_dvfs_domain_config {
    /*!
     * \brief Power supply identifier.
     *
     * \warning This identifier must refer to an element of the \c psu module.
     */
    fwk_id_t psu_id;

    /*!
     * \brief Clock identifier.
     *
     * \warning This identifier must refer to an element of the \c clock module.
     */
    fwk_id_t clock_id;

    /*!
     * \brief Alarm identifier.
     *
     * \warning This identifier must refer to an alarm of the \c timer module.
     */
    fwk_id_t alarm_id;

    /*! Delay in microseconds before retrying a request */
    uint32_t retry_us;

    /*! Worst-case transition latency in microseconds */
    uint16_t latency;

    /*! Sustained operating point index */
    size_t sustained_idx;

    /*!
     * \brief Operating points.
     *
     * \note The frequencies and levels of these operating points must be in
     *      ascending order.
     */
    struct mod_dvfs_opp *opps;
};

/*!
 * \brief Request for SET_OPP.
 */
struct mod_dvfs_request {
    /*! New operating point data for the request */
    struct mod_dvfs_opp new_opp;

    /*! Context-specific value */
    uintptr_t cookie;

    /*! Response expected for this request */
    bool response_required;

    /*! Retry the request on failure */
    bool retry_request;

    /*! This request requires the event source_id to be set */
    bool set_source_id;

    /*! Retry count */
    uint8_t num_retries;
};

/*!
 * \brief Domain context.
 */
struct mod_dvfs_domain_ctx {
    /*! Context Domain ID */
    fwk_id_t domain_id;

    /*! Bound ID of requester to this domain */
    fwk_id_t bound_id;

    /*! Domain configuration */
    const struct mod_dvfs_domain_config *config;

    /*!
     * \brief Struct of APIs.
     */
    struct {
        /*! Power supply API */
        const struct mod_psu_device_api *psu;

        /*! Clock API */
        const struct mod_clock_api *clock;

        /*! Alarm API for pending requests */
        const struct mod_timer_alarm_api *alarm_api;
    } apis;

    /*! Number of operating points */
    size_t opp_count;

    /*! Current operating point */
    struct mod_dvfs_opp current_opp;

    /*! Current request details */
    struct mod_dvfs_request request;

    /*! State */
    enum mod_dvfs_domain_state state;

    /*! Cookie for deferred request response */
    uint32_t cookie;

    /*! Pending request details */
    struct mod_dvfs_request pending_request;

    /*! SET_OPP Request is pending for this domain */
    bool request_pending;
};

/*!
 * \}
 */

/*!
 * \defgroup GroupDvfsApis APIs
 * \{
 */

/*!
 * \brief Domain API.
 */
struct mod_dvfs_domain_api {
    /*!
     * \brief Get the current operating point of a domain.
     *
     * \param domain_id Element identifier of the domain.
     * \param [out] opp Current operating point.
     */
    int (*get_current_opp)(fwk_id_t domain_id, struct mod_dvfs_opp *opp);

    /*!
     * \brief Get the sustained operating point of a domain.
     *
     * \param domain_id Element identifier of the domain.
     * \param [out] opp Sustained operating point.
     */
    int (*get_sustained_opp)(
        fwk_id_t domain_id,
        struct mod_dvfs_opp *opp);

    /*!
     * \brief Get an operating point from its index.
     *
     * \param domain_id Element identifier of the domain.
     * \param n Index of the operating point to retrieve.
     * \param [out] opp Requested operating point.
     */
    int (*get_nth_opp)(
        fwk_id_t domain_id,
        size_t n,
        struct mod_dvfs_opp *opp);

    /*!
     * \brief Get the number of operating points of a domain.
     *
     * \param domain_id Element identifier of the domain.
     * \param [out] opp_count Number of operating points.
     */
    int (*get_opp_count)(fwk_id_t domain_id, size_t *opp_count);

    /*!
     * \brief Get the level id for the given level.
     *
     * \param domain_id Element identifier of the domain.
     * \param level Requested level.
     * \param [out] level id inside the OPP table.
     */
    int (*get_level_id)(fwk_id_t domain_id, uint32_t level, size_t *level_id);

    /*!
     * \brief Get the worst-case transition latency of a domain.
     *
     * \param domain_id Element identifier of the domain.
     * \param [out] latency Worst-case transition latency.
     */
    int (*get_latency)(fwk_id_t domain_id, uint16_t *latency);

    /*!
     * \brief Set the level of a domain.
     *
     * \param domain_id Element identifier of the domain.
     * \param cookie Context-specific value.
     * \param level Requested level.
     */
    int (*set_level)(fwk_id_t domain_id, uintptr_t cookie, uint32_t level);
};

/*!
 * \}
 */

/*!
 * \defgroup GroupDvfsEvents Events
 * \{
 */

/*!
 * \brief <tt>Get current OPP </tt> event response parameters.
 */
struct mod_dvfs_params_response {
    /*! Event response status */
    int status;

    /*! Event response frequency */
    uint32_t performance_level;
};

/*!
 * \}
 */
/*!
 * \defgroup GroupDvfsIds Identifiers
 * \{
 */

/*!
 * \brief API indices.
 */
enum mod_dvfs_api_idx {
    MOD_DVFS_API_IDX_DVFS, /*!< API index for mod_dvfs_api_id_dvfs() */
    MOD_DVFS_API_IDX_CNTRL_DRV, /*!< API index for mod_dvfs_api_id_cntrl_drv */
    MOD_DVFS_API_IDX_COUNT /*!< Number of defined APIs */
};

/*! Module API identifier */
static const fwk_id_t mod_dvfs_api_id_dvfs =
    FWK_ID_API_INIT(FWK_MODULE_IDX_DVFS, MOD_DVFS_API_IDX_DVFS);

#ifdef BUILD_HAS_MOD_PERF_CONTROLLER
static const fwk_id_t mod_dvfs_api_id_cntrl_drv =
    FWK_ID_API_INIT(FWK_MODULE_IDX_DVFS, MOD_DVFS_API_IDX_CNTRL_DRV);
#endif

/*!
 * \brief Event indices.
 */
enum mod_dvfs_event_idx {
    MOD_DVFS_EVENT_IDX_SET, /*!< Set level/limits */
    MOD_DVFS_EVENT_IDX_GET_OPP, /*!< Get Operating Performance Point */
    MOD_DVFS_EVENT_IDX_COUNT, /*!< event count */
};

/*! <tt>Set operating point/limits</tt> event identifier */
static const fwk_id_t mod_dvfs_event_id_set =
    FWK_ID_EVENT_INIT(FWK_MODULE_IDX_DVFS, MOD_DVFS_EVENT_IDX_SET);

/*! <tt>Get current OPP </tt> event identifier */
static const fwk_id_t mod_dvfs_event_id_get_opp =
    FWK_ID_EVENT_INIT(FWK_MODULE_IDX_DVFS, MOD_DVFS_EVENT_IDX_GET_OPP);

/*!
 * \}
 */

/*!
 * \}
 */

#endif /* MOD_DVFS_H */
