/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (C) STMicroelectronics 2022 - All Rights Reserved
 */

#ifndef __DRIVERS_STM32MP1_RCC_UTIL_H__
#define __DRIVERS_STM32MP1_RCC_UTIL_H__

/* Platform util for the RCC drivers */
vaddr_t stm32_rcc_base(void);
void stm32_reset_system(void);

/* Helper from platform RCC clock driver */
struct clk *stm32mp_rcc_clock_id_to_clk(unsigned long clock_id);
unsigned int stm32mp_rcc_clk_to_clock_id(struct clk *clk);

#ifdef CFG_STM32MP15_CLK
/* Export stm32mp1_clk_ops to make it pager resisdent for STM32MP15 */
extern const struct clk_ops stm32mp1_clk_ops;
#endif

#ifdef CFG_DRIVERS_RSTCTRL
/* Helper from platform RCC reset driver */
struct rstctrl *stm32mp_rcc_reset_id_to_rstctrl(unsigned int binding_id);
#endif

/* Save PLL1 configuration data for low power sequence if any */
void stm32mp1_clk_lp_save_opp_pll1_settings(uint8_t *data, size_t size);

#ifdef CFG_STM32_CPU_OPP
/*
 * Util for PLL1 settings management based on DT OPP table content.
 */
int stm32mp1_clk_compute_all_pll1_settings(const void *fdt, int node,
					   uint32_t buck1_voltage);
TEE_Result stm32mp1_set_opp_khz(uint32_t freq_khz);
int stm32mp1_round_opp_khz(uint32_t *freq_khz);
#endif

/* PM sequences specific to SoC STOP mode support */
void stm32mp1_clk_save_context_for_stop(void);
void stm32mp1_clk_restore_context_for_stop(void);

#endif /*__DRIVERS_STM32MP1_RCC_UTIL_H__*/
