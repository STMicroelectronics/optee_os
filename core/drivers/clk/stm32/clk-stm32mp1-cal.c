// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2026, STMicroelectronics
 */

#include <arm.h>
#include <drivers/clk.h>
#include <drivers/clk_dt.h>
#include <drivers/counter.h>
#include <drivers/stm32mp_dt_bindings.h>
#ifdef CFG_STM32MP13
#include <drivers/stm32mp13_rcc.h>
#endif
#ifdef CFG_STM32MP15
#include <drivers/stm32mp1_rcc.h>
#endif
#include <initcall.h>
#include <keep.h>
#include <kernel/boot.h>
#include <kernel/dt.h>
#include <kernel/panic.h>
#include <kernel/pm.h>
#include <kernel/pseudo_ta.h>
#include <libfdt.h>
#include <limits.h>
#include <stm32_pta_calib.h>
#include <stm32_util.h>
#include <string.h>

#include "clk-calibration-core.h"

#define CSI_CALIBRATION_NAME		"csi-calibration"
#define HSI_CALIBRATION_NAME		"hsi-calibration"

#define OSC_CALIBRATION_COMPAT		"st,osc-calibration"
#define CSI_CALIBRATION_COMPAT		"st,csi-cal"
#define HSI_CALIBRATION_COMPAT		"st,hsi-cal"

#define PRESCAL_HSI			10
#define PRESCAL_CSI			7

#define RCC_HSICFGR_HSICAL_MASK_LIMITED	GENMASK_32(24, 16)

enum osc_calibration {
	HSI_CAL = 0,
	CSI_CAL,
	MAX_CAL,
};

/* List of forbidden values for HSI and CSI */
static const uint16_t fbv_hsi[] = {
	512, 480, 448, 416, 384, 352, 320, 288,
	256, 224, 192, 160, 128, 96, 64, 32, 0,
};

static const uint16_t fbv_csi[] = {
	256, 240, 224, 208, 192, 176, 160, 144,
	128, 112, 96, 80, 64, 48, 32, 16, 0,
};

struct counter_calib {
	struct counter_device *counter;
	struct clk *clock;
	void *config;
};

struct stm32mp1_clk_cal {
	struct stm32_calib_config data;

	TEE_Result (*init_cal)(struct stm32mp1_clk_cal *clk_cal);
	unsigned int prescaler;
	struct counter_calib cnt_cal;
};

static TEE_Result init_hsi_calibration(struct stm32mp1_clk_cal *clk_cal);
static TEE_Result init_csi_calibration(struct stm32mp1_clk_cal *clk_cal);

static unsigned long clk_stm32_read_hsi_frequency(void);
static unsigned long clk_stm32_read_csi_frequency(void);

struct stm32mp1_clk_cal osc_cal_device[MAX_CAL] = {
	[HSI_CAL] = {
		.data = {
			.compatible = HSI_CALIBRATION_COMPAT,
			.name = HSI_CALIBRATION_NAME,
			.reg = RCC_HSICFGR,
			.cal_mask = RCC_HSICFGR_HSICAL_MASK_LIMITED,
			.cal_shift = RCC_HSICFGR_HSICAL_SHIFT,
			.trim_mask = RCC_HSICFGR_HSITRIM_MASK,
			.trim_shift = RCC_HSICFGR_HSITRIM_SHIFT,
			.fbv = fbv_hsi,
			.trim_max = 63,
			.trim_min = -64,
			.margin_ratio = 5,
			.read_frequency = clk_stm32_read_hsi_frequency,
		},
		.init_cal = init_hsi_calibration,
		.prescaler = PRESCAL_HSI,

	},
	[CSI_CAL] = {
		.data = {
			.compatible = CSI_CALIBRATION_COMPAT,
			.name = CSI_CALIBRATION_NAME,
			.reg = RCC_CSICFGR,
			.cal_mask = RCC_CSICFGR_CSICAL_MASK,
			.cal_shift = RCC_CSICFGR_CSICAL_SHIFT,
			.trim_mask = RCC_CSICFGR_CSITRIM_MASK,
			.trim_shift = RCC_CSICFGR_CSITRIM_SHIFT,
			.fbv = fbv_csi,
			.trim_max = 31,
			.trim_min = 0,
			.margin_ratio = 8,
			.read_frequency = clk_stm32_read_csi_frequency,
		},
		.init_cal = init_csi_calibration,
		.prescaler = PRESCAL_CSI,
	},
};

static unsigned long clk_stm32_get_frequency(struct stm32mp1_clk_cal *clk_cal)
{
	TEE_Result res = TEE_ERROR_GENERIC;
	unsigned int ticks = 0;
	unsigned long freq = 0;
	struct counter_calib *cnt = &clk_cal->cnt_cal;
	struct stm32_calib_config *data __maybe_unused = &clk_cal->data;

	res = counter_get_value(cnt->counter, &ticks);
	if (res && !ticks)
		panic();

	/*
	 * Calibration uses timer in input mode.
	 * It uses the default timer clock to increase
	 * timer counter
	 * Warning: If a prescaler is set in timer, rate
	 * must be adapted accordingly.
	 */
	freq = (clk_get_rate(cnt->clock) / ticks) << clk_cal->prescaler;

	FMSG("%s Freq is %lu", data->name, freq);
	if (!freq)
		panic("Freq calculation error");

	return freq;
}

static unsigned long clk_stm32_read_hsi_frequency(void)
{
	return clk_stm32_get_frequency(&osc_cal_device[HSI_CAL]);
}

static unsigned long clk_stm32_read_csi_frequency(void)
{
	return clk_stm32_get_frequency(&osc_cal_device[CSI_CAL]);
}

static void stm32mp_start_clock_calib(void)
{
	unsigned int i = 0;

	for (i = 0; i < ARRAY_SIZE(osc_cal_device); i++) {
		struct stm32mp1_clk_cal *clk_cal = &osc_cal_device[i];
		struct stm32_calib_config *data = &clk_cal->data;
		struct counter_calib *cnt_cal = &clk_cal->cnt_cal;

		if (data->is_used) {
			TEE_Result res = TEE_ERROR_GENERIC;

			/* Start associated counter */
			res = counter_start(cnt_cal->counter,
					    cnt_cal->config);
			if (res)
				panic();

			stm32_calib_start_calibration(data);

			res = counter_stop(cnt_cal->counter);
			if (res)
				panic();
		} else {
			if (HSI_CAL == i)
				DMSG("HSI clock calibration not supported");
			else
				DMSG("CSI clock calibration not supported");
		}
	}
}

static void set_cal_counter(struct counter_calib *cnt_cal,
			    const void *fdt, int node)
{
	int phandle = 0;
	int counter_node = 0;
	const int *cuint = NULL;
	struct clk *clock = NULL;
	TEE_Result res = TEE_SUCCESS;

	cnt_cal->counter = fdt_counter_get(fdt, node, &cnt_cal->config);
	if (!cnt_cal->counter || !cnt_cal->config)
		panic();

	cuint = fdt_getprop(fdt, node, "counter", NULL);
	if (!cuint)
		panic();

	phandle = fdt32_to_cpu(*cuint);
	counter_node = fdt_node_offset_by_phandle(fdt, phandle);
	if (!counter_node)
		panic();

	res = clk_dt_get_by_index(fdt, counter_node, 0, &clock);
	if (res)
		panic();

	cnt_cal->clock = clock;

	FMSG("Counter freq is %li", clk_get_rate(cnt_cal->clock));
}

static TEE_Result init_hsi_calibration(struct stm32mp1_clk_cal *clk_cal)
{
	struct stm32_calib_config *data = &clk_cal->data;
	struct clk *hsi_clk = stm32mp_rcc_clock_id_to_clk(CK_HSI);

	data->ref_freq = clk_get_rate(hsi_clk);
	data->freq_margin = (data->ref_freq * (data->margin_ratio) / 1000);

	return TEE_SUCCESS;
}

static TEE_Result init_csi_calibration(struct stm32mp1_clk_cal *clk_cal)
{
	struct stm32_calib_config *data = &clk_cal->data;
	struct clk *csi_clk = stm32mp_rcc_clock_id_to_clk(CK_CSI);

	data->ref_freq = clk_get_rate(csi_clk);
	data->freq_margin = (data->ref_freq * (data->margin_ratio) / 1000);

	return TEE_SUCCESS;
}

static TEE_Result
stm32mp1_calib_pm(enum pm_op op, unsigned int pm_hint __unused,
		  const struct pm_callback_handle *hdl __unused)
{
	unsigned int i = 0;

	if (op == PM_OP_RESUME)
		return TEE_SUCCESS;

	/* SUSPEND */
	for (i = 0; i < ARRAY_SIZE(osc_cal_device); i++) {
		struct stm32_calib_config *data = &osc_cal_device[i].data;

		if (data->is_used)
			data->init_done = false;
	}

	return TEE_SUCCESS;
}
DECLARE_KEEP_PAGER_PM(stm32mp1_calib_pm);

static TEE_Result init_stm32mp1_calib(void)
{
	void *fdt = NULL;
	TEE_Result res = TEE_ERROR_GENERIC;
	int node = 0;
	int subnode = 0;
	unsigned int i = 0;

	fdt = get_embedded_dt();
	if (!fdt)
		panic();

	node = fdt_node_offset_by_compatible(fdt, -1, OSC_CALIBRATION_COMPAT);
	if (node < 0)
		panic();

	/*  Parse subnode to find the calibration */
	fdt_for_each_subnode(subnode, fdt, node) {
		const char *compat = NULL;
		int status = fdt_get_status(fdt, subnode);

		if (status == DT_STATUS_DISABLED)
			continue;

		compat = fdt_stringlist_get(fdt, subnode,
					    "compatible", 0, NULL);
		if (!compat)
			continue;

		for (i = 0; i < ARRAY_SIZE(osc_cal_device); i++) {
			struct stm32mp1_clk_cal *clk_cal = &osc_cal_device[i];
			struct stm32_calib_config *data = &clk_cal->data;
			struct counter_calib *cnt_cal = &clk_cal->cnt_cal;

			if (strcmp(data->compatible, compat) != 0)
				continue;

			DMSG("Calibration %s enabled",
			     fdt_get_name(fdt, subnode, NULL));

			data->is_used = true;

			/* Start counter */
			set_cal_counter(cnt_cal, fdt, subnode);

			res = counter_start(cnt_cal->counter,
					    cnt_cal->config);
			if (res)
				panic();

			res = clk_cal->init_cal(clk_cal);
			if (res) {
				EMSG("Calibration %s failed", data->name);
				panic();
			}

			res = counter_stop(cnt_cal->counter);
			if (res)
				panic();
		}
	}

	stm32mp_start_clock_calib();

	register_pm_driver_cb(stm32mp1_calib_pm, NULL, "service-calib");

	return TEE_SUCCESS;
}

service_init(init_stm32mp1_calib);

void stm32mp_start_clock_calibration(void)
{
	stm32mp_start_clock_calib();
}
