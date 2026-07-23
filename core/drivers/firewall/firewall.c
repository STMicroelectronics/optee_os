// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (C) 2024, STMicroelectronics
 */

#include <assert.h>
#include <config.h>
#include <drivers/firewall.h>
#include <kernel/dt_driver.h>
#include <kernel/panic.h>
#include <libfdt.h>
#include <malloc.h>
#include <stdio.h>
#include <trace.h>

/* The firewall framework requires device tree support */
static_assert(IS_ENABLED(CFG_DT));

static TEE_Result firewall_get(struct dt_pargs *parg, void *data,
			       struct firewall_query **out_fw)
{
	struct firewall_query *fw = NULL;
	unsigned int i = 0;

	assert(parg->args_count >= 0);

	fw = calloc(1, sizeof(*fw));
	if (!fw)
		return TEE_ERROR_OUT_OF_MEMORY;

	fw->ctrl = (struct firewall_controller *)data;
	fw->arg_count = parg->args_count;

	if (fw->arg_count) {
		fw->args = calloc(fw->arg_count, sizeof(*fw->args));
		if (!fw->args) {
			free(fw);
			return TEE_ERROR_OUT_OF_MEMORY;
		}
	}

	for (i = 0; i < (unsigned int)parg->args_count; i++)
		fw->args[i] = parg->args[i];

	*out_fw = fw;

	return TEE_SUCCESS;
}

/* Firewall device API */

void firewall_put(struct firewall_query *fw)
{
	if (fw) {
		free(fw->args);
		free(fw);
	}
}

void firewall_alternate_conf_put(struct firewall_alt_conf *conf)
{
	if (conf) {
		size_t i = 0;

		for (i = 0; i < conf->nb_queries; i++)
			firewall_put(conf->queries[i]);
		free(conf->queries);
		free(conf);
	}
}

TEE_Result firewall_dt_get_by_index(const void *fdt, int node, uint32_t index,
				    struct firewall_query **out_fw)
{
	return dt_driver_device_from_node_idx_prop("access-controllers", fdt,
						   node, index,
						   DT_DRIVER_FIREWALL,
						   out_fw);
}

TEE_Result firewall_dt_get_by_name(const void *fdt, int node, const char *name,
				   struct firewall_query **out_fw)
{
	int index = 0;

	index = fdt_stringlist_search(fdt, node, "access-controllers-names",
				      name);
	if (index == -FDT_ERR_NOTFOUND)
		return TEE_ERROR_ITEM_NOT_FOUND;
	else if (index < 0)
		return TEE_ERROR_GENERIC;

	return firewall_dt_get_by_index(fdt, node, index, out_fw);
}

TEE_Result firewall_dt_get_alternate_conf(const void *fdt, int node,
					  const char *conf_name,
					  struct firewall_alt_conf **out_conf)
{
	struct firewall_alt_conf *conf = NULL;
	TEE_Result res = TEE_ERROR_GENERIC;
	char *prop_name = NULL;
	size_t nb_element = 0;
	int max_len = 0;
	size_t i = 0;

	assert(conf_name && out_conf);

	max_len = strlen(conf_name) + strlen("access-controllers-conf-") + 1;
	prop_name = calloc(1, max_len);
	if (!prop_name)
		return TEE_ERROR_OUT_OF_MEMORY;

	(void)snprintf(prop_name, max_len, "access-controllers-conf-%s",
		       conf_name);
	res = dt_driver_count_devices(prop_name, fdt, node, DT_DRIVER_FIREWALL,
				      &nb_element);
	if (res) {
		if (res != TEE_ERROR_DEFER_DRIVER_INIT)
			EMSG("Could not count devices for %s conf in node %s",
			     prop_name, fdt_get_name(fdt, node, NULL));
		goto out;
	}

	if (!nb_element) {
		EMSG("No firewall alternate configuration: %s in node %s",
		     prop_name, fdt_get_name(fdt, node, NULL));
		res = TEE_ERROR_BAD_PARAMETERS;
		goto out;
	}

	conf = calloc(1, sizeof(*conf));
	if (!conf) {
		res = TEE_ERROR_OUT_OF_MEMORY;
		goto out;
	}

	conf->nb_queries = nb_element;
	conf->queries = calloc(nb_element, sizeof(*conf->queries));
	if (!conf->queries) {
		free(conf);
		res = TEE_ERROR_OUT_OF_MEMORY;
		goto out;
	}

	for (i = 0; i < nb_element; i++) {
		res = dt_driver_device_from_node_idx_prop(prop_name, fdt, node,
							  i, DT_DRIVER_FIREWALL,
							  conf->queries + i);
		if (res) {
			firewall_alternate_conf_put(conf);
			goto out;
		}
	}

	*out_conf = conf;
	res = TEE_SUCCESS;

out:
	free(prop_name);

	return res;
}

TEE_Result firewall_set_configuration(struct firewall_query *fw)
{
	assert(fw && fw->ctrl && fw->ctrl->ops);

	if (!fw->ctrl->ops->set_conf)
		return TEE_ERROR_NOT_SUPPORTED;

	return fw->ctrl->ops->set_conf(fw);
}

TEE_Result firewall_set_alternate_conf(struct firewall_alt_conf *conf)
{
	TEE_Result res = TEE_ERROR_GENERIC;
	size_t i = 0;

	assert(conf);

	for (i = 0; i < conf->nb_queries; i++) {
		res = firewall_set_configuration(conf->queries[i]);
		if (res)
			return res;
	}

	return TEE_SUCCESS;
}

TEE_Result firewall_set_memory_configuration(struct firewall_query *fw,
					     paddr_t paddr, size_t size)
{
	assert(fw && fw->ctrl && fw->ctrl->ops);

	if (!fw->ctrl->ops->set_memory_conf)
		return TEE_ERROR_NOT_SUPPORTED;

	return fw->ctrl->ops->set_memory_conf(fw, paddr, size);
}

TEE_Result firewall_set_memory_alternate_conf(struct firewall_alt_conf *conf,
					      paddr_t paddr, size_t size)
{
	TEE_Result res = TEE_ERROR_GENERIC;
	size_t i = 0;

	assert(conf);

	for (i = 0; i < conf->nb_queries; i++) {
		res = firewall_set_memory_configuration(conf->queries[i], paddr,
							size);
		if (res)
			return res;
	}

	return TEE_SUCCESS;
}

TEE_Result firewall_check_access(struct firewall_query *fw)
{
	assert(fw && fw->ctrl && fw->ctrl->ops);

	if (!fw->ctrl->ops->check_access)
		return TEE_ERROR_NOT_SUPPORTED;

	return fw->ctrl->ops->check_access(fw);
}

TEE_Result firewall_acquire_access(struct firewall_query *fw)
{
	assert(fw && fw->ctrl && fw->ctrl->ops);

	if (!fw->ctrl->ops->acquire_access)
		return TEE_ERROR_NOT_SUPPORTED;

	return fw->ctrl->ops->acquire_access(fw);
}

TEE_Result firewall_check_memory_access(struct firewall_query *fw,
					paddr_t paddr, size_t size, bool read,
					bool write)
{
	assert(fw && fw->ctrl && fw->ctrl->ops);

	if (!fw->ctrl->ops->check_memory_access)
		return TEE_ERROR_NOT_SUPPORTED;

	return fw->ctrl->ops->check_memory_access(fw, paddr, size, read, write);
}

TEE_Result firewall_acquire_memory_access(struct firewall_query *fw,
					  paddr_t paddr, size_t size, bool read,
					  bool write)
{
	assert(fw && fw->ctrl && fw->ctrl->ops);

	if (!fw->ctrl->ops->acquire_memory_access)
		return TEE_ERROR_NOT_SUPPORTED;

	return fw->ctrl->ops->acquire_memory_access(fw, paddr, size, read,
						    write);
}

void firewall_release_access(struct firewall_query *fw)
{
	assert(fw && fw->ctrl && fw->ctrl->ops);

	if (fw->ctrl->ops->release_access)
		fw->ctrl->ops->release_access(fw);
}

void firewall_release_memory_access(struct firewall_query *fw, paddr_t paddr,
				    size_t size, bool read, bool write)
{
	assert(fw && fw->ctrl && fw->ctrl->ops);

	if (fw->ctrl->ops->release_memory_access)
		fw->ctrl->ops->release_memory_access(fw, paddr, size, read,
						     write);
}

/* Firewall controller API */

TEE_Result firewall_dt_controller_register(const void *fdt, int node,
					   struct firewall_controller *ctrl)
{
	assert(ctrl);

	DMSG("Registering %s firewall controller", ctrl->name);

	return dt_driver_register_provider(fdt, node,
					   (get_of_device_func)firewall_get,
					   ctrl, DT_DRIVER_FIREWALL);
}
