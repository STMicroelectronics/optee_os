/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOD_CCSM_H
#define MOD_CCSM_H

#include <mod_clock.h>

#include <fwk_element.h>
#include <fwk_id.h>
#include <fwk_macros.h>

#include <stdbool.h>
#include <stdint.h>

/*!
 * \ingroup GroupModules
 * \defgroup GroupSystemCCSM Module
 *
 * \details A driver for Clock Control State Machines.
 *
 * \{
 */

/*! Indexes of APIs that the module offers for binding. */
enum mod_ccsm_api_types {
    /*! Implementation of Clock API. */
    MOD_CCSM_CLOCK_API,
    /*! Number of APIs available for binding. */
    MOD_CCSM_API_COUNT,
};

/*! Rate lookup table clock rates entry. */
struct mod_ccsm_clock_rate {
    /*! Rate of nominal clock in Hertz. */
    uint64_t nominal_clock_rate_hz;

    /*! Rate of fallback clock in Hertz. */
    uint64_t fallback_clock_rate_hz;
};

/*! Rate register values lookup table entry. */
struct mod_ccsm_clock_rate_reg_value {
    /*! Rate of clock in Hertz. */
    uint64_t clock_rate_hz;

    /*! Value for PLL settings 0 register for rate. */
    uint32_t pll_settings_0;

    /*! Value for PLL settings 1 register for rate. */
    uint32_t pll_settings_1;
};

/*! Configuration values for droop mitigation strategy. */
enum mod_ccsm_dm_strategy {
    /*! Stop output clock on droop. */
    MOD_CCSM_DM_STOP_CLK = 0,
    /*! Switch output from nominal to fallback clock on droop. */
    MOD_CCSM_DM_SW_FB = 1 << 16,
    /*! Do not mitigate droops, always output nominal clock. */
    MOD_CCSM_DM_NOM_ONLY = 1 << 17,
    /*! Do not stop the output clock during DVFS sequence. */
    MOD_CCSM_DM_CLK_ON_DVFS = 1 << 20
};

/*! Configuration values for droop detector in use. */
enum mod_ccsm_dm_dd {
    /*! Use Arm Droop Detector for TRIG_DROOP. */
    MOD_CCSM_DM_ARM_DD = 0,
    /*! Use an alternate droop input for TRIG_DROOP. */
    MOD_CCSM_DM_TPIP = 1 << 28
};

/*! Configuration values for TRIG_SOFF input. */
enum mod_ccsm_dm_dd_soff {
    /*! Ignore TRIG_SOFF input. */
    MOD_CCSM_DM_SW_SOFF_IGNORE = 0,
    /*! Pause the output clock on 0->1 transition of TRIG_SOFF. */
    MOD_CCSM_DM_SW_SOFF_STOP = 1 << 24,
};

/*! Droop mitigation configuration options. */
struct mod_ccsm_dm_config {
    /*! Strategy for mitigating droop events. */
    enum mod_ccsm_dm_strategy strategy;

    /*! Selector for droop detector in use. */
    enum mod_ccsm_dm_dd dd;

    /*! Configuration of TRIG_SOFF input handling. */
    enum mod_ccsm_dm_dd_soff soff;

    /*! Number of clock cycles to pause during nominal-fallback transition. */
    uint8_t transition_pause;

    /*! Minimum duration (number of clock cycles) of droop mitigation. */
    uint16_t mitigation_duration;
};

/*! Modulator configuration options. */
struct mod_ccsm_mod_config {
    /*! Value of the numerator on PMIC_OC warning. */
    uint8_t numerator_oc;

    /*! Value of the numerator in regular operation. */
    uint8_t numerator_regular;

    /*! Value of the denominator. */
    uint8_t denominator;
};

/*! PLL device configuration. */
struct mod_ccsm_dev_config {
    /*! Selects if a rate lookup table should be used (indexed clock). */
    bool rate_lookup_table_is_provided;

    /*! The minimum rate at which to enable the fallback clock. */
    uint64_t minimum_clock_rate_fallback_enable_hz;

    /*! Maximum percentage of nominal rate to set fallback clock to. */
    uint8_t fallback_clock_percentage;

    /*!
     * \brief The slowest clock rate either PLL can be set to.
     *
     * \details This will represent the minimum clock rate in operation, and
     *      may differ from the hardware-imposed limit.
     */
    uint64_t min_clock_rate_hz;

    /*!
     * \brief The fastest rate either PLL can be set to.
     *
     * \details This will represent the maximum clock rate in operation, and
     *      may differ from the hardware-imposed limit.
     */
    uint64_t max_clock_rate_hz;

    /*!
     * \brief The minimum step size between clock rates when setting the PLL.
     *
     * \details This represents the precision that can be used when setting
     *      the PLL rates. Rates should be a multiple of this value. Step sizes
     *      may be greater than the hardware-imposed limit.
     */
    uint64_t clock_rate_step_hz;

    /*! Base address of the CCSM device registers. */
    uintptr_t base_address;

    /*!
     * \brief The static register value to set for PLL0.
     *
     * \details This value will be set during initialisation of the CCSM, and
     *      should not be changed during operation.
     */
    uint32_t pll_0_static_reg_value;

    /*!
     * \brief The static register value to set for PLL1.
     *
     * \details This value will be set during initialisation of the CCSM, and
     *      should not be changed during operation.
     */
    uint32_t pll_1_static_reg_value;

    /*! The initial droop mitigation configuration. */
    const struct mod_ccsm_dm_config *droop_mitigation_default;

    /*! The initial modulator configuration. */
    const struct mod_ccsm_mod_config *modulator_default;

    /*! The number of modulators implemented in the CCSM instance. */
    uint8_t modulator_count;

    /*! The default clock rates for the CCSM PLLs. */
    const struct mod_ccsm_clock_rate *default_rates_table;

    /*!
     * \brief Pointer to the clock rate lookup table for indexed clocks.
     *
     * \details Required for indexed clocks. Contains each pair of nominal and
     *      fallback clock rates the PLLs may be set to for a DVFS state. Rates
     *      must be ordered by nominal clock rate, from lowest to highest.
     */
    struct mod_ccsm_clock_rate const *rate_table;

    /*! The number of rates in the rate lookup table. */
    uint32_t rate_count;

    /*!
     * \brief Pointer to rate-register value lookup table.
     *
     * \details Contains entries with clock rates and corresponding register
     *      configuration values. For continuous clocks, an entry should be
     *      present for each clock rate step between the minimum and maximum
     *      clock rates. Rates must be ordered by nominal clock rate, from
     *      lowest to highest.
     */
    struct mod_ccsm_clock_rate_reg_value const *register_rate_table;

    /*! Number of register-rate values in the lookup table. */
    uint32_t register_rate_count;

    /*! Element identifier for the timer device. */
    fwk_id_t timer_id;
};

/*!
 * \}
 */

#endif /* MOD_CCSM_H */
