/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017-2023, STMicroelectronics
 */

#ifndef __DRIVERS_STM32_UART_H
#define __DRIVERS_STM32_UART_H

#include <drivers/clk.h>
#include <drivers/pinctrl.h>
#include <drivers/serial.h>
#include <io.h>
#include <types_ext.h>
#include <stdbool.h>

struct stm32_uart_pdata {
	struct io_pa_va base;
	struct serial_chip chip;
	bool is_console;
	struct clk *clock;
	struct pinctrl_state *pinctrl;
	uint32_t brr;
	uint32_t presc;
	uint32_t cr1;
	uint32_t cr2;
	uint32_t cr3;
};

/*
 * stm32_uart_init - Initialize a UART serial chip and base address
 * @pd: Output initialized UART platform data
 * @base: UART interface physical base address
 */
void stm32_uart_init(struct stm32_uart_pdata *pd, vaddr_t base);

#endif /*__DRIVERS_STM32_UART_H*/
