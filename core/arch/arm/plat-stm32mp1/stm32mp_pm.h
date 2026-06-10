/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017-2018, STMicroelectronics
 */
#ifndef __STM32MP_PM_H__
#define __STM32MP_PM_H__

#include <dt-bindings/power/stm32mp1-power.h>

#define PSCI_MODE_SYSTEM_SUSPEND	0
#define PSCI_MODE_SYSTEM_OFF		1

/* Default configuration for PMIC (first level with alternate setting)*/
#define STM32_PM_DEFAULT		STM32_PM_CSTOP_ALLOW_LP_STOP

#endif /*__STM32MP_PM_H__*/

