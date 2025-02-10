// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2024, STMicroelectronics
 */

#include <assert.h>
#include <drivers/clk.h>
#include <drivers/clk_dt.h>
#include <drivers/regulator.h>
#include <kernel/dt.h>
#include <kernel/panic.h>
#include <libfdt.h>
#include <malloc.h>
#include <scmi_agent_configuration.h>
#include <scmi_pd_consumer.h>
#include <tee_api_defines_extensions.h>
#include <trace.h>

TEE_Result optee_scmi_server_init_pd(const void *fdt, int node,
				     struct scpfw_agent_config *agent_cfg,
				     struct scpfw_channel_config *channel_cfg)
{
	TEE_Result res = TEE_ERROR_GENERIC;
	int item_node = 0;
	int subnode = 0;
	bool have_subnodes = false;

	item_node = fdt_subnode_offset(fdt, node, "power-domains");
	if (item_node < 0)
		return TEE_SUCCESS;

	/* Compute the number of domains to allocate */
	fdt_for_each_subnode(subnode, fdt, item_node) {
		paddr_t reg = fdt_reg_base_address(fdt, subnode);

		assert(reg != DT_INFO_INVALID_REG);
		if (reg > channel_cfg->pd_count)
			channel_cfg->pd_count = reg;

		have_subnodes = true;
	}

	if (!have_subnodes)
		return TEE_SUCCESS;

	/* Number of SCMI power domains is the max domain ID + 1 */
	channel_cfg->pd_count++;
	channel_cfg->pd = calloc(channel_cfg->pd_count,
				 sizeof(*channel_cfg->pd));
	if (!channel_cfg->pd)
		return TEE_ERROR_OUT_OF_MEMORY;

	fdt_for_each_subnode(subnode, fdt, item_node) {
		struct regulator *regulator = NULL;
		const char *domain_name = NULL;
		const fdt32_t *cuint = NULL;
		struct clk *clock = NULL;
		uint32_t domain_id = 0;

		res = regulator_dt_get_supply(fdt, subnode, "voltd",
					      &regulator);
		if (res == TEE_ERROR_DEFER_DRIVER_INIT) {
			panic("Unexpected init deferral");
		} else if (res) {
			EMSG("Can't get regulator for power domain %s " \
			     "(%#"PRIx32"), skipped",
			     fdt_get_name(fdt, subnode, NULL), res);
			continue;
		}

		res = clk_dt_get_by_index(fdt, subnode, 0, &clock);
		if (res == TEE_ERROR_DEFER_DRIVER_INIT) {
			panic("Unexpected init deferral");
		} else if (res) {
			EMSG("Can't get clock for power domain %s " \
			     "(%#"PRIx32"), skipped",
			     fdt_get_name(fdt, subnode, NULL), res);
			continue;
		}

		domain_id = fdt_reg_base_address(fdt, subnode);

		cuint = fdt_getprop(fdt, subnode, "domain-name", NULL);
		if (cuint)
			domain_name = (char *)cuint;
		else
			domain_name = fdt_get_name(fdt, subnode, NULL);

		domain_name = strdup(domain_name);
		if (!domain_name)
			panic();

		channel_cfg->pd[domain_id] = (struct scmi_pd){
			.name = domain_name,
			.regu = regulator,
			.clk = clock,
		};

		DMSG("scmi power domain shares %s on domain ID %"PRIu32,
		     channel_cfg->pd->name, domain_id);
	}

	return TEE_SUCCESS;
}
