// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2020-2023, STMicroelectronics
 */

#include <assert.h>
#include <drivers/regulator.h>
#include <kernel/dt.h>
#include <kernel/panic.h>
#include <libfdt.h>
#include <malloc.h>
#include <scmi_agent_configuration.h>
#include <scmi_regulator_consumer.h>
#include <tee_api_defines_extensions.h>
#include <trace.h>

/*
 * struct scmi_server_regu: data for a SCMI regulator in DT
 *
 * @domain_id: SCMI domain identifier
 * @domain_name: SCMI domain name
 * @regulator: regulator to control through SCMI protocol
 * @enabled: if regulator is enabled by default or not
 */
struct scmi_server_regu {
	uint32_t domain_id;
	const char *domain_name;
	struct regulator *regulator;
	bool enabled;
};

TEE_Result optee_scmi_server_init_regulators(const void *fdt, int node,
					     struct scpfw_agent_config
							*agent_cfg,
					     struct scpfw_channel_config
							*channel_cfg)
{
	TEE_Result res = TEE_ERROR_GENERIC;
	struct scmi_server_regu *s_regu = NULL;
	size_t voltd_domain_count = 0;
	size_t s_regu_count = 0;
	int item_node = 0;
	int subnode = 0;
	bool have_subnodes = false;
	size_t n = 0;

	item_node = fdt_subnode_offset(fdt, node, "regulators");
	if (item_node < 0)
		return TEE_SUCCESS;

	/* Compute the number of domains to allocate */
	fdt_for_each_subnode(subnode, fdt, item_node) {
		paddr_t reg = fdt_reg_base_address(fdt, subnode);

		assert(reg != DT_INFO_INVALID_REG);
		if (reg > voltd_domain_count)
			voltd_domain_count = reg;

		have_subnodes = true;
	}

	if (!have_subnodes)
		return TEE_SUCCESS;

	/* Number of SCMI voltage domains is the max domain ID + 1 */
	s_regu_count = voltd_domain_count + 1;

	s_regu = calloc(s_regu_count, sizeof(*s_regu));
	if (!s_regu)
		return TEE_ERROR_OUT_OF_MEMORY;

	fdt_for_each_subnode(subnode, fdt, item_node) {
		struct scmi_server_regu *regu = NULL;
		struct regulator *regulator = NULL;
		const fdt32_t *cuint = NULL;
		uint32_t domain_id = 0;

		res = regulator_dt_get_supply(fdt, subnode, "voltd",
					      &regulator);
		if (res == TEE_ERROR_DEFER_DRIVER_INIT) {
			panic("Unexpected init deferral");
		} else if (res) {
			EMSG("Can't get regulator for voltd %s" \
			     "(%#"PRIx32"), skipped",
			     fdt_get_name(fdt, subnode, NULL), res);
			continue;
		}

		domain_id = fdt_reg_base_address(fdt, subnode);
		regu = s_regu + domain_id;
		regu->domain_id = domain_id;

		cuint = fdt_getprop(fdt, subnode, "domain-name", NULL);
		if (cuint)
			regu->domain_name = (char *)cuint;
		else
			regu->domain_name = regulator_name(regulator);

		/* Check that the domain_id is not already used */
		if (regu->regulator) {
			EMSG("Domain ID %"PRIu32" already used", domain_id);
			panic();
		}
		regu->regulator = regulator;

		/*
		 * Synchronize SCMI regulator current configuration
		 * Boot-on can be disabled by non secure
		 * Always-on can not be updated but status will be synchronized
		 * in non secure.
		 */
		if (regulator->flags & REGULATOR_ALWAYS_ON)
			regu->enabled = true;

		if (regulator->flags & REGULATOR_BOOT_ON) {
			if (regulator_enable(regulator))
				IMSG("Can't enable SCMI voltage regulator %s",
				     regulator_name(regulator));
			else
				regu->enabled = true;
		}

		DMSG("scmi voltd shares %s on domain ID %"PRIu32,
		     regu->domain_name, domain_id);
	}

	for (n = 0; n < s_regu_count; n++) {
		/*
		 * Assign domain IDs to un-exposed regulator as SCMI
		 * specification requires the resource is defined even if not
		 * accessible.
		 */
		if (!s_regu[n].regulator) {
			s_regu[n].domain_id = n;
			s_regu[n].domain_name = "";
		}

		s_regu[n].domain_name = strdup(s_regu[n].domain_name);
		if (!s_regu[n].domain_name)
			panic();
	}

	if (channel_cfg->voltd) {
		EMSG("Voltage domain already loaded: agent %u, channel %u",
		     agent_cfg->agent_id, channel_cfg->channel_id);
		panic();
	}

	channel_cfg->voltd_count = s_regu_count;
	channel_cfg->voltd = calloc(channel_cfg->voltd_count,
				    sizeof(*channel_cfg->voltd));
	if (!channel_cfg->voltd) {
		free(s_regu);
		return TEE_ERROR_OUT_OF_MEMORY;
	}

	for (n = 0; n < s_regu_count; n++) {
		unsigned int domain_id = s_regu[n].domain_id;

		if (s_regu[n].regulator) {
			channel_cfg->voltd[domain_id] = (struct scmi_voltd){
				.name = s_regu[n].domain_name,
				.regulator = s_regu[n].regulator,
				.enabled = s_regu[n].enabled,
			};
		}
	}

	/*
	 * We can free s_regu resources since content is now
	 * referenced from the SCMI server configuration data.
	 */
	free(s_regu);

	return TEE_SUCCESS;
}
