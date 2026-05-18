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
#include <drivers/stm32mp_dt_bindings.h>
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
#if defined(CFG_STM32MP21) || defined(CFG_STM32MP23) || defined(CFG_STM32MP25)
#include <stm32mp_pm.h>
#include <stm32_sysconf.h>
#endif
#include <stm32_util.h>
#include <trace.h>

#define TIMEOUT_US_OPP			U(1000)

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

#if defined(CFG_STM32MP21) || defined(CFG_STM32MP23) || defined(CFG_STM32MP25)
/*
 * To guarantee a correct synchronization of the ARM counter with STGEN and
 * TSGEN counter, they has to be isolated during DVFS request with LPI register.
 * Once this is done on STGEN, the ARM generic timer and associated delays or
 * timeouts are not functional.
 */
static void ca35ss_lpi_isolate(bool *tsgen)
{
	struct clk *dbg_clk = stm32mp_rcc_clock_id_to_clk(CK_KER_TSDBG);
	uint64_t timeout = 0;
	uint32_t counter = UINT32_MAX;

	*tsgen = false;

	/* Isolate TSGEN for debug only if associated clock is enabled */
	if (clk_is_enabled(dbg_clk)) {
		stm32mp_syscfg_write(CA35SS_SSC_LPI_TSGEN_NTS_CR, 0,
				     CA35SS_SSC_LPI_TSGEN_CSYSREQ);
		timeout = timeout_init_us(TIMEOUT_US_OPP);
		while (stm32mp_syscfg_read(CA35SS_SSC_LPI_TSGEN_NTS_CR)
			& CA35SS_SSC_LPI_TSGEN_CSYSACK) {
			if (timeout_elapsed(timeout))
				panic("Timeout CA35SS_SSC_LPI_TSGEN_CSYSACK");
		}
		*tsgen = true;
	}

	/* Isolate STGEN: Clear bit and waiting ACK */
	stm32mp_syscfg_write(CA35SS_SSC_LPI_STGEN_NTS_CR, 0,
			     CA35SS_SSC_LPI_STGEN_CSYSREQ);
	while (stm32mp_syscfg_read(CA35SS_SSC_LPI_STGEN_NTS_CR)
		& CA35SS_SSC_LPI_STGEN_CSYSACK) {
		/* With STGEN isolated, timer is not functional */
		counter--;
		if (!counter)
			panic("Timeout CA35SS_SSC_LPI_STGEN_CSYSACK");
	}
}

static void ca35ss_lpi_restore(bool tsgen)
{
	uint64_t timeout = 0;
	uint32_t counter = UINT32_MAX;

	/* Set bit REQ for STGEN and polling ACK */
	stm32mp_syscfg_write(CA35SS_SSC_LPI_STGEN_NTS_CR,
			     CA35SS_SSC_LPI_STGEN_CSYSREQ,
			     CA35SS_SSC_LPI_STGEN_CSYSREQ);
	while (!(stm32mp_syscfg_read(CA35SS_SSC_LPI_STGEN_NTS_CR)
		 & CA35SS_SSC_LPI_STGEN_CSYSACK)) {
		/* With STGEN isolated, timer is not functional */
		counter--;
		if (!counter)
			panic("Timeout CA35SS_SSC_LPI_STGEN_CSYSACK");
	}

	/* Set bit REQ for TSGEN and polling ACK */
	if (tsgen) {
		stm32mp_syscfg_write(CA35SS_SSC_LPI_TSGEN_NTS_CR,
				     CA35SS_SSC_LPI_TSGEN_CSYSREQ,
				     CA35SS_SSC_LPI_TSGEN_CSYSREQ);
		timeout = timeout_init_us(TIMEOUT_US_OPP);
		while (!(stm32mp_syscfg_read(CA35SS_SSC_LPI_TSGEN_NTS_CR)
			 & CA35SS_SSC_LPI_TSGEN_CSYSACK)) {
			if (timeout_elapsed(timeout))
				panic("Timeout CA35SS_SSC_LPI_TSGEN_CSYSACK");
		}
	}
}

/*
 * Configure CA35SS for new OPP with syscfg register SSC_MEM and OPP_REQ
 * the parameter overdrive indicates if the new OPP has an overdrive frequency
 * NB: as STGEN is deactivated during OPP request, use polling without timeout
 */
static void ca35ss_configure_opp(unsigned int overdrive)
{
	uint32_t mem_ctrl = stm32mp_syscfg_read(CA35SS_SSC_MEM_CTRL);
	bool tsgen = false;
	uint32_t counter = UINT32_MAX;
	uint32_t exceptions;

	/*
	 * SSC_MEM control the speed of memories inside Cortex-A35
	 * - RME: 0 default margin setting for nominal Cortex-A35 frequencies
	 * - RME: 1 margin setting from RM[2:0]
	 * - RM: CA35SS_SSC_MEM_CTRL_RM_OVERDRIVE the recommended value for
	 *       Cortex-A35 overdrive frequencies, not used if RME = 0
	 *
	 * This function configure the CA35SS values only if the OPP change
	 * (nominal vs overdrive)
	 */
	if (overdrive) {
		if (mem_ctrl & CA35SS_SSC_MEM_CTRL_RME)
			return;

		stm32mp_syscfg_write(CA35SS_SSC_MEM_CTRL,
				     CA35SS_SSC_MEM_CTRL_RME |
				     CA35SS_SSC_MEM_CTRL_RM_OVERDRIVE,
				     CA35SS_SSC_MEM_CTRL_RME |
				     CA35SS_SSC_MEM_CTRL_RM_MASK);
	} else {
		if (!(mem_ctrl & CA35SS_SSC_MEM_CTRL_RME))
			return;

		stm32mp_syscfg_write(CA35SS_SSC_MEM_CTRL,
				     CA35SS_SSC_MEM_CTRL_RM_OVERDRIVE,
				     CA35SS_SSC_MEM_CTRL_RME |
				     CA35SS_SSC_MEM_CTRL_RM_MASK);
	}

	exceptions = thread_mask_exceptions(THREAD_EXCP_ALL);

	ca35ss_lpi_isolate(&tsgen);

	/* Launch the OPP request*/
	stm32mp_syscfg_write(CA35SS_SSC_OPP_REQ, CA35SS_SSC_OPP_REQ_REQ,
			     CA35SS_SSC_OPP_REQ_REQ);

	/* Wait for the request to be complete */
	while (!(stm32mp_syscfg_read(CA35SS_SSC_OPP_REQ)
		 & CA35SS_SSC_OPP_REQ_ACK)) {
		/* After LPI isolate, the ARM generic timer is frozen  */
		counter--;
		if (!counter)
			panic("Timeout CA35SS_SSC_OPP_REQ_ACK");
	}

	/* Stop the OPP request */
	stm32mp_syscfg_write(CA35SS_SSC_OPP_REQ, 0, CA35SS_SSC_OPP_REQ_REQ);

	ca35ss_lpi_restore(tsgen);

	/* Restore interrupts */
	thread_unmask_exceptions(exceptions);
}

static TEE_Result set_opp(unsigned int opp)
{
	bool overdrive = false;
	unsigned int def_freq_khz = cpu_opp.dvfs[cpu_opp.default_opp].freq_khz;
	unsigned int opp_freq_khz = cpu_opp.dvfs[opp].freq_khz;
	int opp_volt_uv = cpu_opp.dvfs[opp].volt_uv;
	int res = TEE_ERROR_GENERIC;

	if (opp_freq_khz == clk_get_rate(cpu_opp.clock))
		return TEE_SUCCESS;

	/* The default OPP is the OPP with nominal frequency */
	if (opp_freq_khz > def_freq_khz)
		overdrive = true;

	/* switch to bypass and disable PLL1 */
	clk_set_parent(cpu_opp.clock,
		       clk_get_parent_by_index(cpu_opp.clock, 1));

	/* Change OPP/RM(nominal) when moving OPP Overdrive to Nominal */
	if (!overdrive)
		ca35ss_configure_opp(false);

	/* Update voltage */
	res = regulator_set_voltage(cpu_opp.regul, opp_volt_uv);
	if (res) {
		EMSG("Failed to set OPP %uuV", opp_volt_uv);
		return res;
	}

	/* Change OPP/RM(overdrive) when moving OPP Nominal to Overdrive */
	if (overdrive)
		ca35ss_configure_opp(true);

	/* Configure and start PLL1 */
	if (clk_set_rate(cpu_opp.clock, opp_freq_khz * 1000UL)) {
		EMSG("Failed to set OPP %ukHz", opp_freq_khz);
		return TEE_ERROR_GENERIC;
	}

	return TEE_SUCCESS;
}
#endif /* CFG_STM32MP21 || CFG_STM32MP23 || CFG_STM32MP25*/

#ifdef CFG_SCPFW_MOD_DVFS
/* Request to switch to CPU operating point related to @rate */
TEE_Result stm32_cpu_opp_set_rate(unsigned int rate)
{
	size_t opp = 0;
	uint64_t rate_khz = rate / 1000UL;
	TEE_Result res = TEE_ERROR_GENERIC;

	for (opp = 0U; opp < cpu_opp.opp_count; opp++)
		if (cpu_opp.dvfs[opp].freq_khz == rate_khz)
			break;

	if (opp > cpu_opp.opp_count)
		return TEE_ERROR_BAD_PARAMETERS;

	/* set OPP: configure clock and voltage with SoC function */
	res = set_opp(opp);
	if (!res)
		cpu_opp.current_opp = opp;

	return res;
}

/* Get rate related to current CPU operating point */
unsigned int stm32_cpu_opp_get_rate(void)
{
	return clk_get_rate(cpu_opp.clock);
}

/* Request to CPU operating point related to @level */
TEE_Result stm32_cpu_opp_get_rate_for_level(unsigned int level,
					    unsigned int *rate)
{
	if (level >= cpu_opp.opp_count)
		return TEE_ERROR_BAD_PARAMETERS;

	*rate = cpu_opp.dvfs[level].freq_khz * 1000UL;

	return TEE_SUCCESS;
}

static int cmp_cpu_opp_by_freq(const void *a, const void *b)
{
	const struct cpu_dvfs *opp_a = a;
	const struct cpu_dvfs *opp_b = b;

	assert(opp_a->freq_khz != opp_b->freq_khz);

	return CMP_TRILEAN(opp_a->freq_khz, opp_b->freq_khz);
}

TEE_Result optee_scmi_server_cpu_dvfs(int perf_id,
				      struct scpfw_channel_config *channel_cfg)
{
	unsigned int *dvfs_khz = NULL;
	unsigned int *dvfs_mv = NULL;
	size_t opp = 0;
	struct cpu_dvfs *sorted_dvfs = NULL;

	assert(channel_cfg && cpu_opp.opp_count);

	sorted_dvfs = calloc(cpu_opp.opp_count, sizeof(*sorted_dvfs));
	dvfs_khz = calloc(cpu_opp.opp_count, sizeof(*dvfs_khz));
	dvfs_mv = calloc(cpu_opp.opp_count, sizeof(*dvfs_mv));
	if (!sorted_dvfs || !dvfs_khz || !dvfs_mv) {
		free(sorted_dvfs);
		free(dvfs_mv);
		free(dvfs_khz);

		return TEE_ERROR_OUT_OF_MEMORY;
	}

	/*
	 * Sort operating points by increasing frequencies as expected by
	 * SCP-firmware DVFS module.
	 */
	memcpy(sorted_dvfs, cpu_opp.dvfs,
	       cpu_opp.opp_count * sizeof(*sorted_dvfs));

	qsort(sorted_dvfs, cpu_opp.opp_count, sizeof(*sorted_dvfs),
	      cmp_cpu_opp_by_freq);

	/* Feed SCP-firmware with CPU DVFS configuration data */
	for (opp = 0; opp < cpu_opp.opp_count; opp++) {
		dvfs_khz[opp] = sorted_dvfs[opp].freq_khz;
		dvfs_mv[opp] = sorted_dvfs[opp].volt_uv / U(1000);
	}

	free(sorted_dvfs);

	channel_cfg->perfd[perf_id] = (struct scmi_perfd){
		.name = "CPU DVFS",
		.initial_opp = cpu_opp.default_opp,
		.dvfs_opp_count = cpu_opp.opp_count,
		.dvfs_opp_khz = dvfs_khz,
		.dvfs_opp_mv = dvfs_mv,
		.opp_id = OPP_ID_CPU,
	};

	return TEE_SUCCESS;
}
#endif /*CFG_SCPFW_MOD_DVFS*/

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

#if defined(CFG_STM32MP25) || defined(CFG_STM32MP23) || defined(CFG_STM32MP21)
	/* Nothing to do on Stop1, LP-Stop1 modes */
	if (PM_HINT_PLATFORM_STATE(pm_hint) == PM_CORE_LEVEL)
		return TEE_SUCCESS;
#else
	/* nothing to do if RCC clock tree is not lost */
	if (!PM_HINT_IS_STATE(pm_hint, CONTEXT))
		return TEE_SUCCESS;
#endif

	if (op == PM_OP_SUSPEND) {
#ifdef CFG_SCPFW_MOD_DVFS
		unsigned int opp = 0;

		/*
		 * When CFG_SCPFW_MOD_DVFS is enabled, save CPU OPP on suspend
		 * for restoration at resume. If CPU is not in an expected
		 * OPP state, fallback to default OPP at PM resume.
		 */
		unsigned int cur_khz = 0;
		int cur_uv = 0;

		cur_khz = clk_get_rate(cpu_opp.clock) / 1000;

		cur_uv = regulator_get_voltage(cpu_opp.regul);
		if (!cur_uv)
			panic();

		/* Get current OPP or fallback to default */
		for (opp = 0; opp < cpu_opp.opp_count; opp++)
			if (cur_khz == cpu_opp.dvfs[opp].freq_khz  &&
			    cur_uv == cpu_opp.dvfs[opp].volt_uv)
				break;
		if (opp >= cpu_opp.opp_count) {
			EMSG("Unexpected OPP state, select default");

			opp = cpu_opp.default_opp;
		}
		cpu_opp.current_opp = opp;
#endif
		/*
		 * Force default OPP before suspend
		 * On STM32MP2, it is not allowed to enter LPLV-Stop
		 * with overdrive voltage on VDDCPU
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

