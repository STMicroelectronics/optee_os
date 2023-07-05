/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2022, STMicroelectronics
 */

#ifndef __STM32MP25_PWR_H
#define __STM32MP25_PWR_H

#include <kernel/interrupt.h>
#include <tee_api_types.h>
#include <types_ext.h>
#include <util.h>

#ifdef CFG_STM32_PWR_IRQ
/*
 * Flags for PWR wakeup event management
 * PWR_WKUP_FLAG_RISING - Detect event on signal rising edge
 * PWR_WKUP_FLAG_FALLING - Detect event on signal falling edge
 */
#define PWR_WKUP_FLAG_RISING	0
#define PWR_WKUP_FLAG_FALLING	BIT(0)

enum pwr_monitoring {
	PWR_MON_V08CAP_TEMP,
	PWR_MON_VCORE,
	PWR_MON_VCPU,
};

TEE_Result stm32mp25_pwr_irq_probe(const void *fdt, int node);
#endif /* CFG_STM32_PWR_IRQ */

vaddr_t stm32_pwr_base(void);

void stm32mp_pwr_monitoring_enable(enum pwr_monitoring monitoring);

#endif /*__STM32MP25_PWR_H*/
