// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2017-2022, STMicroelectronics
 */

#include <arm.h>
#include <boot_api.h>
#include <config.h>
#include <console.h>
#include <drivers/clk.h>
#include <drivers/rstctrl.h>
#include <drivers/stm32mp_dt_bindings.h>
#include <drivers/stm32mp1_rcc.h>
#include <drivers/stm32mp1_pwr.h>
#include <drivers/gic.h>
#include <initcall.h>
#include <io.h>
#include <keep.h>
#include <kernel/cache_helpers.h>
#include <kernel/delay.h>
#include <kernel/boot.h>
#include <kernel/interrupt.h>
#include <kernel/misc.h>
#include <kernel/panic.h>
#include <kernel/pm.h>
#include <kernel/spinlock.h>
#include <mm/core_memprot.h>
#include <pm/stm32mp1_psci.h>
#include <platform_config.h>
#include <sm/pm.h>
#include <sm/psci.h>
#include <sm/std_smc.h>
#include <stm32_util.h>
#include <stm32mp_pm.h>
#include <string.h>
#include <trace.h>
#include <util.h>

#include "context.h"
#include "power.h"

#define CONSOLE_FLUSH_DELAY_MS		10

#define STM32_PM_CPU_STANDBY_FLAG		BIT(31)
#define STM32_PM_CARE_OTHER_CPU_STATE_FLAG	BIT(30)
#define STM32_PM_SOC_MODE_MASK			GENMASK_32(7, 0)

static_assert(STM32_PM_MAX_SOC_MODE ==
	      (STM32_PM_MAX_SOC_MODE & STM32_PM_SOC_MODE_MASK));

/*
 * SMP boot support and access to the mailbox
 */

enum core_state_id {
	CORE_OFF = 0,
	CORE_RET,
	CORE_AWAKE,
	CORE_ON,
};

static enum core_state_id core_state[2];
static unsigned int __maybe_unused state_lock = SPINLOCK_UNLOCK;

static uint32_t __maybe_unused lock_state_access(void)
{
	return may_spin_lock(&state_lock);
}

static void __maybe_unused unlock_state_access(uint32_t exceptions)
{
	may_spin_unlock(&state_lock, exceptions);
}

int psci_affinity_info(uint32_t affinity, uint32_t lowest_affinity_level)
{
	unsigned int pos = get_core_pos_mpidr(affinity);

	DMSG("core %zu, state %u", pos, core_state[pos]);

	if ((pos >= CFG_TEE_CORE_NB_CORE) ||
	    (lowest_affinity_level > PSCI_AFFINITY_LEVEL_ON)) {
		return PSCI_RET_INVALID_PARAMETERS;
	}

	switch (core_state[pos]) {
	case CORE_OFF:
	case CORE_RET:
		return PSCI_AFFINITY_LEVEL_OFF;
	case CORE_AWAKE:
		return PSCI_AFFINITY_LEVEL_ON_PENDING;
	case CORE_ON:
		return PSCI_AFFINITY_LEVEL_ON;
	default:
		panic();
	}
}

#if CFG_TEE_CORE_NB_CORE == 1
/*
 * Function called when a CPU is booted through the OP-TEE.
 * All cores shall register when online.
 */
void stm32mp_register_online_cpu(void)
{
	assert(core_state[0] == CORE_OFF || core_state[0] == CORE_RET);
	core_state[0] = CORE_ON;
}
#else
void stm32mp_register_online_cpu(void)
{
	size_t pos = get_core_pos();
	uint32_t exceptions = lock_state_access();

	if (pos == 0) {
		assert((core_state[pos] == CORE_OFF) ||
		       (core_state[pos] == CORE_RET));
	} else {
		if (!stm32mp_supports_second_core())
			return;

		if (core_state[pos] != CORE_AWAKE) {
			core_state[pos] = CORE_OFF;
			unlock_state_access(exceptions);
			stm32_pm_cpu_power_down_wfi();

			/* should not return */
			panic();
		}

		/* Balance BKPREG clock gating */
		clk_disable(stm32mp_rcc_clock_id_to_clk(RTCAPB));

		/* Clear hold in pen flag */
		io_write32(stm32mp_bkpreg(BCKR_CORE1_MAGIC_NUMBER),
			   BOOT_API_A7_RESET_MAGIC_NUMBER);

	}

	core_state[pos] = CORE_ON;
	unlock_state_access(exceptions);
}

#define GICD_SGIR		0xF00
static void raise_sgi0_as_secure(void)
{
	dsb_ishst();
	io_write32(get_gicd_base() + GICD_SGIR,
		   GIC_NON_SEC_SGI_0 | SHIFT_U32(TARGET_CPU1_GIC_MASK, 16));
}

static void release_secondary_early_hpen(size_t __unused pos)
{
	struct itr_chip *itr_chip = interrupt_get_main_chip();

	/* Need to send SIG#0 over Group0 after individual core 1 reset */
	raise_sgi0_as_secure();
	udelay(20);

	io_write32(stm32mp_bkpreg(BCKR_CORE1_BRANCH_ADDRESS),
		   TEE_LOAD_ADDR);
	io_write32(stm32mp_bkpreg(BCKR_CORE1_MAGIC_NUMBER),
		   BOOT_API_A7_CORE1_MAGIC_NUMBER);

	dsb_ishst();
	interrupt_raise_sgi(itr_chip, GIC_SEC_SGI_0, TARGET_CPU1_GIC_MASK);
}

/* Override default psci_cpu_on() with platform specific sequence */
int psci_cpu_on(uint32_t core_id, uint32_t entry, uint32_t context_id)
{
	size_t pos = get_core_pos_mpidr(core_id);
	uint32_t exceptions = 0;
	int rc = 0;

	if (!pos || pos >= CFG_TEE_CORE_NB_CORE)
		return PSCI_RET_INVALID_PARAMETERS;

	DMSG("core %zu, ns_entry 0x%" PRIx32 ", state %u",
		pos, entry, core_state[pos]);

	exceptions = lock_state_access();

	switch (core_state[pos]) {
	case CORE_ON:
		rc = PSCI_RET_ALREADY_ON;
		break;
	case CORE_AWAKE:
		rc = PSCI_RET_ON_PENDING;
		break;
	case CORE_RET:
		rc = PSCI_RET_DENIED;
		break;
	case CORE_OFF:
		core_state[pos] = CORE_AWAKE;
		rc = PSCI_RET_SUCCESS;
		break;
	default:
		panic();
	}

	unlock_state_access(exceptions);

	if (rc == PSCI_RET_SUCCESS) {
		boot_set_core_ns_entry(pos, entry, context_id);
		release_secondary_early_hpen(pos);
	}

	return rc;
}

/* Override default psci_cpu_off() with platform specific sequence */
int psci_cpu_off(void)
{
	unsigned int pos = get_core_pos();
	uint32_t exceptions = 0;

	if (pos == 0) {
		EMSG("PSCI_CPU_OFF not supported for core #0");
		return PSCI_RET_INTERNAL_FAILURE;
	}

	DMSG("core %u", pos);

	exceptions = lock_state_access();

	assert(core_state[pos] == CORE_ON);
	core_state[pos] = CORE_OFF;

	unlock_state_access(exceptions);

	/* Enable BKPREG access for the disabled CPU */
	if (clk_enable(stm32mp_rcc_clock_id_to_clk(RTCAPB)))
		panic();

	thread_mask_exceptions(THREAD_EXCP_ALL);
	stm32_pm_cpu_power_down_wfi();

	/* should not return */
	panic();
}
#endif

/* Start non-pageable section */
static int enter_cstop_suspend(unsigned int arg)
{
	unsigned int soc_mode = arg & STM32_PM_SOC_MODE_MASK;
	TEE_Result __maybe_unused ret = TEE_ERROR_GENERIC;
	int rc = 1;

	if (read_isr())
		return rc;

	stm32_enter_cstop(soc_mode);

#ifndef CFG_STM32MP1_OPTEE_IN_SYSRAM
	ret = stm32mp_pm_call_bl2_lp_entry(arg);
	if (ret == TEE_ERROR_BAD_STATE)
		rc = -1;
	else if (ret == TEE_SUCCESS)
		rc = 0;
#else
	if (need_to_backup_cpu_context(soc_mode)) {
		stm32_pm_cpu_power_down_wfi();
	} else {
		stm32_pm_cpu_wfi();
		rc = 0;
	}
#endif

	stm32_exit_cstop();

	return rc;
}
DECLARE_KEEP_PAGER(enter_cstop_suspend);

static int plat_suspend(uint32_t arg)
{
	unsigned int soc_mode = arg & STM32_PM_SOC_MODE_MASK;
	size_t pos = get_core_pos();
	int rc = 1;

	if (read_isr())
		return rc;

	/* No need to lock state access as CPU is alone when here */
	assert(core_state[pos] == CORE_ON);
	core_state[pos] = CORE_RET;

	if (stm32mp_pm_save_context(soc_mode) == TEE_SUCCESS)
		rc = enter_cstop_suspend(arg);

	stm32mp_pm_restore_context(soc_mode);
	stm32mp_pm_wipe_context();

	assert(core_state[pos] == CORE_RET);
	core_state[pos] = CORE_ON;

	return rc;
}

static void plat_resume(uint32_t arg)
{
	unsigned int soc_mode = arg;

	stm32mp_register_online_cpu();

	assert(core_state[get_core_pos()] == CORE_ON);

	stm32mp_pm_restore_context(soc_mode);
}

static bool plat_can_suspend(void)
{
	size_t pos = get_core_pos();
	size_t n = 0;
	uint32_t exceptions = 0;
	bool rc = true;

	/* STM32MP15 in SYSRAM requires STM32 RNG in order to suspend */
	if (IS_ENABLED(CFG_STM32MP15) && !IS_ENABLED(CFG_STM32_RNG) &&
	    IS_ENABLED(CFG_STM32MP1_OPTEE_IN_SYSRAM))
		return false;

	if (CFG_TEE_CORE_NB_CORE == 1)
		return true;

	exceptions = lock_state_access();

	for (n = 0; n < ARRAY_SIZE(core_state); n++) {
		if (n == pos)
			continue;

		if (core_state[n] == CORE_AWAKE) {
			/* State core as lost and proceed suspend */
			core_state[n] = CORE_OFF;
		}

		if (core_state[n] != CORE_OFF)
			rc = false;
	}

	unlock_state_access(exceptions);

	return rc;
}

/*
 * Note: this function is weak just to make it possible to exclude it from
 * the unpaged area.
 */
uint32_t __weak __psci_system_suspend(uintptr_t entry,
				      uint32_t context_id __unused,
				      struct sm_nsec_ctx *nsec)
{
	int ret = PSCI_RET_INVALID_PARAMETERS;
	uint32_t soc_mode = 0;
	int __maybe_unused pos = get_core_pos();

	DMSG("core %u", pos);

	if (!plat_can_suspend())
		return PSCI_RET_DENIED;

	soc_mode = stm32mp1_get_lp_soc_mode(PSCI_MODE_SYSTEM_SUSPEND);

	DMSG("Soc mode requested is %"PRIu32, soc_mode);

	if (soc_mode == STM32_PM_CSLEEP_RUN) {
		stm32_enter_csleep();
		nsec->mon_lr = (uint32_t)entry;
		return PSCI_RET_SUCCESS;
	}

	assert(cpu_mmu_enabled() && core_state[pos] == CORE_ON);

	if (need_to_backup_cpu_context(soc_mode)) {
#ifdef CFG_STM32MP1_OPTEE_IN_SYSRAM
		if (!stm32mp_supports_hw_cryp())
			return PSCI_RET_DENIED;
#endif

		if (!IS_ENABLED(CFG_PAGED_PSCI_SYSTEM_SUSPEND))
			sm_save_unbanked_regs(&nsec->ub_regs);

		/*
		 * sm_pm_cpu_suspend(arg, func) saves the CPU core context in
		 * TEE RAM then calls func(arg) to run the platform lower power
		 * sequence.
		 *
		 * If platform fails to suspend, sm_pm_cpu_suspend() returns
		 * with a non null return code. When sm_pm_cpu_suspend()
		 * returns 0 platform context must be restored.
		 */
		ret = sm_pm_cpu_suspend((uint32_t)soc_mode, plat_suspend);
		if (ret == 0) {
#ifndef CFG_STM32MP1_OPTEE_IN_SYSRAM
			stm32_exit_cstop();
#endif
			plat_resume((uint32_t)soc_mode);
			if (!IS_ENABLED(CFG_PAGED_PSCI_SYSTEM_SUSPEND))
				sm_restore_unbanked_regs(&nsec->ub_regs);
		}
#ifndef CFG_STM32MP1_OPTEE_IN_SYSRAM
		/*
		 * TF-A return to caller if standby is aborted (error)
		 * or is reduced to Stop mode with simple WFI exit
		 * so result OK as low power mode is performed (Stop)
		 * PS: for OPTEE in SYSRAM and aborted Standby,
		 *     stm32_pm_cpu_power_down_wfi() calls
		 *     stm32mp_sysram_resume()
		 *     and this part is NOT reached
		 */
		ret = 0;
#endif
	} else {
		ret = plat_suspend((uint32_t)soc_mode);
	}

	if (ret == 0) {
		nsec->mon_lr = (uint32_t)entry;
		IMSG("Resumed");
		return PSCI_RET_SUCCESS;
	}
	EMSG("Resumed with error (%d)", ret);

	return PSCI_RET_INTERNAL_FAILURE;
}

/* Override default psci_system_suspend() with platform specific sequence */
int psci_system_suspend(uintptr_t entry, uint32_t context_id,
			struct sm_nsec_ctx *nsec)
{
	return __psci_system_suspend(entry, context_id, nsec);
}

/*
 * Note: this function is weak just to make it possible to exclude it from
 * the unpaged area.
 */
void __weak __noreturn __psci_system_off(void)
{
	uint32_t soc_mode = stm32mp1_get_lp_soc_mode(PSCI_MODE_SYSTEM_OFF);

	DMSG("core %u", get_core_pos());

	stm32_enter_cstop_shutdown(soc_mode);
}

void __noreturn psci_system_off(void)
{
	__psci_system_off();
}

/* Override default psci_system_reset() with platform specific sequence */
void __noreturn psci_system_reset(void)
{
	do_reset("PSCI");
}

/* Override default psci_cpu_on() with platform supported features */
int psci_features(uint32_t psci_fid)
{
	switch (psci_fid) {
	case ARM_SMCCC_VERSION:
	case PSCI_PSCI_FEATURES:
	case PSCI_SYSTEM_RESET:
	case PSCI_VERSION:
		return PSCI_RET_SUCCESS;
	case PSCI_CPU_ON:
	case PSCI_CPU_OFF:
		return PSCI_RET_SUCCESS;
	case PSCI_SYSTEM_OFF:
		if (stm32mp_with_pmic())
			return PSCI_RET_SUCCESS;
		return PSCI_RET_NOT_SUPPORTED;
	case PSCI_SYSTEM_SUSPEND:
		return PSCI_RET_SUCCESS;
	case PSCI_CPU_SUSPEND:
		if (IS_ENABLED(CFG_STM32_PSCI_OSI))
			return PSCI_RET_SUCCESS | PSCI_FEATURE_OSI_SUPPORTED;
		return PSCI_RET_NOT_SUPPORTED;
	default:
		return PSCI_RET_NOT_SUPPORTED;
	}
}

/* Override default psci_version() to enable PSCI_VERSION_1_0 API */
uint32_t psci_version(void)
{
	return PSCI_VERSION_1_0;
}

#ifdef CFG_STM32_PSCI_OSI

int psci_set_system_suspend_mode(uint32_t mode)
{
	switch (mode) {
		case PSCI_SUSPEND_MODE_PLATFORM_COORDINATED:
			return PSCI_RET_SUCCESS;
		case PSCI_SUSPEND_MODE_OS_INITIATED:
			return PSCI_RET_SUCCESS;
		default:
			return PSCI_RET_INVALID_PARAMETERS;
	}
}

enum cstop_state_id {
	STATE_NONE = 0,
	STATE_AUTO_CSTOP_ENTRY
};

static unsigned int cstop_lock = SPINLOCK_UNLOCK;
static volatile enum cstop_state_id cstop_cpux_state[CFG_TEE_CORE_NB_CORE];
static volatile enum cstop_state_id cstop_enter;

static enum cstop_state_id get_locked(volatile enum cstop_state_id *state)
{
	uint32_t exceptions = 0;
	volatile enum cstop_state_id val = STATE_NONE;

	exceptions = may_spin_lock(&cstop_lock);
	val = *state;
	may_spin_unlock(&cstop_lock, exceptions);

	return val;
}

static void set_locked(volatile enum cstop_state_id *state,
		       enum cstop_state_id val)
{
	uint32_t exceptions = 0;

	exceptions = may_spin_lock(&cstop_lock);
	*state = val;
	may_spin_unlock(&cstop_lock, exceptions);
}

static inline size_t stm32_get_pos_other(void) {
	return get_core_pos() == 1 ? 0 : 1;
}

static int stm32_cpu_standby(void)
{
	TEE_Result tee_result __maybe_unused = TEE_ERROR_GENERIC;
	int ret = PSCI_RET_SUCCESS;
	size_t pos = get_core_pos();
	uint32_t exceptions = 0;

	set_locked(&cstop_cpux_state[pos], STATE_AUTO_CSTOP_ENTRY);

#ifdef CFG_STM32MP1_OPTEE_IN_SYSRAM
	/*
	 * Enter standby state.
	 * Synchronize on memory accesses and instruction flow before the WFI
	 * instruction.
	 */
	do {
		dsb();
		isb();
		wfi();
	} while (!read_isr());
#else
	tee_result = stm32mp_pm_call_bl2_lp_entry(STM32_PM_CPU_STANDBY_FLAG);
	if (tee_result == TEE_ERROR_BAD_STATE)
		ret = PSCI_RET_DENIED;
	else if (tee_result != TEE_SUCCESS)
		ret = PSCI_RET_INTERNAL_FAILURE;
#endif

	exceptions = may_spin_lock(&cstop_lock);

#ifdef CFG_STM32MP1_OPTEE_IN_SYSRAM
	if (cstop_enter == STATE_AUTO_CSTOP_ENTRY) {
		struct itr_chip *itr_chip = interrupt_get_main_chip();
		size_t other_pos = stm32_get_pos_other();

		interrupt_raise_sgi(itr_chip, GIC_SEC_SGI_6,
				    TARGET_CPUX_GIC_MASK(other_pos));
	}
#endif

	cstop_cpux_state[pos] = STATE_NONE;

	may_spin_unlock(&cstop_lock, exceptions);

	while (get_locked(&cstop_enter) != STATE_NONE)
		wfe();

	return ret;
}
DECLARE_KEEP_PAGER(stm32_cpu_standby);

static int stm32_pwr_domain_suspend(unsigned int soc_mode)
{
	size_t other_pos = stm32_get_pos_other();
	int ret = PSCI_RET_INTERNAL_FAILURE;
	uint32_t arg = (uint32_t)soc_mode;
	uint32_t exceptions = 0;
	bool denied = false;
	uint32_t scr = 0;
	int rc = 1;

	if (!stm32mp1_lp_soc_mode_is_supported(soc_mode))
		return PSCI_RET_INVALID_PARAMETERS;

	exceptions = may_spin_lock(&cstop_lock);

	/* If we are already handling a low power request */
	if (cstop_enter == STATE_AUTO_CSTOP_ENTRY)
		denied = true;

	if (core_state[other_pos] != CORE_OFF) {
		arg |= STM32_PM_CARE_OTHER_CPU_STATE_FLAG;

		/* If the other core is ON but not already in LP */
		if (cstop_cpux_state[other_pos] != STATE_AUTO_CSTOP_ENTRY)
			denied = true;
	}

	/* If an IRQ is pending */
	if (read_isr())
		denied = true;

	if (denied) {
		may_spin_unlock(&cstop_lock, exceptions);
		return PSCI_RET_DENIED;
	}

	cstop_enter = STATE_AUTO_CSTOP_ENTRY;

	may_spin_unlock(&cstop_lock, exceptions);

	scr = read_scr();
	write_scr(scr | SCR_IRQ | SCR_FIQ);

	rc = plat_suspend(arg);
	if (rc == -1)
		ret = PSCI_RET_DENIED;
	else if (rc == 0)
		ret = PSCI_RET_SUCCESS;

	interrupt_ack_sgi(interrupt_get_main_chip(),
			  GIC_SEC_SGI_6,
			  TARGET_CPUX_GIC_MASK(other_pos));

	write_scr(scr);
	set_locked(&cstop_enter, STATE_NONE);
	sev();

	return ret;
}

/*
 * Some part of this code are not able to handle more than 2 CPUs.
 */
static_assert(CFG_TEE_CORE_NB_CORE <= 2);

#define STM32_STATE_ID_CPU_PWRDN	U(0x2)
#define STM32_STATE_ID_STOP1		U(0x12)
#define STM32_STATE_ID_LP_STOP1		U(0x22)
#define STM32_STATE_ID_LPLV_STOP1	U(0x222)

/*
 * Note: this function is weak just to make it possible to exclude it from
 * the unpaged area.
 */
int __weak __psci_cpu_suspend(uint32_t power_state,
			      uintptr_t entry __unused,
			      uint32_t context_id __unused,
			      struct sm_nsec_ctx *nsec __unused)
{
	uint32_t state_id = 0;
	uint32_t state_type = 0;
	uint32_t power_level __maybe_unused = 0;
	int ret = PSCI_RET_INTERNAL_FAILURE;

	state_id = (power_state & PSCI_POWER_STATE_ID_MASK) >>
		   PSCI_POWER_STATE_ID_SHIFT;
	state_type = (power_state & PSCI_POWER_STATE_TYPE_MASK) >>
		     PSCI_POWER_STATE_TYPE_SHIFT;
	power_level = (power_state & PSCI_POWER_STATE_AFFL_MASK) >>
		      PSCI_POWER_STATE_AFFL_SHIFT;

	DMSG("power-level %#x state-type %#x state-id %#x",
	     power_level, state_type, state_id);

	if (state_type != PSCI_POWER_STATE_TYPE_STANDBY)
		return PSCI_RET_INVALID_PARAMETERS;

	switch (state_id) {
	case STM32_STATE_ID_CPU_PWRDN:
		ret = stm32_cpu_standby();
		break;
	case STM32_STATE_ID_STOP1:
		ret = stm32_pwr_domain_suspend(STM32_PM_CSTOP_ALLOW_STOP);
		break;
	case STM32_STATE_ID_LP_STOP1:
		ret = stm32_pwr_domain_suspend(STM32_PM_CSTOP_ALLOW_LP_STOP);
		break;
	case STM32_STATE_ID_LPLV_STOP1:
		ret = stm32_pwr_domain_suspend(STM32_PM_CSTOP_ALLOW_LPLV_STOP);
		break;
	default:
	}

	return ret;
}

/* Override default psci_cpu_suspend() with platform specific sequence */
int psci_cpu_suspend(uint32_t power_state, uintptr_t entry, uint32_t context_id,
		     struct sm_nsec_ctx *nsec)
{
	return __psci_cpu_suspend(power_state, entry, context_id, nsec);
}

#endif /* CFG_STM32_PSCI_OSI */
