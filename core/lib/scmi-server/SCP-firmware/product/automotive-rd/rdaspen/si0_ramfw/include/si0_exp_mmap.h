/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Base address definitions for SI0's expansion memory regions.
 */

#ifndef SI0_EXP_MMAP_H
#define SI0_EXP_MMAP_H

#include "si0_mmap.h"

#define SI0_PLL_BASE (SI0_SMD_EXPANSION3_BASE)

#define SI0_PLL_CORE    (SI0_PLL_BASE + 0x0004UL)
#define SI0_PLL_CORE0   (SI0_PLL_BASE + 0x0008UL)
#define SI0_PLL_CORE1   (SI0_PLL_BASE + 0x000CUL)
#define SI0_PLL_SYS     (SI0_PLL_BASE + 0x0010UL)
#define SI0_PLL_PERIPH  (SI0_PLL_BASE + 0x0014UL)
#define SI0_PLL_GIC     (SI0_PLL_BASE + 0x0018UL)
#define SI0_PLL_IOBLOCK (SI0_PLL_BASE + 0x001CUL)
#define SI0_PLL_RSE     (SI0_PLL_BASE + 0x0020UL)
#define SI0_PLL_SI      (SI0_PLL_BASE + 0x0024UL)
#define SI0_PLL_SMD     (SI0_PLL_BASE + 0x0028UL)
#define SI0_PLL_DBG     (SI0_PLL_BASE + 0x002CUL)
#define SI0_PLL_TRACE   (SI0_PLL_BASE + 0x0030UL)

#define SI0_PLL_LOCK_MASK (0x1UL)

#define SI0_ROS_CLOCK_BASE (SI0_ATW2_SMD_EXPANSION_BASE + (64 * FWK_KIB))

#endif /* SI0_EXP_MMAP_H */
