// SPDX-License-Identifier: (GPL-2.0-or-later OR BSD-3-Clause)
/*
 * Copyright (C) 2023, STMicroelectronics - All Rights Reserved
 */

#include <config.h>
#include <drivers/clk.h>
#include <drivers/clk_dt.h>
#include <drivers/stm32_gpio.h>
#include <kernel/dt.h>
#include <kernel/pm.h>
#include <stdio.h>
#include <stdlib.h>
#include <tee_api_defines_extensions.h>

struct stm32mp_rcc_clock {
	struct clk *clk;

	SLIST_ENTRY(stm32mp_rcc_clock) link;
};

static SLIST_HEAD(, stm32mp_rcc_clock) stm32mp_rcc_list =
	SLIST_HEAD_INITIALIZER(stm32mp_rcc_list);
static struct pinctrl_state *rcc_pinctrl;

static TEE_Result
stm32mp_rcc_pm(enum pm_op op, unsigned int pm_hint __unused,
	       const struct pm_callback_handle *pm_handle __unused)
{
	struct stm32mp_rcc_clock *rcc_clock = NULL;

	if (op == PM_OP_RESUME && rcc_pinctrl &&
	    pinctrl_apply_state(rcc_pinctrl))
		panic();

	SLIST_FOREACH(rcc_clock, &stm32mp_rcc_list, link) {
		if (op == PM_OP_RESUME) {
			if (clk_enable(rcc_clock->clk)) {
				EMSG("Can't enable %s clock",
				     rcc_clock->clk->name);
				panic();
			}
		} else {
			clk_disable(rcc_clock->clk);
		}
	}

	return TEE_SUCCESS;
}

static TEE_Result stm32mp_rcc_probe(const void *fdt, int node,
				    const void *compat_data __unused)
{
	TEE_Result res = TEE_SUCCESS;
	struct clk *clk = NULL;
	int idx = 0;

	/* Apply pinctrl configuration if present */
	res = pinctrl_get_state_by_idx(fdt, node, 0, &rcc_pinctrl);
	if (res && res != TEE_ERROR_ITEM_NOT_FOUND) {
		if (res == TEE_ERROR_DEFER_DRIVER_INIT)
			return res;
		panic("Failed to get RCC pinctrl");
	}

	if (rcc_pinctrl) {
		res = pinctrl_apply_state(rcc_pinctrl);
		if (res)
			return res;
	}

	/* Enable clock(s) if present */
	while (!clk_dt_get_by_index(fdt, node, idx++, &clk)) {
		struct stm32mp_rcc_clock *rcc_clock = NULL;

		if (clk_enable(clk))
			panic("Can't enable RCC clock");

		rcc_clock = calloc(1, sizeof(*rcc_clock));
		if (!rcc_clock)
			panic("Out of memory");

		rcc_clock->clk = clk;

		SLIST_INSERT_HEAD(&stm32mp_rcc_list, rcc_clock, link);
	}

	register_pm_core_service_cb(stm32mp_rcc_pm, NULL, "stm32mp-rcc");

	return TEE_SUCCESS;
}

static const struct dt_device_match stm32mp_rcc_match_table[] = {
	{ .compatible = "st,rcc" },
	{ }
};

DEFINE_DT_DRIVER(stm32mp_rcc_dt_driver) = {
	.name = "stm32mp-rcc",
	.match_table = stm32mp_rcc_match_table,
	.probe = stm32mp_rcc_probe,
};
