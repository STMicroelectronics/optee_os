// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2023, STMicroelectronics
 */

#include <config.h>
#include <console.h>
#include <drivers/gic.h>
#include <drivers/stm32_bsec.h>
#include <drivers/rstctrl.h>
#include <drivers/stm32_rif.h>
#include <drivers/stm32_risab.h>
#include <drivers/stm32_risaf.h>
#include <drivers/stm32_serc.h>
#include <drivers/stm32_tamp.h>
#include <drivers/stm32_uart.h>
#include <drivers/stm32mp_dt_bindings.h>
#include <drivers/stm32mp25_pwr.h>
#if defined(CFG_STM32MP25) || defined(STM32MP23)
#include <drivers/stm32mp25_rcc.h>
#else /* defined(CFG_STM32MP21) */
#include <drivers/stm32mp21_rcc.h>
#endif
#include <initcall.h>
#include <kernel/abort.h>
#include <kernel/boot.h>
#include <kernel/delay.h>
#include <kernel/dt.h>
#include <kernel/interrupt.h>
#include <kernel/misc.h>
#include <kernel/spinlock.h>
#include <libfdt.h>
#include <mm/core_memprot.h>
#include <platform_config.h>
#include <stm32_util.h>
#include <stm32mp_pm.h>
#include <string.h>
#include <trace.h>
#include <util.h>

/* DBGMCU registers */
#define DBGMCU_CR			U(0x4)
#define DBGMCU_DBG_AUTH_DEV		U(0x104)

#define DBGMCU_CR_DBG_SLEEP		BIT(0)
#define DBGMCU_CR_DBG_STOP		BIT(1)
#define DBGMCU_CR_DBG_STANDBY		BIT(2)

register_phys_mem_pgdir(MEM_AREA_IO_NSEC, APB1_BASE, APB1_SIZE);

register_phys_mem_pgdir(MEM_AREA_IO_SEC, APB1_BASE, APB1_SIZE);
register_phys_mem_pgdir(MEM_AREA_IO_SEC, APB2_BASE, APB2_SIZE);
register_phys_mem_pgdir(MEM_AREA_IO_SEC, APB3_BASE, APB3_SIZE);
register_phys_mem_pgdir(MEM_AREA_IO_SEC, APB4_BASE, APB4_SIZE);
register_phys_mem_pgdir(MEM_AREA_IO_SEC, AHB2_BASE, AHB2_SIZE);
register_phys_mem_pgdir(MEM_AREA_IO_SEC, AHB3_BASE, AHB3_SIZE);
register_phys_mem_pgdir(MEM_AREA_IO_SEC, AHB4_BASE, AHB4_SIZE);
register_phys_mem_pgdir(MEM_AREA_IO_SEC, AHB5_BASE, AHB5_SIZE);
register_phys_mem_pgdir(MEM_AREA_IO_SEC, SAPB_BASE, SAPB_SIZE);
register_phys_mem_pgdir(MEM_AREA_IO_SEC, SAHB_BASE, SAHB_SIZE);

register_phys_mem_pgdir(MEM_AREA_IO_SEC, GIC_BASE, GIC_SIZE);

register_phys_mem_pgdir(MEM_AREA_IO_NSEC, DBGMCU_BASE, DBGMCU_SIZE);

#ifdef CFG_STM32_CM33TDCID
/* Map beginning SRAM1 as read-only non-secure for BSEC shadow */
register_phys_mem(MEM_AREA_RAM_NSEC, SRAM1_BASE, SIZE_4K);
#else
/* Map beginning SRAM1 as read write for BSEC shadow */
register_phys_mem(MEM_AREA_RAM_SEC, SRAM1_BASE, SIZE_4K);
#endif

#define _ID2STR(id)		(#id)
#define ID2STR(id)		_ID2STR(id)

static TEE_Result platform_banner(void)
{
	IMSG("Platform stm32mp2: DT %s", ID2STR(CFG_EMBED_DTB_SOURCE_FILE));
	IMSG("OP-TEE ST profile: %s", TO_STR(CFG_STM32MP_PROFILE));

	return TEE_SUCCESS;
}

service_init(platform_banner);

/*
 * Console
 *
 * CFG_STM32_EARLY_CONSOLE_UART specifies the ID of the UART used for
 * trace console. Value 0 disables the early console.
 *
 * We cannot use the generic serial_console support since probing
 * the console requires the platform clock driver to be already
 * up and ready which is done only once service_init are completed.
 */
static struct stm32_uart_pdata console_data;

void plat_console_init(void)
{
#ifdef CFG_STM32_UART
	/* Early console initialization before MMU setup */
	struct uart {
		paddr_t pa;
	} uarts[] = {
		[0] = { .pa = 0 },
		[1] = { .pa = USART1_BASE },
		[2] = { .pa = USART2_BASE },
		[3] = { .pa = USART3_BASE },
		[4] = { .pa = UART4_BASE },
		[5] = { .pa = UART5_BASE },
		[6] = { .pa = USART6_BASE },
		[7] = { .pa = UART7_BASE },
		[8] = { .pa = UART8_BASE },
		[9] = { .pa = UART9_BASE },
	};

	static_assert(ARRAY_SIZE(uarts) > CFG_STM32_EARLY_CONSOLE_UART);

	if (!uarts[CFG_STM32_EARLY_CONSOLE_UART].pa)
		return;

	/* No clock yet bound to the UART console */
	console_data.clock = NULL;
	stm32_uart_init(&console_data, uarts[CFG_STM32_EARLY_CONSOLE_UART].pa);
	register_serial_console(&console_data.chip);

	IMSG("Early console on UART#%u", CFG_STM32_EARLY_CONSOLE_UART);
#endif
}

vaddr_t stm32_rcc_base(void)
{
	static struct io_pa_va base = { .pa = RCC_BASE };

	return io_pa_or_va_secure(&base, 1);
}

static uintptr_t stm32_dbgmcu_base(void)
{
	static struct io_pa_va dbgmcu_base = { .pa = DBGMCU_BASE };

	return io_pa_or_va_nsec(&dbgmcu_base, 1);
}

void boot_primary_init_intc(void)
{
	gic_init(GIC_BASE + GICC_OFFSET, GIC_BASE + GICD_OFFSET);
}

void boot_secondary_init_intc(void)
{
	gic_init_per_cpu();
}

#ifdef CFG_STM32_RIF
void stm32_rif_access_violation_action(void)
{
#ifdef CFG_STM32_RISAF
	stm32_risaf_print_erroneous_data();
	stm32_risaf_clear_illegal_access_flags();
#endif
#ifdef CFG_STM32_RISAB
	stm32_risab_print_erroneous_data();
	stm32_risab_clear_illegal_access_flags();
#endif
}
#endif /* CFG_STM32_RIF */

bool stm32mp_allow_probe_shared_device(const void *fdt, int node)
{
	static int uart_console_node = -1;
	const char *compat = NULL;
	static bool once;

	if (!once) {
		get_console_node_from_dt((void *)fdt, &uart_console_node,
					 NULL, NULL);
		once = true;
	}

	compat = fdt_stringlist_get(fdt, node, "compatible", 0, NULL);

	/*
	 * Allow OP-TEE console to be shared with non-secure world.
	 * Allow GPU to be probed to handle GPU OPP.
	 */
	if (node == uart_console_node ||
	    !strcmp(compat, "vivante,gc"))
		return true;

	return false;
}

void plat_external_abort_handler(struct abort_info *ai __unused)
{
	/* External abort may be due to SERC events */
	stm32_serc_handle_ilac();
}

#ifdef CFG_STM32_BSEC3
void plat_bsec_get_static_cfg(struct stm32_bsec_static_cfg *cfg)
{
	cfg->base = BSEC3_BASE;
	cfg->mirror = SRAM1_BASE;
	cfg->upper_start = STM32MP2_UPPER_OTP_START;
	cfg->max_id = STM32MP2_OTP_MAX_ID;
}

#ifndef CFG_STM32_CM33TDCID
static TEE_Result init_debug(void)
{
	TEE_Result res = TEE_SUCCESS;
	struct clk *dbg_clk = stm32mp_rcc_clock_id_to_clk(CK_SYSDBG);
	uint32_t state = 0;

	res = stm32_bsec_get_state(&state);
	if (res)
		return res;

	if (state != BSEC_STATE_SEC_CLOSED) {
		struct clk __maybe_unused *dbgmcu_clk = NULL;

		if (IS_ENABLED(CFG_INSECURE))
			IMSG("WARNING: All debug access are allowed");

		res = stm32_bsec_write_debug_conf(STM32_BSEC_DEBUG_ALL);
		if (res)
			panic("Debug configuration failed");

		/* Enable DBG as used to access coprocessor debug registers */
		assert(dbg_clk);
		if (clk_enable(dbg_clk))
			panic("Could not enable debug clock");

#if defined(CFG_STM32MP21)
		dbgmcu_clk = stm32mp_rcc_clock_id_to_clk(CK_DBGMCU);

		assert(dbgmcu_clk);
		if (clk_enable(dbgmcu_clk))
			panic("Could not enable DBGMCU clock");

		stm32_bsec_mp21_ap0_unlock();

		/*
		 * Write a dummy value to trigger the full visibility
		 * of the debug port.
		 */
		io_write32(stm32_dbgmcu_base() + DBGMCU_DBG_AUTH_DEV, 1);
#endif
	}

	return res;
}
early_init_late(init_debug);
#endif /* !CFG_STM32_CM33TDCID */
#endif /* CFG_STM32_BSEC3 */

#ifdef CFG_STM32_CPU_OPP
bool stm32mp_supports_cpu_opp(uint32_t opp_id __maybe_unused)
{
#ifdef CFG_STM32_BSEC3
	static uint32_t part_number;
	uint32_t otp = 0;
	size_t bit_len = 0;
	uint32_t id = 0;

	if (stm32_bsec_find_otp_in_nvmem_layout("part_number_otp",
						&otp, NULL, &bit_len))
		return false;

	if (stm32_bsec_read_otp(&part_number, otp))
		return false;

	/* The bit 31 indicate support of 1.5GHz in RPN (variant d/f) */
	if (part_number & BIT(31))
		id = BIT(1);
	else
		id = BIT(0);

	return (opp_id & id) == id;
#elif CFG_STM32_CM33TDCID
	/* can't read OTP in M33TDCID to support all OPP */
	return true;
#else
	return false;
#endif
}
#endif /* CFG_STM32_CPU_OPP */

static bool stm32mp_supports_second_core(void)
{
	if (CFG_TEE_CORE_NB_CORE == 1)
		return false;

	return true;
}

void __noreturn plat_system_reset(const char *str __maybe_unused)
{
	struct rstctrl *rstctrl = NULL;

	if (stm32mp_supports_second_core()) {
		uint32_t target_mask = 0;

		if (get_core_pos() == 0)
			target_mask = TARGET_CPU1_GIC_MASK;
		else
			target_mask = TARGET_CPU0_GIC_MASK;

		interrupt_raise_sgi(interrupt_get_main_chip(),
				    CFG_HALT_CORES_SGI, target_mask);
		/* Wait that other core is halted */
		mdelay(1);
	}

	IMSG("Forced system reset: %s", str);
	console_flush();

	/* Request system reset to RCC driver */
	rstctrl = stm32mp_rcc_reset_id_to_rstctrl(SYS_R);
	rstctrl_assert(rstctrl);
	udelay(100);

	/* Cannot occur */
	panic();
}

/*
 * Handle low-power emulation mode
 * In PWR debug Standby mode, the low power modes is not signaled to external
 * regulators with the SOC signals PWR_CPU_ON and PWR_ON so the regulators
 * required by the boot devices are not restored after the simulated low power
 * modes.
 * On reference design with STPMIC25, the NRSTC1MS signals are connected to
 * pins PWRCTRL3 and to used to drive the PMIC regulators required by ROM code
 * to reboot for external mass-storage (D1 domain exits DStandby, not Standby).
 * To avoid a wake-up issue in ROM Code in simulated STANDBY low power mode,
 * we force the STPMIC25 regulators configuration with a pulse on NRSTC1MS pin
 * as a workaround.
 */
void stm32_debug_suspend(unsigned long a0)
{
	struct clk *dbgmcu_clk = stm32mp_rcc_clock_id_to_clk(CK_ICN_APBDBG);
	uint32_t dbgmcu_cr = U(0);
	static bool info_displayed;

	if (!stm32_bsec_self_hosted_debug_is_enabled())
		return;

	if (!dbgmcu_clk)
		return;

	if (clk_enable(dbgmcu_clk))
		return;

	dbgmcu_cr = io_read32(stm32_dbgmcu_base() + DBGMCU_CR);

	/* WARN ONCE when emulation debug is activated */
	if (!info_displayed &&
	    dbgmcu_cr & (DBGMCU_CR_DBG_SLEEP | DBGMCU_CR_DBG_STOP |
			 DBGMCU_CR_DBG_STANDBY)) {
		MSG("Low power emulation mode enable (DBGMCU_CR=%#"PRIx32")",
		    dbgmcu_cr);
		info_displayed = true;
	}

	/* Pulse on NRSTC1MS pin for standby request when PMIC is used */
	if (dbgmcu_cr & DBGMCU_CR_DBG_STANDBY && a0 >= PM_D2_LPLV_LEVEL &&
	    stm32_stpmic2_is_present())
		io_setbits32(stm32_rcc_base() + RCC_C1MSRDCR,
			     RCC_C1MSRDCR_C1MSRST);

	clk_disable(dbgmcu_clk);
}
