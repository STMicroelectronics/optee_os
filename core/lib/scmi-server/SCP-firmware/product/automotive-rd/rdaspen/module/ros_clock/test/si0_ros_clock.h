/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Safety Island Cluster 0 RoS Clock definitions
 */

#ifndef SI0_ROS_CLOCK_H
#define SI0_ROS_CLOCK_H

#include <stdint.h>

/*!
 * \brief RoS Clock register definitions
 */
struct ros_clock_reg {
    uint32_t FEATURES;
    FWK_RW uint32_t CLUSTERCLK;
    FWK_RW uint32_t CORECLK;
    FWK_RW uint32_t SYSCLK;
    FWK_RW uint32_t GICCLK;
    FWK_RW uint32_t IOCLK;
    FWK_RW uint32_t PERIPHCLK;
    FWK_RW uint32_t RSECLK;
    FWK_RW uint32_t SICLK;
    FWK_RW uint32_t SMDCLK;
    FWK_RW uint32_t DBGCLK;
    FWK_RW uint32_t SWCLKTCK;
};

struct ros_clock_reg ros_clock_reg;

/* Pointer to RoS Clock register block */
#define ROS_CLOCK_PTR (&ros_clock_reg)

#endif /* SI0_ROS_CLOCK_H */
