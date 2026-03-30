/*
 * Arm SCP/MCP Software
 * Copyright (c) 2023-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef CLOCK_SOC_H
#define CLOCK_SOC_H

#include <fwk_macros.h>

#if !defined(PLAT_FVP)
#    define CLOCK_RATE_GTCLK (1000UL * FWK_MHZ)
#else
#    define CLOCK_RATE_GTCLK (125UL * FWK_MHZ)
#endif

#define CLOCK_RATE_SYSPLLCLK (2000UL * FWK_MHZ)

/*
 * PLL clock indexes.
 */
enum clock_pll_idx {
    CLOCK_PLL_IDX_SYS,
    CLOCK_PLL_IDX_DPU,
    CLOCK_PLL_IDX_PIX0,
    CLOCK_PLL_IDX_PIX1,
    CLOCK_PLL_IDX_COUNT
};

/*
 * PIK clock indexes.
 */
enum clock_pik_idx {
    CLOCK_PIK_IDX_CLUS0_CPU0,
    CLOCK_PIK_IDX_CLUS0_CPU1,
    CLOCK_PIK_IDX_CLUS0_CPU2,
    CLOCK_PIK_IDX_CLUS0_CPU3,
    CLOCK_PIK_IDX_CLUS0_CPU4,
    CLOCK_PIK_IDX_CLUS0_CPU5,
    CLOCK_PIK_IDX_CLUS0_CPU6,
    CLOCK_PIK_IDX_CLUS0_CPU7,
    CLOCK_PIK_IDX_GIC,
    CLOCK_PIK_IDX_PCLKSCP,
    CLOCK_PIK_IDX_SYSPERCLK,
    CLOCK_PIK_IDX_UARTCLK,
    CLOCK_PIK_IDX_DPU,
    CLOCK_PIK_IDX_COUNT
};

/*
 * CCSM clock indexes
 */
enum clock_ccsm_idx {
    CLOCK_CCSM_IDX_CPU_GROUP_LITTLE,
    CLOCK_CCSM_IDX_CPU_GROUP_MID,
    CLOCK_CCSM_IDX_CPU_GROUP_BIG,
    CLOCK_CCSM_IDX_GPU,
    CLOCK_CCSM_IDX_COUNT,
};

/*!
 * \brief Selectable clock sources for TC4 cluster clocks.
 */
enum mod_clusclock_source_tc4 {
    /*! The clock is gated */
    MOD_PIK_CLOCK_CLUSCLK_SOURCE_TC_GATED = 0x0,
    /*! The clock source is set to the system reference clock */
    MOD_PIK_CLOCK_CLUSCLK_SOURCE_TC_SYSREFCLK = 0x1,
    /*! The clock source is set to a private cluster PLL */
    MOD_PIK_CLOCK_CLUSCLK_SOURCE_TC_PLL0 = 0x2,
    /*! The clock source is set to a private cluster PLL */
    MOD_PIK_CLOCK_CLUSCLK_SOURCE_TC_PLL1 = 0x4,
    /*! The clock source is set to a private cluster PLL */
    MOD_PIK_CLOCK_CLUSCLK_SOURCE_TC_PLL2 = 0x8,
    /*! The clock source is set to a private cluster PLL */
    MOD_PIK_CLOCK_CLUSCLK_SOURCE_TC_PLL3 = 0x10,
    /*! Number of valid clock sources */
    MOD_PIK_CLOCK_CLUSCLK_SOURCE_TC_MAX
};

/*
 * CSS clock indexes.
 */
enum clock_css_idx { CLOCK_CSS_IDX_DPU, CLOCK_CSS_IDX_COUNT };

/*
 * Clock indexes.
 */
enum clock_idx {
    CLOCK_IDX_CPU_GROUP_LITTLE,
    CLOCK_IDX_CPU_GROUP_MID,
    CLOCK_IDX_CPU_GROUP_BIG,
    CLOCK_IDX_DPU,
    CLOCK_IDX_PIXEL_0,
    CLOCK_IDX_PIXEL_1,
    CLOCK_IDX_GPU,
    CLOCK_IDX_COUNT
};

#endif /* CLOCK_SOC_H */
