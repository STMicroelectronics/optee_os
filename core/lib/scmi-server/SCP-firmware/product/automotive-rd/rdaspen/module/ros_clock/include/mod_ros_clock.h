/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOD_ROS_CLOCK_H
#define MOD_ROS_CLOCK_H

/*!
 * \addtogroup GroupPLATFORMModule PLATFORM Product Modules
 * \{
 */

/*!
 * \defgroup GroupRoSClock ROS Clock Driver
 *
 * \details A driver for clock devices that are part of RoS.
 *
 * \{
 */

/*!
 * \brief APIs provided by the driver.
 */
enum mod_ros_clock_api_type {
    /*! An implementation of the Clock HAL module's clock driver API */
    MOD_ROS_CLOCK_API_TYPE_CLOCK,
    MOD_ROS_CLOCK_API_COUNT,
};

/*!
 * \brief Selectable clock sources for cluster clock.
 */
enum mod_ros_clock_cluster_clock_source {
    /*! The clock source is set to the reference clock */
    MOD_ROS_CLOCK_CLUSTER_CLK_SOURCE_REFCLK = 0,
    /*! The clock source is set to the system PLL clock */
    MOD_ROS_CLOCK_CLUSTER_CLK_SOURCE_SYSPLLCLK = 1,
    /*! The clock source is set to the cluster PLL clock */
    MOD_ROS_CLOCK_CLUSTER_CLK_SOURCE_CLUSTERPLLCLK = 2,
};

/*!
 * \brief Selectable clock sources for core clock.
 */
enum mod_ros_clock_core_clock_source {
    /*! The clock source is set to the reference clock */
    MOD_ROS_CLOCK_CORE_CLK_SOURCE_REFCLK = 0,
    /*! The clock source is set to the core PLL 0 clock */
    MOD_ROS_CLOCK_CORE_CLK_SOURCE_COREPLL0CLK = 1,
    /*! The clock source is set to the core PLL 1 clock */
    MOD_ROS_CLOCK_CORE_CLK_SOURCE_COREPLL1CLK = 2,
};

/*!
 * \brief Selectable clock sources for system clock.
 */
enum mod_ros_clock_sys_clock_source {
    /*! The clock source is set to the reference clock */
    MOD_ROS_CLOCK_SYS_CLK_SOURCE_REFCLK = 0,
    /*! The clock source is set to the system PLL clock */
    MOD_ROS_CLOCK_SYS_CLK_SOURCE_SYSPLLCLK = 1,
};

/*!
 * \brief Selectable clock sources for GIC clock.
 */
enum mod_ros_clock_gic_clock_source {
    /*! The clock source is set to the reference clock */
    MOD_ROS_CLOCK_GIC_CLK_SOURCE_REFCLK = 0,
    /*! The clock source is set to the system PLL clock */
    MOD_ROS_CLOCK_GIC_CLK_SOURCE_SYSPLLCLK = 1,
    /*! The clock source is set to the GIC PLL clock */
    MOD_ROS_CLOCK_GIC_CLK_SOURCE_GICPLLCLK = 2,
};

/*!
 * \brief Selectable clock sources for IO clock.
 */
enum mod_ros_clock_io_clock_source {
    /*! The clock source is set to the reference clock */
    MOD_ROS_CLOCK_IO_CLK_SOURCE_REFCLK = 0,
    /*! The clock source is set to the system PLL clock */
    MOD_ROS_CLOCK_IO_CLK_SOURCE_SYSPLLCLK = 1,
    /*! The clock source is set to the IO PLL clock */
    MOD_ROS_CLOCK_IO_CLK_SOURCE_IOPLLCLK = 2,
};

/*!
 * \brief Selectable clock sources for peripheral clock.
 */
enum mod_ros_clock_periph_clock_source {
    /*! The clock source is set to the reference clock */
    MOD_ROS_CLOCK_PERIPH_CLK_SOURCE_REFCLK = 0,
    /*! The clock source is set to the system PLL clock */
    MOD_ROS_CLOCK_PERIPH_CLK_SOURCE_SYSPLLCLK = 1,
    /*! The clock source is set to the Perphiral PLL clock */
    MOD_ROS_CLOCK_PERIPH_CLK_SOURCE_PERIPHPLLCLK = 2,
};

/*!
 * \brief Selectable clock sources for RSE clock.
 */
enum mod_ros_clock_rse_clock_source {
    /*! The clock source is set to the reference clock */
    MOD_ROS_CLOCK_RSE_CLK_SOURCE_REFCLK = 0,
    /*! The clock source is set to the system PLL clock */
    MOD_ROS_CLOCK_RSE_CLK_SOURCE_SYSPLLCLK = 1,
    /*! The clock source is set to the RSE PLL clock */
    MOD_ROS_CLOCK_RSE_CLK_SOURCE_RSEPLLCLK = 2,
};

/*!
 * \brief Selectable clock sources for Safety Island clock.
 */
enum mod_ros_clock_si_clock_source {
    /*! The clock source is set to the reference clock */
    MOD_ROS_CLOCK_SI_CLK_SOURCE_REFCLK = 0,
    /*! The clock source is set to the system PLL clock */
    MOD_ROS_CLOCK_SI_CLK_SOURCE_SYSPLLCLK = 1,
    /*! The clock source is set to the SI PLL clock */
    MOD_ROS_CLOCK_SI_CLK_SOURCE_SIPLLCLK = 2,
};

/*!
 * \brief Selectable clock sources for SMD clock.
 */
enum mod_ros_clock_smd_clock_source {
    /*! The clock source is set to the reference clock */
    MOD_ROS_CLOCK_SMD_CLK_SOURCE_REFCLK = 0,
    /*! The clock source is set to the system PLL clock */
    MOD_ROS_CLOCK_SMD_CLK_SOURCE_SYSPLLCLK = 1,
    /*! The clock source is set to the SMD PLL clock */
    MOD_ROS_CLOCK_SMD_CLK_SOURCE_SMDPLLCLK = 2,
};

/*!
 * \brief Selectable clock sources for DBG clock.
 */
enum mod_ros_clock_dbg_clock_source {
    /*! The clock source is set to the reference clock */
    MOD_ROS_CLOCK_DBG_CLK_SOURCE_REFCLK = 0,
    /*! The clock source is set to the system PLL clock */
    MOD_ROS_CLOCK_DBG_CLK_SOURCE_SYSPLLCLK = 1,
    /*! The clock source is set to the DBG PLL clock */
    MOD_ROS_CLOCK_DBG_CLK_SOURCE_DBGPLLCLK = 2,
};

/*!
 * \brief Selectable clock sources for Trace clock.
 */
enum mod_ros_clock_trace_clock_source {
    /*! The clock source is set to the reference clock */
    MOD_ROS_CLOCK_SW_CLK_TCK_SOURCE_REFCLK = 0,
    /*! The clock source is set to the system PLL clock */
    MOD_ROS_CLOCK_SW_CLK_TCK_SOURCE_SYSPLLCLK = 1,
    /*! The clock source is set to the Trace PLL clock */
    MOD_ROS_CLOCK_SW_CLK_TCK_SOURCE_TRACEPLLCLK = 2,
};

/*!
 * \brief Divider register types.
 */
enum mod_ros_clock_clock_divider {
    /*! No divider. Only valid in case of REFCLK */
    MOD_ROS_CLOCK_CLOCK_NO_DIVIDER = 0,
    /*! Divider 0. */
    MOD_ROS_CLOCK_CLOCK_DIVIDER_0 = 1,
    /*! Divider1. */
    MOD_ROS_CLOCK_CLOCK_DIVIDER_1 = 2,
};

/*!
 * \brief ROS clock module configuration.
 */
struct mod_ros_clock_module_config {
    /*! The maximum divider value. */
    uint32_t divider_max;
};

/*!
 * \brief Rate lookup entry.
 */
struct mod_ros_clock_rate {
    /*! Rate in Hertz. */
    uint64_t rate;
    /*! Clock source used to obtain the rate. */
    uint8_t source;
    /*! Divider used to obtain the rate. */
    uint32_t divider;
};

/*!
 * \brief Subsystem clock device configuration.
 */
struct mod_ros_clock_dev_config {
    /*! Pointer to the clock's control register. */
    volatile uint32_t *const control_reg;

    /*! Pointer to the clock's rate lookup table. */
    const struct mod_ros_clock_rate *const rate_table;

    /*! The number of rates in the rate lookup table. */
    const unsigned int rate_count;

    /*! The rate, in Hz, to set during module initialization. */
    const uint64_t initial_rate;
};

/*!
 * \}
 */

/*!
 * \}
 */

#endif /* MOD_ROS_CLOCK_H */
