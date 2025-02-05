/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2020-2023, STMicroelectronics
 */

#ifndef SCMI_SERVER_REGULATOR_CONSUMER_H
#define SCMI_SERVER_REGULATOR_CONSUMER_H

#include <stdbool.h>
#include <stdint.h>
#include <tee_api_types.h>
#include <scmi_agent_configuration.h>

#ifdef CFG_SCMI_SERVER_REGULATOR_CONSUMER
/*
 * Initialize SCMI regulator from regulator consumer information from DT.
 * Returns a TEE_Result compliant value
 */
TEE_Result optee_scmi_server_init_regulators(const void *fdt, int node,
					     struct scpfw_agent_config
							*agent_cfg,
					     struct scpfw_channel_config
							*channel_cfg);
#else
static inline TEE_Result
optee_scmi_server_init_regulators(const void *fdt __unused, int node __unused,
				  struct scpfw_agent_config *agent_cfg __unused,
				  struct scpfw_channel_config
							*channel_cfg __unused)
{
	return TEE_SUCCESS;
}
#endif /* CFG_SCMI_SERVER_REGULATOR_CONSUMER */
#endif /* SCMI_SERVER_REGULATOR_CONSUMER_H */
