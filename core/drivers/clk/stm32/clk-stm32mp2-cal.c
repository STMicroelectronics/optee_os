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

#define HSI_CALIBRATION_NAME		"hsi-calibration"
#define MSI_CALIBRATION_NAME		"msi-calibration"
#define HSI_CALIBRATION_COMPAT		"st,hsi-cal"
#define MSI_CALIBRATION_COMPAT		"st,msi-cal"

#define CALIBRATION_TIMEOUT_US		100000

#define MAX_CALIBRATION_BOUNDARY	17

enum osc_calibration {
	HSI_CAL = 0,
	MSI_CAL,
	MAX_CAL,
};

/* Max and min boundary trim value around forbidden value */
struct stm32mp2_trim_boundary_t {
	unsigned int max;
	unsigned int min;
};

struct stm32_calib_pdata {
	struct itr_handler *itr;
	int irq;
	uint32_t irq_type;
	uint32_t irq_prio;
	unsigned int hsi_dev;
	unsigned int hsi_ref;
};

struct stm32_calib_config {
	const char *compatible;
	const char *name;
	unsigned int freq_margin;
	int trim_max;
	int trim_min;
	const uint16_t *fbv;
};

struct calib_ops {
	TEE_Result (*init_cal)(struct stm32_calib_pdata *pdata);
	void (*set_trim)(unsigned int cal, unsigned int cal_ref);
	unsigned int (*get_cal)(void);
	unsigned long (*read_frequency)(void);
};

struct stm32_calib {
	struct stm32mp2_trim_boundary_t boundary[MAX_CALIBRATION_BOUNDARY];
	unsigned int boundary_max;
	unsigned int cal_ref;
	unsigned long ref_freq;
	bool is_used;
	bool force_init;
	const struct calib_ops *ops;
	const struct stm32_calib_config *cfg;
};

static void stm32mp2_clk_hsi_set_trim(unsigned int cal, unsigned int cal_ref);
static void stm32mp2_clk_msi_set_trim(unsigned int cal, unsigned int cal_ref);

static unsigned int stm32mp2_clk_hsi_get_cal(void);
static unsigned int stm32mp2_clk_msi_get_cal(void);

static TEE_Result stm32mp2_clk_init_hsi_cal(struct stm32_calib_pdata *pdata);
static TEE_Result stm32mp2_clk_init_msi_cal(struct stm32_calib_pdata *pdata);

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

static unsigned long clk_stm32_read_hsi_frequency(void);

static unsigned long clk_stm32_read_msi_frequency(void);

struct stm32_calib hsi_calib = {
	.ops = &(const struct calib_ops) {
		.init_cal = stm32mp2_clk_init_hsi_cal,
		.set_trim = stm32mp2_clk_hsi_set_trim,
		.get_cal = stm32mp2_clk_hsi_get_cal,
		.read_frequency = clk_stm32_read_hsi_frequency,
	},
	.cfg = &(const struct stm32_calib_config) {
		.name = HSI_CALIBRATION_NAME,
		.compatible = HSI_CALIBRATION_COMPAT,
		.fbv = fbv_hsi,
		.trim_max = 63,
		.trim_min = -64,
	},
};

struct stm32_calib msi_calib = {
	.ops = &(const struct calib_ops) {
		.init_cal = stm32mp2_clk_init_msi_cal,
		.set_trim = stm32mp2_clk_msi_set_trim,
		.get_cal = stm32mp2_clk_msi_get_cal,
		.read_frequency = clk_stm32_read_msi_frequency,
	},
	.cfg = &(const struct stm32_calib_config) {
		.name = MSI_CALIBRATION_NAME,
		.compatible = MSI_CALIBRATION_COMPAT,
		.trim_max = 31,
		.trim_min = 0,
		.freq_margin = 8,
		.fbv = fbv_msi,
	},
};

struct stm32_calib_pdata calib_pdata;

struct stm32_calib *calib_tab[MAX_CAL] = {
	[HSI_CAL] = &hsi_calib,
	[MSI_CAL] = &msi_calib,
};

static unsigned long clk_stm32_clock_frequency_calculator_get_ref(void)
{
	uint32_t val = 0;
	struct clk *clk = NULL;

	val = io_read32(stm32_rcc_base() + RCC_FCALCREFCFGR) &
	      RCC_FCALCREFCFGR_FCALCREFCKSEL_MASK;

	val = (io_read32(stm32_rcc_base() + RCC_MUXSELCFGR) >> (val * 4)) &
	      RCC_MUXSELCFGR_MUXSEL0_MASK;

	switch (val) {
	case 0x0:
		clk = stm32mp_rcc_clock_id_to_clk(HSI_CK);
		break;
	case 0x1:
		clk = stm32mp_rcc_clock_id_to_clk(HSE_CK);
		break;

	case 0x3:
		clk = stm32mp_rcc_clock_id_to_clk(MSI_CK);
		break;

	default:
		panic();
		break;
	}

	return clk_get_rate(clk);
}

static unsigned long clk_stm32_clock_frequency_calculator(uint32_t ckintsel)
{
	unsigned long ref_freq = clk_stm32_clock_frequency_calculator_get_ref();
	unsigned long freq = 0UL;
	uintptr_t rcc_base = stm32_rcc_base();
	uint32_t fcalctwc_val = 0xf;
	uint32_t min_fcalctwc = 1;
	uint32_t regval = 0;
	bool freq_meas_complete_flag = false;
	bool fcalcsts_bit = false;
	TEE_Result fstatus = TEE_SUCCESS;

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
					    0, CALIBRATION_TIMEOUT_US))
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
					   0, CALIBRATION_TIMEOUT_US))
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
	return clk_stm32_clock_frequency_calculator(RCC_CKINTSEL_MSI);
}

static unsigned int stm32mp2_clk_hsi_get_cal(void)
{
	return (io_read32(stm32_rcc_base() + RCC_HSICFGR) &
		RCC_HSICFGR_HSICAL_MASK) >> RCC_HSICFGR_HSICAL_SHIFT;
}
DECLARE_KEEP_PAGER(stm32mp2_clk_hsi_get_cal);

static void stm32mp2_clk_hsi_set_trim(unsigned int cal, unsigned int cal_ref)
{
	int clk_trim = cal - cal_ref;
	uint32_t trim = 0;

	trim = ((uint32_t)clk_trim << RCC_HSICFGR_HSITRIM_SHIFT) &
		RCC_HSICFGR_HSITRIM_MASK;

	io_clrsetbits32(stm32_rcc_base() + RCC_HSICFGR,
			RCC_HSICFGR_HSITRIM_MASK, trim);
}
DECLARE_KEEP_PAGER(stm32mp2_clk_hsi_set_trim);

static unsigned int stm32mp2_clk_msi_get_cal(void)
{
	return (io_read32(stm32_rcc_base() + RCC_MSICFGR) &
		RCC_MSICFGR_MSICAL_MASK) >> RCC_MSICFGR_MSICAL_SHIFT;
}
DECLARE_KEEP_PAGER(stm32mp2_clk_msi_get_cal);

static void stm32mp2_clk_msi_set_trim(unsigned int cal, unsigned int cal_ref)
{
	uint32_t clk_trim = cal - cal_ref;
	uint32_t trim = (clk_trim << RCC_MSICFGR_MSITRIM_SHIFT) &
			RCC_MSICFGR_MSITRIM_MASK;

	io_clrsetbits32(stm32_rcc_base() + RCC_MSICFGR,
			RCC_MSICFGR_MSITRIM_MASK, trim);
}
DECLARE_KEEP_PAGER(stm32mp2_clk_msi_set_trim);

static unsigned int trim_increase(struct stm32_calib *data, unsigned int cal)
{
	struct stm32mp2_trim_boundary_t *boundary = NULL;
	unsigned int new_cal = 0;
	int i = 0;

	/* By default: last calibration value */
	new_cal = cal;

	/* Start from lowest cal value */
	for (i = (int)data->boundary_max - 1; i >= 0; i--) {
		boundary = &data->boundary[i];

		if (cal < boundary->min) {
			new_cal = boundary->min;
			break;
		}

		if (cal >= boundary->min && cal <= boundary->max) {
			new_cal = cal + 1;
			break;
		}
	}

	return new_cal;
}

static unsigned int trim_decrease(struct stm32_calib *data, unsigned int cal)
{
	struct stm32mp2_trim_boundary_t *boundary = NULL;
	unsigned int new_cal = 0;
	unsigned int i = 0;

	/* By default: last calibration value */
	new_cal = cal;

	/* Start from Highest cal value */
	for (i = 0; i < data->boundary_max; i++) {
		boundary = &data->boundary[i];

		if (cal > boundary->max) {
			new_cal = boundary->max;
			break;
		}

		if (cal > boundary->min && cal <= boundary->max) {
			new_cal = cal - 1;
			break;
		}
	}

	return new_cal;
}

static void save_trim(struct stm32_calib *data, unsigned int i,
		      unsigned int max, unsigned int min)
{
	data->boundary[i].max = max;
	data->boundary[i].min = min;
}

static int trim_find_prev_boundary(struct stm32_calib *data, unsigned int x1)
{
	unsigned int x = x1;
	unsigned long freq = 0;

	data->ops->set_trim(x1 + 1, data->cal_ref);
	freq = data->ops->read_frequency();

	while (x >= (data->cal_ref + data->cfg->trim_min)) {
		x--;
		data->ops->set_trim(x, data->cal_ref);

		if (data->ops->read_frequency() <= freq)
			break;
	};

	return x;
}

static void trim_table_init(struct stm32_calib *data)
{
	const uint16_t *trim_fbv = data->cfg->fbv;
	unsigned int min = 0;
	unsigned int max = 0;
	int boundary = 0;
	int i = 0;

	max = data->cal_ref + data->cfg->trim_max;
	min = data->cal_ref + data->cfg->trim_min;

	while (trim_fbv[i]) {
		unsigned int x = 0;
		unsigned int x1 = trim_fbv[i];
		unsigned int x2 = trim_fbv[i + 1];

		if (max <= x2 || min >= x1) {
			i++;
			if (boundary != 0)
				goto out;
			continue;
		}

		/* Take forbidden value + 1 */
		x2 = x2 + 1;
		if (x2 < min)
			x2 = min;

		if (boundary == 0) {
			/* Save first boundary */
			save_trim(data, boundary, max, x2);
			boundary++;
			i++;
			continue;
		}

		x = trim_find_prev_boundary(data, x1);
		/* Save boundary values */
		save_trim(data, boundary, x - 1, x2);
		boundary++;
		i++;
	};
out:
	data->boundary_max = boundary;
}

static void stm32mp_start_msi_clock_calib(void)
{
	struct stm32_calib *data = calib_tab[MSI_CAL];
	unsigned long margin = (data->ref_freq *
				data->cfg->freq_margin) / 1000;
	unsigned long min = data->ref_freq - margin;
	unsigned long max = data->ref_freq + margin;
	unsigned long freq = data->ops->read_frequency();
	int trim = 0;
	int new_trim = 0;
	unsigned long conv = 0;
	unsigned long min_conv = ULONG_MAX;
	uint64_t timeout_ref = 0;

	if (!data->is_used) {
		DMSG("MSI clock calibration not supported");

		return;
	}

	if (freq >= min && freq <= max && data->force_init)
		return;

	data->force_init = true;
	trim = data->ops->get_cal();
	timeout_ref = timeout_init_us(CALIBRATION_TIMEOUT_US);

	do {
		if (freq < data->ref_freq)
			new_trim = trim_increase(data, trim);
		else
			new_trim = trim_decrease(data, trim);

		data->ops->set_trim(new_trim, data->cal_ref);

		freq = data->ops->read_frequency();
		if (freq == 0) {
			/* Calibration will be stopped */
			data->ref_freq = 0U;
			return;
		}

		conv = (data->ref_freq < freq) ?
			freq - data->ref_freq : data->ref_freq - freq;

		if (conv < min_conv) {
			min_conv = conv;
			trim = new_trim;
		} else if (conv == min_conv) {
			break;
		}

		if (timeout_elapsed(timeout_ref))
			break;

	} while (conv == min_conv);

	DMSG("Calibration : Freq %lu , trim %i\n", freq, trim);

	data->ops->set_trim(trim, data->cal_ref);

	freq = data->ops->read_frequency();
	if (freq < min || freq > max) {
		EMSG("Calibration failed: Freq %lu , trim %i\n", freq, trim);
		panic();
	}
}

static enum itr_return stm32_hsi_monitor_it_handler(struct itr_handler *handler)
{
	struct stm32_calib *data = handler->data;
	uintptr_t base = stm32_rcc_base();
	uint32_t reg = 0;
	uint32_t var = 0;
	unsigned long freq = 0UL;
	int trim = 0;

	reg = io_read32(base + RCC_HSIFMONCR);
	if ((reg & RCC_HSIFMONCR_HSIMONF) != RCC_HSIFMONCR_HSIMONF)
		return ITRR_HANDLED;

	io_clrbits32(base + RCC_HSIFMONCR, RCC_HSIFMONCR_HSIMONIE);
	io_clrbits32(base + RCC_HSIFMONCR, RCC_HSIFMONCR_HSIMONEN);

	var = io_read32(base + RCC_HSIFVALR) & RCC_HSIFVALR_HSIVAL_MASK;
	freq = var * (data->ref_freq / (reg & RCC_HSIFMONCR_HSIREF_MASK));

	trim = data->ops->get_cal();

	if (freq < data->ref_freq) {
		if (trim == (int)(data->cal_ref + data->cfg->trim_max))
			trim = data->cal_ref + data->cfg->trim_min;
		else
			trim = trim_increase(data, trim);
	} else {
		trim = trim_decrease(data, trim);
	}

	data->ops->set_trim(trim, data->cal_ref);

	io_setbits32(base + RCC_HSIFMONCR, RCC_HSIFMONCR_HSIMONIE);
	io_setbits32(base + RCC_HSIFMONCR, RCC_HSIFMONCR_HSIMONEN);

	return ITRR_HANDLED;
}
DECLARE_KEEP_PAGER(stm32_hsi_monitor_it_handler);

static void stm32mp2_clk_hsi_restore_pm(struct stm32_calib_pdata *pdata)
{
	uintptr_t address = stm32_rcc_base() + RCC_HSIFMONCR;

	/* Set HSI clock count deviation value */
	io_clrsetbits32(address, RCC_HSIFMONCR_HSIDEV_MASK,
			pdata->hsi_dev << RCC_HSIFMONCR_HSIDEV_SHIFT);

	/* Set HSI clock count reference value */
	io_clrsetbits32(address, RCC_HSIFMONCR_HSIREF_MASK,
			pdata->hsi_ref << RCC_HSIFMONCR_HSIREF_SHIFT);

	/* Enable HSI clock period monitor interrupt */
	io_setbits32(address, RCC_HSIFMONCR_HSIMONIE);

	/* Enable HSI clock period monitor */
	io_setbits32(address, RCC_HSIFMONCR_HSIMONEN);
}

static TEE_Result stm32mp2_clk_init_hsi_cal(struct stm32_calib_pdata *pdata)
{
	struct clk *hsi_clk = stm32mp_rcc_clock_id_to_clk(HSI_CK);
	uintptr_t address = stm32_rcc_base() + RCC_HSIFMONCR;
	struct stm32_calib *data = calib_tab[HSI_CAL];
	TEE_Result res = TEE_ERROR_GENERIC;

	data->ref_freq = clk_get_rate(hsi_clk);
	data->cal_ref = stm32mp2_clk_hsi_get_cal();

	/* Set HSI clock count deviation value */
	assert(pdata->hsi_ref <= RCC_HSIFMONCR_HSIDEV_MASK);
	io_clrsetbits32(address, RCC_HSIFMONCR_HSIDEV_MASK,
			pdata->hsi_dev << RCC_HSIFMONCR_HSIDEV_SHIFT);

	/* Set HSI clock count reference value */
	io_clrsetbits32(address, RCC_HSIFMONCR_HSIREF_MASK,
			pdata->hsi_ref << RCC_HSIFMONCR_HSIREF_SHIFT);

	/* Calculate all boundary values and get best trim value */
	trim_table_init(data);
	data->ops->set_trim(data->cal_ref, data->cal_ref);

	res = interrupt_alloc_add_conf_handler(interrupt_get_main_chip(),
					       pdata->irq,
					       stm32_hsi_monitor_it_handler,
					       ITRF_TRIGGER_LEVEL,
					       data,
					       pdata->irq_type,
					       pdata->irq_prio,
					       &pdata->itr);

	if (res)
		return res;

	interrupt_enable(pdata->itr->chip, pdata->itr->it);

	/* Enable HSI clock period monitor interrupt */
	io_setbits32(address, RCC_HSIFMONCR_HSIMONIE);

	/* Enable HSI clock period monitor */
	io_setbits32(address, RCC_HSIFMONCR_HSIMONEN);

	return TEE_SUCCESS;
}

static TEE_Result
stm32mp2_clk_init_msi_cal(struct stm32_calib_pdata *pdata __unused)
{
	struct clk *msi_clk = stm32mp_rcc_clock_id_to_clk(MSI_CK);
	struct stm32_calib *data = calib_tab[MSI_CAL];

	data->ref_freq = clk_get_rate(msi_clk);
	data->cal_ref = stm32mp2_clk_msi_get_cal();
	trim_table_init(data);
	data->ops->set_trim(data->cal_ref, data->cal_ref);

	return TEE_SUCCESS;
}

static void stm32mp2_parse_hsi_clock_calib(const void *fdt, int node,
					   struct stm32_calib_pdata *pdata)
{
	pdata->hsi_dev = fdt_read_uint32_default(fdt, node,
						 "st,cal_hsi_dev", UINT_MAX);

	pdata->hsi_ref = fdt_read_uint32_default(fdt, node,
						 "st,cal_hsi_ref", UINT_MAX);
}

static void stm32mp2_parse_clock_calib(const void *fdt, int node,
				       struct stm32_calib_pdata *pdata)
{
	unsigned int i = 0;
	int subnode = 0;

	pdata->irq = dt_get_irq_type_prio(fdt, node, &pdata->irq_type,
					  &pdata->irq_prio);

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

		for (i = 0; i < MAX_CAL; i++) {
			struct stm32_calib *data = calib_tab[i];

			if (strcmp(data->cfg->compatible, compat) != 0)
				continue;

			if (i == HSI_CAL) {
				if (pdata->irq  == DT_INFO_INVALID_INTERRUPT) {
					EMSG("No IT defined in RCC for %s",
					     data->cfg->name);
					continue;
				}

				stm32mp2_parse_hsi_clock_calib(fdt, subnode,
							       pdata);
			}

			data->is_used = true;
		}
	}
}

static TEE_Result stm32mp2_init_clock_calib(struct stm32_calib_pdata *pdata)
{
	size_t i = 0;
	unsigned int j = 0;

	for (i = 0; i < MAX_CAL; i++) {
		struct stm32_calib *data = calib_tab[i];

		if (!data->is_used)
			continue;

		if (data->ops->init_cal(pdata)) {
			EMSG("Calibration init of %s is failed",
			     data->cfg->name);

			data->is_used = false;

			continue;
		}

		DMSG("Calibration %s enabled", data->cfg->name);
		for (j = 0; j < data->boundary_max; j++) {
			DMSG("boundary[%d].max = %d, boundary[%d].min = %d",
			     j, data->boundary[j].max,
			     j, data->boundary[j].min);
		}
	}

	return TEE_SUCCESS;
}

static TEE_Result
stm32mp2_calib_pm(enum pm_op op, unsigned int pm_hint,
		  const struct pm_callback_handle *hdl)
{
	struct stm32_calib_pdata *pdata = hdl->handle;
	unsigned int i = 0;

	if (op == PM_OP_RESUME) {
		struct stm32_calib *data = calib_tab[HSI_CAL];

		if (PM_HINT_IS_STATE(pm_hint, CONTEXT) && data->is_used)
			/* Only RCC_HSIFMONCR register need to be restored */
			stm32mp2_clk_hsi_restore_pm(pdata);

		return TEE_SUCCESS;
	}
	/* SUSPEND */
	for (i = 0; i < MAX_CAL; i++) {
		struct stm32_calib *data = calib_tab[i];

		if (data->is_used)
			data->force_init = false;
	}

	return TEE_SUCCESS;
}
DECLARE_KEEP_PAGER(stm32mp2_calib_pm);

TEE_Result clk_stm32_init_calib(const void *fdt, int node)
{
	struct stm32_calib_pdata *pdata = NULL;
	TEE_Result res = TEE_ERROR_GENERIC;

	pdata = calloc(1, sizeof(*pdata));
	if (!pdata) {
		EMSG("Failed to allocate platform data for calibration");

		return TEE_ERROR_OUT_OF_MEMORY;
	}

	stm32mp2_parse_clock_calib(fdt, node, pdata);

	res = stm32mp2_init_clock_calib(pdata);
	if (res) {
		free(pdata);
		return res;
	}

	register_pm_driver_cb(stm32mp2_calib_pm, pdata, "service-calib");

	return TEE_SUCCESS;
}

void stm32mp_start_clock_calibration(void)
{
	stm32mp_start_msi_clock_calib();
}
