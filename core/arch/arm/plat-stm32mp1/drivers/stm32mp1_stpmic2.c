// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2024, STMicroelectronics
 */

#include <config.h>
#include <drivers/regulator.h>
#include <drivers/stm32mp_dt_bindings.h>
#include <drivers/stpmic2.h>
#include <keep.h>
#include <kernel/pm.h>
#include <platform_config.h>
#include <trace.h>

#include "pm/power.h"

size_t plat_pmic2_get_lp_mode_count(void)
{
	return STM32_PM_MAX_SOC_MODE;
}

const char *plat_pmic2_get_lp_mode_name(int mode)
{
	return plat_get_lp_mode_name(mode);
}

static TEE_Result pmic_regu_pm(enum pm_op op, uint32_t pm_hint,
			       const struct pm_callback_handle *pm_handle)
{
	struct regulator *regulator = pm_handle->handle;
	unsigned int pwrlvl = PM_HINT_PLATFORM_STATE(pm_hint);

	if (op == PM_OP_SUSPEND)
		return stm32_pmic2_apply_pm_state(regulator, pwrlvl);
	else if (op == PM_OP_RESUME)
		return stm32_pmic2_resume_regulator(regulator);

	return TEE_SUCCESS;
}

DECLARE_KEEP_PAGER_PM(pmic_regu_pm);

TEE_Result plat_pmic2_supplied_init(struct regulator *regulator)
{
	register_pm_core_service_cb(pmic_regu_pm, regulator,
				    regulator_name(regulator));

	return TEE_SUCCESS;
}
