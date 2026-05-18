// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2022-2024, STMicroelectronics
 */

#include <assert.h>
#include <config.h>
#include <drivers/clk.h>
#include <drivers/clk_dt.h>
#include <drivers/regulator.h>
#include <drivers/stm32_cpu_opp.h>
#ifdef CFG_STM32MP13
#include <drivers/stm32mp1_pwr.h>
#endif
#include <initcall.h>
#include <io.h>
#include <keep.h>
#include <kernel/dt.h>
#include <kernel/dt_driver.h>
#include <kernel/mutex.h>
#include <kernel/panic.h>
#include <kernel/pm.h>
#include <libfdt.h>
#include <stm32_util.h>
#include <trace.h>

/*
 * struct cpu_dvfs - CPU DVFS registered operating point
 * @freq_khz: CPU frequency in kilohertz (kHz)
 * @volt_uv: CPU voltage level in microvolts (uV)
 */
struct cpu_dvfs {
	unsigned int freq_khz;
	int volt_uv;
};

/*
 * struct cpu_opp - CPU operating point
 *
 * @current_opp: Index of current CPU operating point in @dvfs array
 * @default_opp: Index of OPP to set at boot or during suspend in @dvfs array
 *               with a long term sustainable frequency
 * @opp_count: Number of cells of @dvfs
 * @clock: CPU clock handle
 * @regul: CPU regulator supply handle
 * @dvfs: Array of the supported CPU operating points
 * @running: boolean meaning that the dynamic part of the opp driver is running
 */
struct cpu_opp {
	unsigned int current_opp;
	unsigned int default_opp;
	unsigned int opp_count;
	struct clk *clock;
	struct regulator *regul;
	struct cpu_dvfs *dvfs;
#ifdef CFG_STM32MP15
	bool running;
#endif
};

static struct cpu_opp cpu_opp;

#ifdef CFG_SCMI_MSG_PERF_DOMAIN
/* Mutex for protecting CPU OPP changes */
static struct mutex cpu_opp_mu = MUTEX_INITIALIZER;
#endif

unsigned int stm32_cpu_opp_count(void)
{
	return cpu_opp.opp_count;
}

/*
 * This function returns true if the given OPP voltage can be managed.
 * If the exact voltage value is not supported by the regulator,
 * the function may adjust the input parameter volt_uv to a higher
 * supported value and still return true.
 */
static bool opp_voltage_is_supported(struct regulator *regul, uint32_t *volt_uv)
{
	int target_volt_uv = 0;
	int new_volt_uv = 0;
	int min_uv = 0;
	int max_uv = 0;
	struct regulator_voltages_desc *desc = NULL;
	const int *levels = NULL;
	TEE_Result res = TEE_ERROR_GENERIC;

	assert(*volt_uv < INT_MAX);
	target_volt_uv = (int)*volt_uv;

	res = regulator_supported_voltages(regul, &desc, &levels);
	if (res) {
		regulator_get_range(regul, &min_uv, &max_uv);
		if (target_volt_uv > max_uv)
			return false;
		if (target_volt_uv < min_uv)
			*volt_uv = min_uv;
		return true;
	}

	if (desc->type == VOLTAGE_TYPE_FULL_LIST) {
		unsigned int i = 0;

		for (i = 0 ; i < desc->num_levels; i++) {
			if (levels[i] >= target_volt_uv) {
				new_volt_uv = levels[i];
				break;
			}
		}
		if (new_volt_uv == 0)
			return false;

	} else if (desc->type == VOLTAGE_TYPE_INCREMENT) {
		int min = levels[0];
		int max = levels[1];
		int step = levels[2];

		if (new_volt_uv > max)
			return false;

		new_volt_uv = min +
			      DIV_ROUND_UP(target_volt_uv - min, step) * step;
	} else {
		return false;
	}

	*volt_uv = new_volt_uv;

	return true;
}

#if defined(CFG_STM32MP13) || defined(CFG_STM32MP15)
static TEE_Result set_opp_clk_rate(unsigned int opp)
{
#ifdef CFG_STM32MP15
	return stm32mp1_set_opp_khz(cpu_opp.dvfs[opp].freq_khz);
#else
	return clk_set_rate(cpu_opp.clock, cpu_opp.dvfs[opp].freq_khz * 1000);
#endif
}

static TEE_Result set_opp_voltage(unsigned int opp)
{
	return regulator_set_voltage(cpu_opp.regul, cpu_opp.dvfs[opp].volt_uv);
}

static TEE_Result set_clock_then_voltage(unsigned int opp)
{
	TEE_Result res = TEE_ERROR_GENERIC;

	res = set_opp_clk_rate(opp);
	if (res) {
		EMSG("Failed to set OPP %ukHz",
		     cpu_opp.dvfs[opp].freq_khz);
		return res;
	}

#ifdef CFG_STM32MP13
	if (cpu_opp.dvfs[opp].volt_uv <= PWR_MPU_RAM_LOW_SPEED_THRESHOLD)
		io_setbits32(stm32_pwr_base() + PWR_CR1_OFF,
			     PWR_CR1_MPU_RAM_LOW_SPEED);
#endif

	res = set_opp_voltage(opp);
	if (res) {
		EMSG("Failed to set OPP %uuV", cpu_opp.dvfs[opp].volt_uv);

		if (cpu_opp.current_opp >= cpu_opp.opp_count)
			panic();

		/* Restore previous OPP */
#ifdef CFG_STM32MP13
		if (cpu_opp.dvfs[cpu_opp.current_opp].volt_uv >
		    PWR_MPU_RAM_LOW_SPEED_THRESHOLD)
			io_clrbits32(stm32_pwr_base() + PWR_CR1_OFF,
				     PWR_CR1_MPU_RAM_LOW_SPEED);
#endif

		if (set_opp_clk_rate(cpu_opp.current_opp))
			panic("Failed to restore clock");

		return res;
	}

	return TEE_SUCCESS;
}

static TEE_Result set_voltage_then_clock(unsigned int opp)
{
	TEE_Result res = TEE_ERROR_GENERIC;

	res = set_opp_voltage(opp);
	if (res) {
		EMSG("Failed to set OPP %uuV", cpu_opp.dvfs[opp].volt_uv);
		return res;
	}

#ifdef CFG_STM32MP13
	if (cpu_opp.dvfs[opp].volt_uv > PWR_MPU_RAM_LOW_SPEED_THRESHOLD)
		io_clrbits32(stm32_pwr_base() + PWR_CR1_OFF,
			     PWR_CR1_MPU_RAM_LOW_SPEED);
#endif

	res = set_opp_clk_rate(opp);
	if (res) {
		EMSG("Failed to set OPP %ukHz",
		     cpu_opp.dvfs[opp].freq_khz);

		if (cpu_opp.current_opp >= cpu_opp.opp_count)
			panic();

		/* Restore previous OPP */
#ifdef CFG_STM32MP13
		if (cpu_opp.dvfs[cpu_opp.current_opp].volt_uv <=
		    PWR_MPU_RAM_LOW_SPEED_THRESHOLD)
			io_setbits32(stm32_pwr_base() + PWR_CR1_OFF,
				     PWR_CR1_MPU_RAM_LOW_SPEED);
#endif

		if (set_opp_voltage(cpu_opp.current_opp))
			panic("Failed to restore voltage");

		return res;
	}

	return TEE_SUCCESS;
}

static TEE_Result set_opp(unsigned int opp)
{
	unsigned long clk_cpu = clk_get_rate(cpu_opp.clock);
	unsigned long clk_opp = cpu_opp.dvfs[opp].freq_khz * 1000U;
	TEE_Result res = TEE_ERROR_GENERIC;

	assert(clk_cpu);
	if (clk_opp == clk_cpu)
		res = TEE_SUCCESS;
	else if (clk_opp >= clk_cpu)
		res = set_voltage_then_clock(opp);
	else
		res = set_clock_then_voltage(opp);

	return res;
}
#endif /* CFG_STM32MP13 || CFG_STM32MP15 */

#ifdef CFG_SCMI_MSG_PERF_DOMAIN
unsigned int stm32_cpu_opp_sustained_level(void)
{
	return cpu_opp.dvfs[cpu_opp.default_opp].freq_khz;
}

/* Perf level relates straight to CPU frequency in kHz */
unsigned int stm32_cpu_opp_level(unsigned int opp)
{
	assert(opp < cpu_opp.opp_count);

	return cpu_opp.dvfs[opp].freq_khz;
}

TEE_Result stm32_cpu_opp_set_level(unsigned int level)
{
	TEE_Result res = TEE_ERROR_GENERIC;
	unsigned int opp = 0;

	DMSG("Set OPP to %ukHz", level);

	if (!cpu_opp.opp_count)
		return TEE_ERROR_GENERIC;

	mutex_lock(&cpu_opp_mu);

	for (opp = 0; opp < cpu_opp.opp_count; opp++)
		if (level == cpu_opp.dvfs[opp].freq_khz)
			break;

	if (opp == cpu_opp.opp_count) {
		mutex_unlock(&cpu_opp_mu);
		return TEE_ERROR_BAD_PARAMETERS;
	}

	res = set_opp(opp);
	if (!res)
		cpu_opp.current_opp = opp;

	mutex_unlock(&cpu_opp_mu);

	return res;
}

TEE_Result stm32_cpu_opp_read_level(unsigned int *level)
{
	if (!cpu_opp.opp_count) {
		EMSG("No CPU OPP defined");
		return TEE_ERROR_GENERIC;
	}
	if (cpu_opp.current_opp >= cpu_opp.opp_count)
		return TEE_ERROR_BAD_STATE;

	*level = stm32_cpu_opp_level(cpu_opp.current_opp);

#ifdef CFG_STM32MP15
	/* Assume that the dynamic part of the OPP driver is used */
	cpu_opp.running = true;
#endif

	return TEE_SUCCESS;
}
#endif /*CFG_SCMI_MSG_PERF_DOMAIN*/

static TEE_Result stm32_cpu_opp_is_supported(const void *fdt, int subnode)
{
	const fdt32_t *cuint32 = NULL;
	uint32_t opp = 0;

	cuint32 = fdt_getprop(fdt, subnode, "opp-supported-hw", NULL);
	if (!cuint32) {
		DMSG("Can't find property opp-supported-hw");
		return TEE_ERROR_GENERIC;
	}

	opp = fdt32_to_cpu(*cuint32);
	if (!stm32mp_supports_cpu_opp(opp)) {
		DMSG("Not supported opp-supported-hw %#"PRIx32, opp);
		return TEE_ERROR_NOT_SUPPORTED;
	}

	return TEE_SUCCESS;
}

static TEE_Result cpu_opp_pm(enum pm_op op, unsigned int pm_hint,
			     const struct pm_callback_handle *hdl __unused)
{
	assert(op == PM_OP_SUSPEND || op == PM_OP_RESUME);

#ifdef CFG_STM32MP15
	/* nothing to do if OPP is managed by Linux and not by SCMI */
	if (!cpu_opp.running)
		return TEE_SUCCESS;
#endif

	/* nothing to do if RCC clock tree is not lost */
	if (!PM_HINT_IS_STATE(pm_hint, CONTEXT))
		return TEE_SUCCESS;

	if (op == PM_OP_SUSPEND) {
		/*
		 * Force default OPP before suspend
		 */
		DMSG("Suspend to OPP %u", cpu_opp.default_opp);

		if (cpu_opp.current_opp == cpu_opp.default_opp)
			return TEE_SUCCESS;

		return set_opp(cpu_opp.default_opp);
	}

	if (op == PM_OP_RESUME) {
		DMSG("Resume to OPP %u", cpu_opp.current_opp);

		/*
		 * Optimize resume from Stop modes: do not reset
		 * the OPP if we where on default before suspend.
		 */
		if (!PM_HINT_IS_STATE(pm_hint, CONTEXT) &&
		    cpu_opp.current_opp == cpu_opp.default_opp)
			return TEE_SUCCESS;

		return set_opp(cpu_opp.current_opp);
	}

	return TEE_SUCCESS;
}
DECLARE_KEEP_PAGER_PM(cpu_opp_pm);

static TEE_Result stm32_cpu_opp_get_dt_subnode(const void *fdt, int node)
{
	const fdt64_t *cuint64 = NULL;
	const fdt32_t *cuint32 = NULL;
	uint64_t freq_hz = 0;
	uint64_t freq_khz = 0;
	uint64_t freq_khz_opp_def = 0;
	uint32_t volt_uv = 0;
	unsigned int i = 0;
	int subnode = 0;
	TEE_Result res = TEE_ERROR_GENERIC;

	cpu_opp.dvfs = calloc(CFG_STM32MP_OPP_COUNT, sizeof(*cpu_opp.dvfs));
	if (!cpu_opp.dvfs)
		return TEE_ERROR_OUT_OF_MEMORY;

	cpu_opp.opp_count = CFG_STM32MP_OPP_COUNT;

	fdt_for_each_subnode(subnode, fdt, node) {
		cuint64 = fdt_getprop(fdt, subnode, "opp-hz", NULL);
		if (!cuint64) {
			EMSG("Missing opp-hz in node %s",
			     fdt_get_name(fdt, subnode, NULL));
			res = TEE_ERROR_GENERIC;
			goto err;
		}

		freq_hz = fdt64_to_cpu(*cuint64);
		freq_khz = freq_hz / ULL(1000);
		if (freq_khz > (uint64_t)UINT32_MAX) {
			EMSG("Invalid opp-hz %"PRIu64" in node %s",
			     freq_khz, fdt_get_name(fdt, subnode, NULL));
			res = TEE_ERROR_GENERIC;
			goto err;
		}

		cuint32 = fdt_getprop(fdt, subnode, "opp-microvolt", NULL);
		if (!cuint32) {
			EMSG("Missing opp-microvolt in node %s",
			     fdt_get_name(fdt, subnode, NULL));
			res = TEE_ERROR_GENERIC;
			goto err;
		}

		volt_uv = fdt32_to_cpu(*cuint32);

		/* skip OPP when the SOC does not support it */
		if (stm32_cpu_opp_is_supported(fdt, subnode) != TEE_SUCCESS) {
			DMSG("Skip SoC OPP %"PRIu64"kHz/%"PRIu32"uV",
			     freq_khz, volt_uv);
			cpu_opp.opp_count--;
			continue;
		}

		/* skip OPP when voltage is not supported */
		if (!opp_voltage_is_supported(cpu_opp.regul, &volt_uv)) {
			DMSG("Skip volt OPP %"PRIu64"kHz/%"PRIu32"uV",
			     freq_khz, volt_uv);
			cpu_opp.opp_count--;
			continue;
		}

		if (i == cpu_opp.opp_count) {
			EMSG("Too many OPP defined in node %s",
			     fdt_get_name(fdt, node, NULL));
			res = TEE_ERROR_GENERIC;
			goto err;
		}

		cpu_opp.dvfs[i].freq_khz = freq_khz;
		cpu_opp.dvfs[i].volt_uv = volt_uv;

		DMSG("Found OPP %u (%"PRIu64"kHz/%"PRIu32"uV) from DT",
		     i, freq_khz, volt_uv);

		/* Select the max "st,opp-default" node as default OPP */
		if (fdt_getprop(fdt, subnode, "st,opp-default", NULL) &&
		    freq_khz > freq_khz_opp_def) {
			cpu_opp.default_opp = i;
			freq_khz_opp_def = freq_khz;
		}

		i++;
	}

	/* At least one OPP node shall have a "st,opp-default" property */
	if (freq_khz_opp_def == 0) {
		EMSG("No default OPP found");
		res = TEE_ERROR_GENERIC;
		goto err;
	}

	cpu_opp.opp_count = i;
	if (cpu_opp.opp_count == 0) {
		EMSG("No valid OPP found");
		res = TEE_ERROR_GENERIC;
		goto err;
	}

	/* Apply the default OPP */
	DMSG("Set OPP to %"PRIu64"kHz", freq_khz_opp_def);
	res = set_opp(cpu_opp.default_opp);
	if (res)
		goto err;

	cpu_opp.current_opp = cpu_opp.default_opp;

	register_pm_driver_cb(cpu_opp_pm, NULL, "cpu-opp");

	return TEE_SUCCESS;

err:
	free(cpu_opp.dvfs);
	cpu_opp.dvfs = NULL;
	cpu_opp.opp_count = 0;

	return res;
}

static TEE_Result
stm32_cpu_init(const void *fdt, int node, const void *compat_data __unused)
{
	const fdt32_t *cuint = NULL;
	int opp_node = 0;
	int len = 0;
	uint32_t phandle = 0;
	TEE_Result res = TEE_SUCCESS;
	uint16_t __maybe_unused cpu_voltage = 0;

	cuint = fdt_getprop(fdt, node, "operating-points-v2", &len);
	if (!cuint || len != sizeof(uint32_t)) {
		DMSG("No CPU operating points");
		return TEE_SUCCESS;
	}

	res = clk_dt_get_by_index(fdt, node, 0, &cpu_opp.clock);
	if (res)
		return res;

	res = regulator_dt_get_supply(fdt, node, "cpu", &cpu_opp.regul);
	if (res)
		return res;

	phandle = fdt32_to_cpu(*cuint);
	opp_node = fdt_node_offset_by_phandle(fdt, phandle);

#ifdef CFG_STM32MP15
	cpu_voltage = regulator_get_voltage(cpu_opp.regul);

	if (stm32mp1_clk_compute_all_pll1_settings(fdt, opp_node, cpu_voltage))
		panic();
#endif

	return stm32_cpu_opp_get_dt_subnode(fdt, opp_node);
}

static const struct dt_device_match stm32_cpu_match_table[] = {
#if defined(CFG_STM32MP13) || defined(CFG_STM32MP15)
	{ .compatible = "arm,cortex-a7" },
#endif
#if defined(CFG_STM32MP21) || defined(CFG_STM32MP23) || defined(CFG_STM32MP25)
	{ .compatible = "arm,cortex-a35" },
#endif
	{ }
};

DEFINE_DT_DRIVER(stm32_cpu_dt_driver) = {
	.name = "stm32-cpu",
	.match_table = stm32_cpu_match_table,
	.probe = &stm32_cpu_init,
};

static TEE_Result stm32_cpu_initcall(void)
{
	TEE_Result res = TEE_ERROR_GENERIC;
	const void *fdt = get_embedded_dt();
	int node = fdt_path_offset(fdt, "/cpus/cpu@0");

	if (node < 0) {
		panic("cannot find /cpus/cpu@0 node");
	}

	res = dt_driver_maybe_add_probe_node(fdt, node);
	if (res) {
		EMSG("Failed on node %s with %#"PRIx32,
		     fdt_get_name(fdt, node, NULL), res);
		panic();
	}

	return TEE_SUCCESS;
}

early_init(stm32_cpu_initcall);

