// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2017-2022, STMicroelectronics
 * Copyright (c) 2016-2018, Linaro Limited
 */

#include <drivers/clk_dt.h>
#include <drivers/counter.h>
#include <kernel/boot.h>
#include <kernel/pm.h>

/* Percent rate of HSE event */
#define HSE_THRESHOLD_PERCENT	110
#define FREQ_MONITOR_COMPAT	"st,freq-monitor"

static void stm32_hse_over_frequency(void *priv,
				     enum counter_event_type event __unused)
{
	uint32_t __maybe_unused *ticks = priv;

	EMSG("HSE over frequency detected: nb ticks:%"PRIu32, *ticks);
}
DECLARE_KEEP_PAGER(stm32_hse_over_frequency);

static TEE_Result stm32_hse_monitoring(void)
{
	struct counter_device *counter = NULL;
	struct clk *hse_clk = NULL;
	struct clk *hsi_clk = NULL;
	unsigned long hse = 0;
	unsigned long hsi_cal = 0;
	uint32_t ticks = 0;
	void *fdt = NULL;
	void *config = NULL;
	int node = -1;
	TEE_Result res = TEE_ERROR_GENERIC;

	fdt = get_embedded_dt();
	if (!fdt)
		panic();

	node = fdt_node_offset_by_compatible(fdt, 0, FREQ_MONITOR_COMPAT);
	if (node == -FDT_ERR_NOTFOUND)
		return TEE_SUCCESS;
	if (node < 0)
		panic();

	if (fdt_get_status(fdt, node) == DT_STATUS_DISABLED)
		return TEE_SUCCESS;

	res = clk_dt_get_by_name(fdt, node, "hse", &hse_clk);
	if  (res)
		return res;

	res = clk_dt_get_by_name(fdt, node, "hsi", &hsi_clk);
	if  (res)
		return res;

	hse = clk_get_rate(hse_clk);
	hsi_cal = clk_get_rate(hsi_clk);

	/*
	 * hsi_cal is based on hsi & DIVISOR
	 * DIVISOR is fixed (1024)
	 */
	hsi_cal /= 1024;

	ticks = (hse / 100) * HSE_THRESHOLD_PERCENT;
	ticks /= hsi_cal;

	DMSG("HSE:%luHz HSI cal:%luHz event:%"PRIu32, hse, hsi_cal, ticks);

	counter = fdt_counter_get(fdt, node, &config);
	assert(counter && config);

	res = counter_set_threshold(counter, ticks);
	if  (res)
		return res;

	res = counter_enable_event(counter, COUNTER_EVENT_THRESHOLD,
				   stm32_hse_over_frequency,
				   (void *)&ticks);
	if  (res)
		return res;

	res = counter_start(counter, config);
	if  (res)
		return res;

	return TEE_SUCCESS;
}

driver_init_late(stm32_hse_monitoring);
