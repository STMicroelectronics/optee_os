/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOD_MPMM_V2_H
#define MOD_MPMM_V2_H

#include <fwk_id.h>

#include <stddef.h>
#include <stdint.h>

/*!
 * \ingroup GroupModules
 *  \defgroup GroupMPMM_V2 Max Power Mitigation Mechanism Version 2(MPMM_V2)
 * \{
 */
/*! Default gear. */
#define MPMM_DEFAULT_GEAR 0

/*! Maximum number of gears allowed. */
#define MPMM_MAX_GEAR_COUNT 8

/*! Maximum number of supported cores per domain. */
#define MPMM_MAX_NUM_CORES_IN_DOMAIN 8

/*!
 * \brief API indices
 */
enum mod_mpmm_v2_pm_api {
    /*! MPMM_V2 get limit API idx */
    MOD_MPMM_V2_GET_LIMIT_API_IDX,
    /*! MPMM_V2 API count */
    MOD_MPMM_V2_API_IDX_COUNT,
};

/*!
 * \brief MPMM sub-element configuration.
 *
 * \details The configuration data of each core.
 */
struct mod_mpmm_v2_core_config {
    /*! Identifier of the power domain associated with each core. */
    fwk_id_t pd_id;

    /*! Base address of the MPMM registers */
    uintptr_t mpmm_reg_base;

    /*! Core initial power state when the platfrom starts is ON. */
    bool core_starts_online;

    /*! Identifier of the base AMU Auxiliry counter. */
    fwk_id_t base_aux_counter_id;
};

/*!
 * \brief MPMM domain configuration.
 *
 */
struct mod_mpmm_v2_domain_config {
    /*! List of core configurations. */
    struct mod_mpmm_v2_core_config const *core_config;

    /*! Maximum power allowed for this domain. */
    uint32_t max_power;

    /*! Minimum power allowed for this domain. */
    uint32_t min_power;

    /*! Gear which doesn't throttle integer workloads. */
    uint8_t safest_gear;

    /*! It is the compensating factor in our linear power equation, which
     * accounts the workload sharing when certain cores are offline.
     */
    uint32_t core_offline_factor;

    /*! Gear weights are the proportion of power required for maximum
     * efficiency for a gear. It is normalised on a scale of 100 (maximum power
     * allowed for a domain).
     */
    uint32_t *gear_weights;

    /*! Base throttling Count */
    uint64_t base_throtl_count;

    /*! Number of MPMM gears available */
    uint32_t num_of_gears;

    /*! Identifier of the performance domain associated with mpmm domain. */
    fwk_id_t perf_id;
};

/*!
 * \}
 */

#endif /* MOD_MPMM_V2_H */
