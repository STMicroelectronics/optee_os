// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2026, STMicroelectronics
 */

#include <drivers/clk.h>
#include <drivers/stm32mp_dt_bindings.h>
#ifdef CFG_STM32MP21_CLK
#include <drivers/stm32mp21_rcc.h>
#else
#include <drivers/stm32mp25_rcc.h>
#endif
#include <io.h>
#include <keep.h>
#include <kernel/delay.h>
#include <kernel/dt.h>
#include <kernel/interrupt.h>
#include <kernel/panic.h>
#include <kernel/pm.h>
#include <kernel/pseudo_ta.h>
#include <libfdt.h>
#include <stm32_pta_calib.h>
#include <stm32_util.h>

#include "clk-calibration-core.h"

#define HSI_CALIBRATION_NAME		"hsi-calibration"
#define MSI_CALIBRATION_NAME		"msi-calibration"
#define HSI_CALIBRATION_COMPAT		"st,hsi-cal"
#define MSI_CALIBRATION_COMPAT		"st,msi-cal"

#define READ_FREQUENCY_TIMEOUT_US	100000

int calibration_on_going;

struct stm32_calib_config_irq {
	struct itr_handler *itr;
	int irq;
	uint32_t irq_type;
	uint32_t irq_prio;
	unsigned int hsi_dev;
	unsigned int hsi_ref;
};

enum osc_calibration {
	HSI_CAL = 0,
	MSI_CAL,
	MAX_CAL,
};

/* List of forbidden values for HSI, 0 terminated */
static const uint16_t fbv_hsi[] = {
	512, 480, 448, 416, 384, 352, 320, 288,
	256, 224, 192, 160, 128, 96, 64, 32, 0,
};

/* List of forbidden values for MSI, 0 terminated */
static const uint16_t fbv_msi[] = {
	256, 240, 224, 208, 192, 176, 160, 144,
	128, 112, 96, 80, 64, 48, 32, 16, 0,
};

struct stm32mp2_clk_cal {
	struct stm32_calib_config data;

	TEE_Result (*init_cal)(struct stm32mp2_clk_cal *clk_cal);
	struct stm32_calib_config_irq irq_cal;
};

static TEE_Result init_hsi_calibration(struct stm32mp2_clk_cal *clk_cal);
static TEE_Result init_msi_calibration(struct stm32mp2_clk_cal *clk_cal);

static unsigned long clk_stm32_read_hsi_frequency(void);
static unsigned long clk_stm32_read_msi_frequency(void);

struct stm32mp2_clk_cal osc_cal_device[MAX_CAL] = {
	[HSI_CAL] = {
		.data = {
			.compatible = HSI_CALIBRATION_COMPAT,
			.name = HSI_CALIBRATION_NAME,
			.reg = RCC_HSICFGR,
			.cal_mask = RCC_HSICFGR_HSICAL_MASK,
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

	},
	[MSI_CAL] = {
		.data = {
			.compatible = MSI_CALIBRATION_COMPAT,
			.name = MSI_CALIBRATION_NAME,
			.reg = RCC_MSICFGR,
			.cal_mask = RCC_MSICFGR_MSICAL_MASK,
			.cal_shift = RCC_MSICFGR_MSICAL_SHIFT,
			.trim_mask = RCC_MSICFGR_MSITRIM_MASK,
			.trim_shift = RCC_MSICFGR_MSITRIM_SHIFT,
			.fbv = fbv_msi,
			.trim_max = 31,
			.trim_min = 0,
			.margin_ratio = 8,
			.read_frequency = clk_stm32_read_msi_frequency,
		},
		.init_cal = init_msi_calibration,
	},
};

static struct clk *clk_stm32_clock_frequency_calculator_get_ref(void)
{
	uint32_t val = 0;

	val = io_read32(stm32_rcc_base() + RCC_FCALCREFCFGR) &
	      RCC_FCALCREFCFGR_FCALCREFCKSEL_MASK;

	if (val > 0x4)
		panic();

	return stm32mp_rcc_clock_id_to_clk(PLL4_CK + val);
}

static unsigned long clk_stm32_clock_frequency_calculator(uint32_t ckintsel)
{
	struct clk *ref_clk = clk_stm32_clock_frequency_calculator_get_ref();
	struct clk *ref_parent = NULL;
	unsigned long freq = 0UL;
	unsigned long ref_freq = 0UL;
	uintptr_t rcc_base = stm32_rcc_base();
	uint32_t fcalctwc_val = 0xf;
	uint32_t min_fcalctwc = 1;
	uint32_t regval = 0;
	bool freq_meas_complete_flag = false;
	bool fcalcsts_bit = false;
	TEE_Result fstatus = TEE_SUCCESS;

	if (ref_clk)
		ref_parent = clk_get_parent(ref_clk);

	if (!ref_clk || !ref_parent) {
		EMSG("Failed to get reference clock for calibration");
		panic();
	}

	ref_freq = clk_get_rate(ref_parent);

	while ((fcalctwc_val >= min_fcalctwc) && !freq_meas_complete_flag) {
		/*
		 * Set the FCALCRSTN bit in the RCC Clock Frequency Calculator
		 * and Observation 1 clock Configuration Register
		 * (RCC_FCALCOBS1CFGR) to enable the clock frequency calculator.
		 */
		io_setbits32(rcc_base + RCC_FCALCOBS1CFGR,
			     RCC_FCALCOBS1CFGR_FCALCRSTN);

		/*
		 * Set the CKINTSEL (or CKEXTSEL if FCALCCKEXTSEL bit is set)
		 * field in the RCC Clock Frequency Calculator and Observation 0
		 * Clock Configuration Register (RCC_FCALCOBS0CFGR) to select
		 * the clock frequency calculator input clock (i.e. ckin).
		 */
		io_clrsetbits32(rcc_base + RCC_FCALCOBS0CFGR,
				RCC_FCALCOBS0CFGR_CKINTSEL_MASK,
				ckintsel);

		/*
		 * Set the FCALCCKEN field in the RCC Clock Frequency Calculator
		 * and Observation 0 Clock Configuration Register
		 * (RCC_FCALCOBS0CFGR) to enable the selected input clock at the
		 * input of clock frequency calculator.
		 */
		io_setbits32(rcc_base + RCC_FCALCOBS0CFGR,
			     RCC_FCALCOBS0CFGR_FCALCCKEN);

		/*
		 * Set the clock frequency time window value via the FCALCTWC
		 * field in the RCC Clock Frequency Calculator Control
		 * Register 2 (RCC_FCALCCR2)
		 */
		io_clrsetbits32(rcc_base + RCC_FCALCCR2,
				SHIFT_U32(0xf, RCC_FCALCCR2_FCALCTWC_SHIFT),
				SHIFT_U32(fcalctwc_val,
					  RCC_FCALCCR2_FCALCTWC_SHIFT));

		/*
		 * Set the FCALCTYP field to 0x0C in the RCC Clock Frequency
		 * Calculator Control Register 2 (RCC_FCALCCR2) to select the
		 * frequency value type.
		 */
		io_setbits32(rcc_base + RCC_FCALCCR2,
			     SHIFT_U32(0xc, RCC_FCALCCR2_FCALCTYP_SHIFT));
		/*
		 * Select the application mode via the FCALCMD field in the
		 * RCC Clock Frequency Calculator Control Register 2
		 * (RCC_FCALCCR2).
		 */
		io_setbits32(rcc_base + RCC_FCALCCR2,
			     SHIFT_U32(0x3, RCC_FCALCCR2_FCALCMD_SHIFT));

		/*
		 * Set the FCALCRUN bit in the RCC Clock Frequency Calculator
		 * Control Register 1 (RCC_FCALCCR1).
		 */
		io_write32(rcc_base + RCC_FCALCCR1, 1);

		/*
		 * Poll the FCALCSTS bit in the RCC Clock Frequency Calculator
		 * Status Register (RCC_FCALCSR) until the calculation is done.
		 */
		fcalcsts_bit = false;

		/*
		 * Timeout value (in ms) to be tuned according to lowest
		 * frequency to be measured
		 */
		if (!IO_READ32_POLL_TIMEOUT(rcc_base + RCC_FCALCSR, regval,
					    regval & RCC_FCALCSR_FVAL_MASK &&
					    regval & RCC_FCALCSR_FCALCSTS,
					    0, READ_FREQUENCY_TIMEOUT_US))
			fcalcsts_bit = true;

		/*
		 * Read the measured value via the FVAL field in the RCC Clock
		 * Frequency Calculator Status Register (RCC_FCALCSR).
		 * If FVAL[16] is 1 then the measured value is false.
		 */

		/* Detect overflow */
		if (regval & RCC_FCALCSR_FVAL_OVERFLOW) {
			/* Set status for next measurement iteration */
			fstatus = TEE_SUCCESS;
		} else if (!fcalcsts_bit) {
			/* Set status for leaving measurement algorithm */
			fstatus = TEE_ERROR_GENERIC;
		} else {
			if (!(regval & RCC_FCALCSR_FVAL_MASK)) {
				/*
				 * Set status for leaving measurement
				 * algorithm
				 */
				fstatus = TEE_ERROR_GENERIC;
			} else {
				uint64_t ckin_freq_val = 0UL;
				uint32_t denom_val = 0;

				denom_val = SHIFT_U32(1, fcalctwc_val + 1) + 5;

				/* Compute measured frequency */
				ckin_freq_val =
					(uint64_t)((regval &
						    RCC_FCALCSR_FVAL_MASK) -
						   16) * (uint64_t)ref_freq;

				ckin_freq_val /= (uint64_t)denom_val;

				freq = (uint32_t)ckin_freq_val;
				freq_meas_complete_flag = true;
				fstatus = TEE_SUCCESS;
			}
		}

		/*
		 * Re-initialisation :
		 * Clear the FCALCRUN bit in the RCC Clock Frequency Calculator
		 * Control Register 1 (RCC_FCALCCR1)
		 */
		io_write32(rcc_base + RCC_FCALCCR1, 0);

		/*
		 * Clear the FCALCRSTN bit in the RCC Clock Frequency Calculator
		 * and Observation 1 clock Configuration Register
		 * (RCC_FCALCOBS1CFGR) to disable the clock frequency
		 * calculator.
		 */
		io_clrbits32(rcc_base + RCC_FCALCOBS1CFGR,
			     RCC_FCALCOBS1CFGR_FCALCRSTN);

		/* Waiting for status register to be cleared */
		if (IO_READ32_POLL_TIMEOUT(rcc_base + RCC_FCALCSR, regval,
					   !(regval & GENMASK_32(15, 0)),
					   0, READ_FREQUENCY_TIMEOUT_US))
			fstatus = TEE_ERROR_GENERIC;

		/* Decrease FCALCTWC value */
		fcalctwc_val -= 1;

		/* Check status */
		if (fstatus == TEE_ERROR_GENERIC)
			return 0UL;
	}

	return freq;
}

#define RCC_CKINTSEL_HSI	0
#define RCC_CKINTSEL_MSI	2

static unsigned long clk_stm32_read_hsi_frequency(void)
{
	return clk_stm32_clock_frequency_calculator(RCC_CKINTSEL_HSI);
}

static unsigned long clk_stm32_read_msi_frequency(void)
{
	uintptr_t address = stm32_rcc_base() + RCC_HSIFMONCR;
	unsigned long freq = 0UL;

	/* Disable HSI clock period monitor interrupt */
	io_clrbits32(address, RCC_HSIFMONCR_HSIMONIE);

	freq = clk_stm32_clock_frequency_calculator(RCC_CKINTSEL_MSI);

	/* Enable HSI clock period monitor interrupt */
	io_setbits32(address, RCC_HSIFMONCR_HSIMONIE);

	return freq;
}

static void stm32mp_start_msi_clock_calib(void)
{
	struct stm32_calib_config *data = &osc_cal_device[MSI_CAL].data;

	if (data->is_used)
		stm32_calib_start_calibration(data);
}

static enum itr_return stm32_hsi_monitor_it_handler(struct itr_handler *handler)
{
	struct stm32_calib_config *data = handler->data;
	unsigned long freq = 0UL;
	uintptr_t base = stm32_rcc_base();
	uint32_t reg = 0;
	int direction = 0;
	int trim = 0;

	reg = io_read32(base + RCC_HSIFMONCR);
	if ((reg & RCC_HSIFMONCR_HSIMONF) != RCC_HSIFMONCR_HSIMONF)
		return ITRR_HANDLED;

	io_clrbits32(base + RCC_HSIFMONCR, RCC_HSIFMONCR_HSIMONIE);
	trim = stm32_calib_get_trim(data);

	freq = data->read_frequency();
	if (freq < (data->ref_freq - data->freq_margin))
		direction = 1;
	else if (freq > (data->ref_freq + data->freq_margin))
		direction = -1;
	if (direction != 0) {
		trim = stm32_calib_get_next_trim(data, trim, direction);
		stm32_calib_set_trim(data, trim);
	}

	io_setbits32(base + RCC_HSIFMONCR, RCC_HSIFMONCR_HSIMONIE);

	return ITRR_HANDLED;
}

static void stm32mp2_clk_hsi_restore_pm(struct stm32mp2_clk_cal *clk_cal)
{
	uintptr_t address = stm32_rcc_base() + RCC_HSIFMONCR;

	/* Set HSI clock count deviation value */
	io_clrsetbits32(address, RCC_HSIFMONCR_HSIDEV_MASK,
			clk_cal->irq_cal.hsi_dev << RCC_HSIFMONCR_HSIDEV_SHIFT);

	/* Set HSI clock count reference value */
	io_clrsetbits32(address, RCC_HSIFMONCR_HSIREF_MASK,
			clk_cal->irq_cal.hsi_ref << RCC_HSIFMONCR_HSIREF_SHIFT);

	/* Enable HSI clock period monitor interrupt */
	io_setbits32(address, RCC_HSIFMONCR_HSIMONIE);

	/* Enable HSI clock period monitor */
	io_setbits32(address, RCC_HSIFMONCR_HSIMONEN);
}

static void stm32mp2_parse_hsi_clock_calib(const void *fdt, int node,
					   struct stm32_calib_config_irq *irq)
{
	irq->hsi_dev = fdt_read_uint32_default(fdt, node,
					       "st,cal_hsi_dev", UINT_MAX);

	irq->hsi_ref = fdt_read_uint32_default(fdt, node,
					       "st,cal_hsi_ref", UINT_MAX);
}

static void stm32mp2_parse_clock_calib(const void *fdt, int node)
{
	unsigned int i = 0;
	int subnode = 0;

	/*  Parse subnode to find the calibration */
	fdt_for_each_subnode(subnode, fdt, node) {
		int status = fdt_get_status(fdt, subnode);
		const char *compat = NULL;

		if (status == DT_STATUS_DISABLED)
			continue;

		compat = fdt_stringlist_get(fdt, subnode,
					    "compatible", 0, NULL);
		if (!compat)
			continue;

		for (i = 0; i < ARRAY_SIZE(osc_cal_device); i++) {
			struct stm32mp2_clk_cal *clk_cal = &osc_cal_device[i];
			struct stm32_calib_config *data = &clk_cal->data;

			if (strcmp(data->compatible, compat) != 0)
				continue;

			if (i == HSI_CAL) {
				struct stm32_calib_config_irq *irq = NULL;
				int irq_val = 0;

				irq = &clk_cal->irq_cal;

				irq_val = dt_get_irq_type_prio(fdt, node,
							       &irq->irq_type,
							       &irq->irq_prio);

				if (irq_val == DT_INFO_INVALID_INTERRUPT) {
					EMSG("No IT defined in RCC for %s",
					     data->name);
					continue;
				}
				irq->irq = irq_val;

				stm32mp2_parse_hsi_clock_calib(fdt, subnode,
							       irq);
			}
			data->is_used = true;
		}
	}
}

static TEE_Result init_hsi_calibration(struct stm32mp2_clk_cal *clk_cal)
{
	struct stm32_calib_config *data = &clk_cal->data;
	struct clk *hsi_clk = stm32mp_rcc_clock_id_to_clk(HSI_CK);
	uintptr_t address = stm32_rcc_base() + RCC_HSIFMONCR;
	TEE_Result res = TEE_ERROR_GENERIC;

	if (!hsi_clk)
		panic();

	data->ref_freq = clk_get_rate(hsi_clk);
	data->freq_margin = clk_cal->irq_cal.hsi_dev *
		(data->ref_freq / clk_cal->irq_cal.hsi_ref);

	/* Set HSI clock count deviation value */
	assert(clk_cal->irq_cal.hsi_ref <= RCC_HSIFMONCR_HSIDEV_MASK);
	io_clrsetbits32(address, RCC_HSIFMONCR_HSIDEV_MASK,
			clk_cal->irq_cal.hsi_dev << RCC_HSIFMONCR_HSIDEV_SHIFT);

	/* Set HSI clock count reference value */
	io_clrsetbits32(address, RCC_HSIFMONCR_HSIREF_MASK,
			clk_cal->irq_cal.hsi_ref << RCC_HSIFMONCR_HSIREF_SHIFT);

	res = interrupt_alloc_add_conf_handler(interrupt_get_main_chip(),
					       clk_cal->irq_cal.irq,
					       stm32_hsi_monitor_it_handler,
					       ITRF_TRIGGER_LEVEL,
					       data,
					       clk_cal->irq_cal.irq_type,
					       clk_cal->irq_cal.irq_prio,
					       &clk_cal->irq_cal.itr);
	if (res)
		return res;

	interrupt_enable(clk_cal->irq_cal.itr->chip, clk_cal->irq_cal.itr->it);

	/* Enable HSI clock period monitor interrupt */
	io_setbits32(address, RCC_HSIFMONCR_HSIMONIE);

	/* Enable HSI clock period monitor */
	io_setbits32(address, RCC_HSIFMONCR_HSIMONEN);

	return TEE_SUCCESS;
}

static TEE_Result init_msi_calibration(struct stm32mp2_clk_cal *clk_cal)
{
	struct stm32_calib_config *data = &clk_cal->data;
	struct clk *msi_clk = stm32mp_rcc_clock_id_to_clk(MSI_CK);

	if (!msi_clk)
		panic();

	data->ref_freq = clk_get_rate(msi_clk);
	data->freq_margin = (data->ref_freq * (data->margin_ratio) / 1000);

	return TEE_SUCCESS;
}

static void stm32mp2_init_clock_calib(void)
{
	size_t i = 0;

	for (i = 0; i < MAX_CAL; i++) {
		struct stm32mp2_clk_cal *clk_cal = &osc_cal_device[i];
		struct stm32_calib_config *data = &clk_cal->data;

		if (!data->is_used)
			continue;

		if (clk_cal->init_cal(clk_cal)) {
			EMSG("Calibration init of %s is failed",
			     data->name);

			data->is_used = false;

			continue;
		}

		DMSG("Calibration %s enabled", data->name);
	}
}

static TEE_Result
stm32mp2_calib_pm(enum pm_op op, unsigned int pm_hint,
		  const struct pm_callback_handle *hdl __unused)
{
	unsigned int i = 0;

	if (op == PM_OP_RESUME) {
		struct stm32mp2_clk_cal *clk_cal = &osc_cal_device[HSI_CAL];
		struct stm32_calib_config *data = &clk_cal->data;

		if (PM_HINT_IS_STATE(pm_hint, CONTEXT) && data->is_used)
			/* Only RCC_HSIFMONCR register need to be restored */
			stm32mp2_clk_hsi_restore_pm(clk_cal);

		/* Enable HSI clock period monitor interrupt */
		if (clk_cal->irq_cal.itr)
			interrupt_enable(clk_cal->irq_cal.itr->chip,
					 clk_cal->irq_cal.itr->it);

		return TEE_SUCCESS;
	}
	/* SUSPEND */
	for (i = 0; i < MAX_CAL; i++) {
		struct stm32mp2_clk_cal *clk_cal = &osc_cal_device[i];
		struct stm32_calib_config *data = &clk_cal->data;

		/* Disable HSI clock period monitor interrupt */
		if (clk_cal->irq_cal.itr)
			interrupt_disable(clk_cal->irq_cal.itr->chip,
					  clk_cal->irq_cal.itr->it);
		if (data->is_used)
			data->init_done = false;
	}

	return TEE_SUCCESS;
}

/* Initialization of the calibration is called at end of RCC clock driver */
TEE_Result clk_stm32_init_calib(const void *fdt, int node)
{
	stm32mp2_parse_clock_calib(fdt, node);

	stm32mp2_init_clock_calib();

	register_pm_driver_cb(stm32mp2_calib_pm, NULL, "service-calib");

	return TEE_SUCCESS;
}

/* Function called by PTA to start clock calibration */
void stm32mp_start_clock_calibration(void)
{
	stm32mp_start_msi_clock_calib();
}
