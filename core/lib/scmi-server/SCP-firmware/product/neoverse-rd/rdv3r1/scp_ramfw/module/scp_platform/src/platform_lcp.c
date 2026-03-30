/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     SCP Platform Support - LCP interface
 */

#include "platform_core.h"
#include "scp_css_mmap.h"
#include "scp_cu_lcp_periph.h"

#include <internal/scp_platform.h>

#include <fwk_macros.h>
#include <fwk_status.h>

#include <stdbool.h>
#include <stdint.h>

static uint8_t get_lcp_count()
{
    uint32_t lcp_cfg_reg;
    uint8_t lcp_ctrl_ratio;

    /*
     * All LCPs are assumed to have the same LCP control ratio value.
     * So, read from LCP0.
     */
    lcp_cfg_reg = SCP_LCP_EXTENDED_CONTROL_PTR(0)->LCP_CFG;

    /* Read the number of APs controlled by this LCP */
    lcp_ctrl_ratio =
        (((lcp_cfg_reg >> LCP_PERIPH_EXTDCTRL_LCP_CONFIG_CTRL_RATIO_SHIFT) &
          LCP_PERIPH_EXTDCTRL_LCP_CONFIG_CTRL_RATIO_MASK) +
         1);

    return (platform_get_core_count() / lcp_ctrl_ratio);
}

static void enable_lcp_uart(uint8_t lcp_idx)
{
    FWK_RW uint32_t *lcp_uart_ctrl_reg;

    fwk_assert(lcp_idx < get_lcp_count());

    lcp_uart_ctrl_reg =
        (FWK_RW uint32_t *)&(SCP_LCP_EXTERNAL_CONTROL_PTR(lcp_idx)->UART_CTRL);

    /* Enable access to the LCP UART and the LCP UART interrupt routing */
    *lcp_uart_ctrl_reg |=
        (LCP_PERIPH_EXTRCTRL_UART_CTRL_EN_VAL
         << LCP_PERIPH_EXTRCTRL_UART_CTRL_EN_SHIFT);
}

static void release_lcp(uint8_t lcp_idx)
{
    FWK_RW uint32_t *cpu_wait_reg;

    fwk_assert(lcp_idx < get_lcp_count());

    cpu_wait_reg = (FWK_RW uint32_t *)&(SCP_LCP_CONTROL_PTR(lcp_idx)->CPUWAIT);

    /* Deassert CPUWAIT to start LCP execution */
    *cpu_wait_reg &=
        ~(LCP_PERIPH_CONTROL_CPU_WAIT_CPU0WAIT_VAL
          << LCP_PERIPH_CONTROL_CPU_WAIT_CPU0WAIT_SHIFT);
}

void platform_setup_lcp(void)
{
    uint8_t lcp_idx;

    /*
     * Enable UART access for LCP0 only. If all the LCPs are allowed to access
     * the UART at the same time, the output will be unreadable. Hence,
     * restrict the LCP UART to single LCP for now.
     */
    lcp_idx = 0;
    enable_lcp_uart(lcp_idx);

    /* Release all the LCPs */
    for (lcp_idx = 0; lcp_idx < get_lcp_count(); lcp_idx++) {
        release_lcp(lcp_idx);
    }
}
