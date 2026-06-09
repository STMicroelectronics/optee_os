/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017-2018, STMicroelectronics
 */
#ifndef __STM32MP_PM_H__
#define __STM32MP_PM_H__

#include <dt-bindings/power/stm32mp1-power.h>

/* Default configuration for PMIC (first level with alternate setting)*/
#define STM32_PM_DEFAULT		STM32_PM_CSTOP_ALLOW_LP_STOP
/* Power OFF configuration for PMIC */
 #define STM32_PM_SWITCH_OFF		STM32_PM_CSTANDBY_ALLOW_STANDBY_DDR_OFF

#endif /*__STM32MP_PM_H__*/

