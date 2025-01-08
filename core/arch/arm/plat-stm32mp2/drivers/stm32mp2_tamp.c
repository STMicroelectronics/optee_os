// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2025, STMicroelectronics
 */

#include <drivers/stm32_tamp.h>
#include <drivers/stm32mp2_tamp.h>
#include <drivers/stm32mp25_pwr.h>
#if defined(CFG_STM32MP25) || defined(CFG_STM32MP23)
#include <drivers/stm32mp25_rcc.h>
#endif
#if defined(CFG_STM32MP21)
#include <drivers/stm32mp21_rcc.h>
#endif
#include <io.h>
#include <stm32_util.h>

/* Activate the SoC resources required by internal TAMPER */
TEE_Result stm32_activate_internal_tamper(int id)
{
	TEE_Result res = TEE_ERROR_NOT_SUPPORTED;

	switch (id) {
	case INT_TAMP1: /* Backup domain (V08CAP) voltage monitoring */
	case INT_TAMP2: /* Temperature monitoring */
		stm32mp_pwr_monitoring_enable(PWR_MON_V08CAP_TEMP);
		res = TEE_SUCCESS;
		break;

	case INT_TAMP3: /* LSE monitoring (LSECSS) */
		if (io_read32(stm32_rcc_base() + RCC_BDCR) & RCC_BDCR_LSECSSON)
			res = TEE_SUCCESS;
		break;

	case INT_TAMP4: /* HSE monitoring (CSS + over frequency detection) */
		if (io_read32(stm32_rcc_base() + RCC_OCENSETR) &
		    RCC_OCENSETR_HSECSSON)
			res = TEE_SUCCESS;
		break;

	case INT_TAMP7:
		if (IS_ENABLED(CFG_STM32MP21)) {
			/* ADC2 (adc2_awd1) analog watchdog monitoring 1 */
			res = TEE_SUCCESS;
			break;
		} else if (IS_ENABLED(CFG_STM32MP23) ||
			   IS_ENABLED(CFG_STM32MP25)) {
			/* VDDCORE monitoring under/over voltage */
			stm32mp_pwr_monitoring_enable(PWR_MON_VCORE);
			res = TEE_SUCCESS;
			break;
		}
		break;

	case INT_TAMP12:
		if (IS_ENABLED(CFG_STM32MP21)) {
			/* ADC2 (adc2_awd2) analog watchdog monitoring 2 */
			res = TEE_SUCCESS;
			break;
		} else if (IS_ENABLED(CFG_STM32MP23) ||
			   IS_ENABLED(CFG_STM32MP25)) {
			/* VDDCPU (Cortex A35) monitoring under/over voltage */
			stm32mp_pwr_monitoring_enable(PWR_MON_VCPU);
			res = TEE_SUCCESS;
			break;
		}
		break;

	case INT_TAMP13:
	case INT_TAMP16:
		if (IS_ENABLED(CFG_STM32MP21))
			res = TEE_SUCCESS;
		break;

	case INT_TAMP5:
	case INT_TAMP6:
	case INT_TAMP8:
	case INT_TAMP9:
	case INT_TAMP10:
	case INT_TAMP11:
	case INT_TAMP14:
	case INT_TAMP15:
		res = TEE_SUCCESS;
		break;

	default:
		break;
	}

	return res;
}
