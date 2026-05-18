/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2022-2024, STMicroelectronics
 */

#ifndef DRIVERS_STM32_CPU_OPP_H
#define DRIVERS_STM32_CPU_OPP_H

#include <tee_api_types.h>

/* Get the actual number of CPU operating points */
unsigned int stm32_cpu_opp_count(void);

#ifdef CFG_SCPFW_MOD_DVFS
#include <scmi_agent_configuration.h>
/*
 * Initialize SCMI performance protocol with DVFS information
 * Returns a TEE_Result compliant value
 */
TEE_Result optee_scmi_server_cpu_dvfs(int perf_id,
				      struct scpfw_channel_config *channel_cfg);

/* Request to switch to operating point related to @rate */
TEE_Result stm32_cpu_opp_set_rate(unsigned int rate);

/* Get rate related to current operating point */
unsigned int stm32_cpu_opp_get_rate(void);

/* Request to operating point related to @level */
TEE_Result stm32_cpu_opp_get_rate_for_level(unsigned int level,
					    unsigned int *rate);

/* Request number of operating point */
TEE_Result stm32_gpu_opp_get_count(void);

/* Function used by CFG_SCPFW_MOD_DVFS to manage OPP on several domain */
#define OPP_ID_CPU		1

static inline TEE_Result stm32_opp_set_rate(unsigned int opp_id,
					    unsigned int rate)
{
	if (opp_id == OPP_ID_CPU)
		return stm32_cpu_opp_set_rate(rate);
	return TEE_ERROR_NOT_SUPPORTED;
}

static inline unsigned int stm32_opp_get_rate(unsigned int opp_id)
{
	if (opp_id == OPP_ID_CPU)
		return stm32_cpu_opp_get_rate();
	return 0;
}

static inline unsigned int stm32_opp_get_count(unsigned int opp_id)
{
	if (opp_id == OPP_ID_CPU)
		return stm32_cpu_opp_count();
	return 0;
}

static inline unsigned int stm32_opp_get_rate_for_level(unsigned int opp_id,
							unsigned int level,
							unsigned int *rate)
{
	if (opp_id == OPP_ID_CPU)
		return stm32_cpu_opp_get_rate_for_level(level, rate);
	return 0;
}

#else

/* Get sustained frequency level */
unsigned int stm32_cpu_opp_sustained_level(void);

/* Get level value identifying CPU operating point @opp_index */
unsigned int stm32_cpu_opp_level(unsigned int opp);

/* Request to switch to CPU operating point related to @level */
TEE_Result stm32_cpu_opp_set_level(unsigned int level);

/* Get level (Freq KHz) related to current CPU operating point */
TEE_Result stm32_cpu_opp_read_level(unsigned int *level);
#endif /*CFG_SCPFW_MOD_DVFS*/

#endif /*DRIVERS_STM32_CPU_OPP_H*/
