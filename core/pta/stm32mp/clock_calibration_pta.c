// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (C) 2026, STMicroelectronics - All Rights Reserved
 */

#include <kernel/pseudo_ta.h>
#include <kernel/user_access.h>
#include <kernel/user_mode_ctx.h>
#include <kernel/user_ta.h>
#include <stm32_pta_calib.h>
#include <stm32_util.h>

typedef TEE_Result (*ta_func)(struct ts_session *s, uint32_t param_types,
			      TEE_Param params[TEE_NUM_PARAMS]);

void __weak stm32mp_start_clock_calibration(void)
{
}

static TEE_Result request_calibration(struct ts_session *s __unused,
				      uint32_t param_types __unused,
				      TEE_Param params[TEE_NUM_PARAMS] __unused)
{
	stm32mp_start_clock_calibration();

	return TEE_SUCCESS;
}

static const ta_func ta_funcs[] = {
	[PTA_CALIB_REQUEST] = request_calibration,
};

static TEE_Result calib_invoke_command(void *sess_ctx __unused, uint32_t cmd_id,
				       uint32_t param_types,
				       TEE_Param params[TEE_NUM_PARAMS])
{
	struct ts_session *s = ts_get_calling_session();

	DMSG("Invoking calibration command");

	if (cmd_id < ARRAY_SIZE(ta_funcs) && ta_funcs[cmd_id])
		return ta_funcs[cmd_id](s, param_types, params);

	return TEE_ERROR_NOT_IMPLEMENTED;
}

pseudo_ta_register(.uuid = PTA_CALIB_UUID, .name = "calib",
		   .flags = PTA_DEFAULT_FLAGS,
		   .invoke_command_entry_point = calib_invoke_command);
