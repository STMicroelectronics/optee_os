// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2020-2021, STMicroelectronics
 */

#include <assert.h>
#include <drivers/regulator.h>
#include <drivers/scmi-msg.h>
#include <drivers/scmi.h>
#include <initcall.h>
#include <kernel/boot.h>
#include <kernel/dt.h>
#include <kernel/panic.h>
#include <libfdt.h>
#include <malloc.h>
#include <trace.h>

#define VERBOSE_VOLTD(_chan_id, _dom_id, fmt, ...) \
	FMSG("(channel %u/domain %u) " fmt, (_chan_id), (_dom_id), __VA_ARGS__)

struct scmi_voltd {
	uint32_t domain_id;
	struct regulator *regulator;
	bool enabled;
};

struct scmi_voltd_channel {
	unsigned int channel_id;
	struct scmi_voltd *voltd_array;
	size_t voltd_count;
};

struct scmi_voltd_channel *voltd_channel;
static size_t voltd_channel_count;

static struct scmi_voltd_channel *channel_from_id(unsigned int channel_id)
{
	size_t i = 0;

	for (i = 0; i < voltd_channel_count; i++)
		if (channel_id == voltd_channel[i].channel_id)
			return voltd_channel + i;

	return NULL;
}

static struct scmi_voltd *voltd_from_id(unsigned int channel_id,
					unsigned int domain_id)
{
	struct scmi_voltd_channel *chan = channel_from_id(channel_id);

	if (!chan || domain_id >= chan->voltd_count)
		return NULL;

	return chan->voltd_array + domain_id;
}

size_t plat_scmi_voltd_count(unsigned int channel_id)
{
	struct scmi_voltd_channel *chan = channel_from_id(channel_id);

	if (chan)
		return chan->voltd_count;

	return 0;
}

const char *plat_scmi_voltd_get_name(unsigned int channel_id,
				     unsigned int domain_id)
{
	struct scmi_voltd *voltd = voltd_from_id(channel_id, domain_id);

	if (!voltd || !voltd->regulator)
		return NULL;

	return regulator_name(voltd->regulator);
}

int32_t plat_scmi_voltd_levels_array(unsigned int channel_id,
				     unsigned int domain_id,
				     size_t start_index,
				     long *microvolt, size_t *nb_elts)
{
	struct scmi_voltd *voltd = voltd_from_id(channel_id, domain_id);
	struct regulator_voltages_desc *desc = NULL;
	TEE_Result res = TEE_ERROR_GENERIC;
	const int *levels = NULL;
	size_t out_count = 0;
	size_t i = 0;

	if (!voltd)
		return SCMI_NOT_FOUND;
	if (!voltd->regulator)
		return SCMI_DENIED;

	res = regulator_supported_voltages(voltd->regulator, &desc, &levels);
	if (res)
		return SCMI_GENERIC_ERROR;

	if (desc->type != VOLTAGE_TYPE_FULL_LIST)
		return SCMI_NOT_SUPPORTED;

	if (!microvolt) {
		*nb_elts = desc->num_levels - start_index;

		return SCMI_SUCCESS;
	}

	if (SUB_OVERFLOW(desc->num_levels, start_index, &out_count))
		return SCMI_GENERIC_ERROR;

	out_count = MIN(out_count, *nb_elts);

	VERBOSE_VOLTD(channel_id, domain_id,
		      "%zu levels, start %zu req %zu out %zu",
		      desc->num_levels, start_index, *nb_elts, out_count);

	for (i = 0; i < out_count; i++)
		microvolt[i] = levels[start_index + i];

	*nb_elts = out_count;

	return SCMI_SUCCESS;
}

int32_t plat_scmi_voltd_get_level(unsigned int channel_id,
				  unsigned int domain_id, long *level)
{
	struct scmi_voltd *voltd = voltd_from_id(channel_id, domain_id);

	if (!voltd)
		return SCMI_NOT_FOUND;
	if (!voltd->regulator)
		return SCMI_DENIED;

	*level = regulator_get_voltage(voltd->regulator);

	VERBOSE_VOLTD(channel_id, domain_id, "get voltage = %lduV", *level);

	return SCMI_SUCCESS;
}

int32_t plat_scmi_voltd_set_level(unsigned int channel_id,
				  unsigned int domain_id,
				  long microvolt)
{
	struct scmi_voltd *voltd = voltd_from_id(channel_id, domain_id);

	if (!voltd)
		return SCMI_NOT_FOUND;
	if (!voltd->regulator)
		return SCMI_DENIED;

	VERBOSE_VOLTD(channel_id, domain_id, "set voltage to %lduV", microvolt);

	if (regulator_set_voltage(voltd->regulator, microvolt))
		return SCMI_GENERIC_ERROR;

	return SCMI_SUCCESS;
}

int32_t plat_scmi_voltd_get_config(unsigned int channel_id,
				   unsigned int domain_id,
				   uint32_t *config)
{
	struct scmi_voltd *voltd = voltd_from_id(channel_id, domain_id);

	if (!voltd)
		return SCMI_NOT_FOUND;
	if (!voltd->regulator)
		return SCMI_DENIED;

	if (voltd->enabled)
		*config = SCMI_VOLTAGE_DOMAIN_CONFIG_ARCH_ON;
	else
		*config = SCMI_VOLTAGE_DOMAIN_CONFIG_ARCH_OFF;

	VERBOSE_VOLTD(channel_id, domain_id, "config %#"PRIx32, *config);

	return SCMI_SUCCESS;
}

int32_t plat_scmi_voltd_set_config(unsigned int channel_id,
				   unsigned int domain_id,
				   uint32_t config)
{
	struct scmi_voltd *voltd = voltd_from_id(channel_id, domain_id);

	if (!voltd)
		return SCMI_NOT_FOUND;
	if (!voltd->regulator)
		return SCMI_DENIED;

	VERBOSE_VOLTD(channel_id, domain_id, "set config to %u", config);

	switch (config) {
	case SCMI_VOLTAGE_DOMAIN_CONFIG_ARCH_ON:
		if (!voltd->enabled) {
			if (regulator_enable(voltd->regulator))
				return SCMI_GENERIC_ERROR;

			voltd->enabled = true;
		}

		return TEE_SUCCESS;
	case SCMI_VOLTAGE_DOMAIN_CONFIG_ARCH_OFF:
		if (voltd->enabled) {
			if (regulator_disable(voltd->regulator))
				return SCMI_GENERIC_ERROR;

			voltd->enabled = false;
		}

		return SCMI_SUCCESS;
	default:
		return SCMI_INVALID_PARAMETERS;
	}
}

static TEE_Result scmi_regulator_consumer_init(void)
{
	int dev_node = 0;
	int node = 0;
	void *fdt = NULL;
	const fdt32_t *cuint = NULL;
	unsigned int channel_id = 0;
	struct scmi_voltd_channel *chan = NULL;
	TEE_Result res = TEE_ERROR_GENERIC;

	fdt = get_embedded_dt();
	if (!fdt)
		return TEE_SUCCESS;

	dev_node = fdt_node_offset_by_compatible(fdt, -1,
						 "st,scmi-regulator-consumer");
	if (dev_node < 0)
		return TEE_SUCCESS;

	/* Currently supports a single device */
	if (fdt_node_offset_by_compatible(fdt, dev_node,
					  "st,scmi-regulator-consumer") !=
	    -FDT_ERR_NOTFOUND)
		panic();

	cuint = fdt_getprop(fdt, dev_node, "scmi-channel-id", NULL);
	if (!cuint) {
		DMSG("Missing property scmi-channel-id");
		panic();
	}
	channel_id = fdt32_to_cpu(*cuint);

	assert(!voltd_channel_count && !voltd_channel);
	voltd_channel_count = 1;
	voltd_channel = calloc(voltd_channel_count, sizeof(*voltd_channel));
	assert(voltd_channel);

	chan = voltd_channel;
	chan->channel_id = channel_id;

	/* Compute the number of domains to allocate */
	fdt_for_each_subnode(node, fdt, dev_node) {
		paddr_t reg = 0;

		/* Get an ID for the domain */
		reg = fdt_reg_base_address(fdt, node);
		assert(reg != DT_INFO_INVALID_REG);

		if ((uint32_t)reg >= chan->voltd_count)
			chan->voltd_count = (uint32_t)reg + U(1);
	}

	chan->voltd_array = calloc(chan->voltd_count,
				   sizeof(struct scmi_voltd));
	if (!chan->voltd_array)
		return TEE_ERROR_OUT_OF_MEMORY;

	fdt_for_each_subnode(node, fdt, dev_node) {
		struct scmi_voltd *voltd = NULL;
		struct regulator *regulator = NULL;
		uint32_t domain_id = 0;

		res = regulator_dt_get_supply(fdt, node, "voltd", &regulator);
		if (res) {
			EMSG("Regulator not found (%#"PRIx32") for voltd %s, skipped",
			     res, fdt_get_name(fdt, node, NULL));
			continue;
		}

		/* Get an ID for the domain */
		domain_id = (uint32_t)fdt_reg_base_address(fdt, node);
		voltd = chan->voltd_array + domain_id;

		/* Check that the domain_id is not already used */
		if (voltd->regulator) {
			EMSG("Domain ID %"PRIu32" already used", domain_id);
			panic();
		}

		voltd->regulator = regulator;

		/*
		 * Synchronize SCMI regulator current configuration
		 * Boot-on can be disabled by non secure
		 * Always-on can not be updated but status will be synchronized
		 * in non secure.
		 */
		if (regulator->flags & REGULATOR_BOOT_ON ||
		    regulator->flags & REGULATOR_ALWAYS_ON) {
			if (regulator_enable(regulator))
				IMSG("Failed to enable SCMI regul");
			else
				voltd->enabled = true;
		}

		DMSG("scmi voltd shares %s on domain ID %"PRIu32,
		     regulator_name(regulator), domain_id);
	}

	return TEE_SUCCESS;
}
driver_init_late(scmi_regulator_consumer_init);
