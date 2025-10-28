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

enum pwr_monitoring {
	PWR_MON_V08CAP_TEMP,
	PWR_MON_VCORE,
	PWR_MON_VCPU,
};

vaddr_t stm32_pwr_base(void);

#endif /*__STM32MP25_PWR_H*/
