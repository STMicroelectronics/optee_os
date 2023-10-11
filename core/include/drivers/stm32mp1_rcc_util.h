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

#ifdef CFG_STM32MP15_CLK
/* Export stm32mp1_clk_ops to make it pager resisdent for STM32MP15 */
extern const struct clk_ops stm32mp1_clk_ops;
#endif

#ifdef CFG_DRIVERS_RSTCTRL
/* Helper from platform RCC reset driver */
struct rstctrl *stm32mp_rcc_reset_id_to_rstctrl(unsigned int binding_id);
#endif

#endif /*__DRIVERS_STM32MP1_RCC_UTIL_H__*/
