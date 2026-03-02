// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2025, STMicroelectronics - All Rights Reserved
 */
#include <drivers/clk.h>
#include <drivers/clk_dt.h>
#include <drivers/stm32_dbgmcu_mbx.h>
#include <io.h>
#include <kernel/dt.h>
#include <kernel/interrupt.h>
#include <kernel/notif.h>
#include <libfdt.h>
#include <psa_adac_platform.h>
#include <stdlib_ext.h>

/* DBGMCU mailbox offset register */
#define DBGMCU_DBG_AUTH_HOST		U(0x0)
#define DBGMCU_DBG_AUTH_DEV		U(0x4)
#define DBGMCU_DBG_AUTH_ACK		U(0x8)

/* DBGMCU_DBG_AUTH_ACK register*/
#define DBGMCU_DBG_AUTH_ACK_HOST	BIT(0)
#define DBGMCU_DBG_AUTH_ACK_DEV		BIT(1)

/* Timeout for mailbox read and write */
#define DBGMCU_MBX_DELAY_US		50
#define DBGMCU_MBX_USEC_PER_MSEC	UL(1000)

/* DBGMCU interrupts */
enum {
	DBGMCU_ITR_AUTH_WR, /* debugger_auth_write */
	DBGMCU_ITR_AUTH_RD, /* debugger_auth_read */
	DBGMCU_ITR_AUTH_NUM,
};

struct dbgmcu_device {
	struct clk *clock;
	vaddr_t base;

	size_t itr_irq[DBGMCU_ITR_AUTH_NUM];
	struct itr_chip *itr_chip[DBGMCU_ITR_AUTH_NUM];
	struct itr_handler *itr[DBGMCU_ITR_AUTH_NUM];
};

/* Only 1 instance of DBGMCU is expected per platform */
static struct dbgmcu_device dbgmcu_d;

TEE_Result stm32_dbgmcu_mbx_read_auth_host(uint32_t *value, uint32_t timeout_ms)
{
	vaddr_t reg_auth_ack = dbgmcu_d.base + DBGMCU_DBG_AUTH_ACK;
	uint32_t auth_ack = 0;

	/* Wait until the debugger has written DBG_AUTH_HOST */
	if (IO_READ32_POLL_TIMEOUT(reg_auth_ack, auth_ack,
				   auth_ack & DBGMCU_DBG_AUTH_ACK_HOST,
				   DBGMCU_MBX_DELAY_US,
				   timeout_ms * DBGMCU_MBX_USEC_PER_MSEC))
		return TEE_ERROR_TIMEOUT;

	*value = io_read32(dbgmcu_d.base + DBGMCU_DBG_AUTH_HOST);
	DMSG("%s: DBGMCU_DBG_AUTH_HOST = 0x%08x", __func__, *value);

	return TEE_SUCCESS;
}

TEE_Result stm32_dbgmcu_mbx_write_auth_dev(uint32_t value, uint32_t timeout_ms)
{
	vaddr_t reg_auth_ack = dbgmcu_d.base + DBGMCU_DBG_AUTH_ACK;
	uint32_t auth_ack = 0;

	/* Wait until debugger has read DBG_AUTH_DEV */
	if (IO_READ32_POLL_TIMEOUT(reg_auth_ack, auth_ack,
				   !(auth_ack & DBGMCU_DBG_AUTH_ACK_DEV),
				   DBGMCU_MBX_DELAY_US,
				   timeout_ms * DBGMCU_MBX_USEC_PER_MSEC))
		return TEE_ERROR_TIMEOUT;

	io_write32(dbgmcu_d.base + DBGMCU_DBG_AUTH_DEV, value);
	DMSG("%s: DBGMCU_DBG_AUTH_DEV = 0x%08x",
	     __func__, io_read32(dbgmcu_d.base + DBGMCU_DBG_AUTH_DEV));

	return TEE_SUCCESS;
}

/* TODO Maybe to move to platform */
static void stm32_dbgmcu_mbx_secure_debug(void)
{
	psa_adac_start_secure_debug();
}

static void stm32_dbgmcu_mbx_enable_interrupts(bool both)
{
	/* Enable detection of write DBG_AUTH_HOST, aka need to read it */
	interrupt_enable(dbgmcu_d.itr_chip[DBGMCU_ITR_AUTH_WR],
			 dbgmcu_d.itr[DBGMCU_ITR_AUTH_WR]->it);

	if (!both)
		return;

	/* Enable detection of read DBG_AUTH_DEV, aka need to write it */
	interrupt_enable(dbgmcu_d.itr_chip[DBGMCU_ITR_AUTH_RD],
			 dbgmcu_d.itr[DBGMCU_ITR_AUTH_RD]->it);
}

static void stm32_dbgmcu_mbx_disable_interrupts(void)
{
	/* Disable detection of write DBG_AUTH_HOST */
	interrupt_disable(dbgmcu_d.itr_chip[DBGMCU_ITR_AUTH_WR],
			  dbgmcu_d.itr[DBGMCU_ITR_AUTH_WR]->it);

	/* Disable detection of read DBG_AUTH_DEV */
	interrupt_disable(dbgmcu_d.itr_chip[DBGMCU_ITR_AUTH_RD],
			  dbgmcu_d.itr[DBGMCU_ITR_AUTH_RD]->it);
}

static enum itr_return stm32_dbgmcu_mbx_threaded_itr(void)
{
	DMSG("Process request from Debugger");

	stm32_dbgmcu_mbx_secure_debug();

	stm32_dbgmcu_mbx_enable_interrupts(true);

	return ITRR_HANDLED;
}

static enum itr_return stm32_dbgmcu_mbx_itr_auth_wr(struct itr_handler *h
						    __unused)
{
	uint32_t value = 0;

	DMSG("Request available from Debugger");

	if (notif_async_is_started(0)) {
		stm32_dbgmcu_mbx_disable_interrupts();
		notif_send_async(NOTIF_VALUE_DO_BOTTOM_HALF, 0);
	} else {
		/* Dummy read to avoid spurious IT */
		stm32_dbgmcu_mbx_read_auth_host(&value, 1000);
		EMSG("Request dropped (0x%x), async notifications not started",
		     value);
	}

	return ITRR_HANDLED;
}

static enum itr_return stm32_dbgmcu_mbx_itr_auth_rd(struct itr_handler *h
						    __unused)
{
	TEE_Result res __maybe_unused = TEE_ERROR_GENERIC;
	uint32_t value = 0xbadf00d1;

	/*
	 * We should theoretically never enter this interrupt handler. As per
	 * ADAC specification, "In this protocol, the debug host is always the
	 * Requester and initiator of commands.". That's why a dummy value is
	 * written.
	 */
	res = stm32_dbgmcu_mbx_write_auth_dev(value, 1000);

	DMSG("Reply requested by Debugger: res=%x", res);

	return ITRR_HANDLED;
}

static void yielding_stm32_dbgmcu_mbx_notif(struct notif_driver *ndrv __unused,
					    enum notif_event ev)
{
	vaddr_t reg_auth_ack = dbgmcu_d.base + DBGMCU_DBG_AUTH_ACK;

	switch (ev) {
	case NOTIF_EVENT_DO_BOTTOM_HALF:
		DMSG("Notif DO_BOTTOM_HALF");
		if (io_read32(reg_auth_ack) & DBGMCU_DBG_AUTH_ACK_HOST)
			stm32_dbgmcu_mbx_threaded_itr();
		break;
	case NOTIF_EVENT_STOPPED:
		DMSG("Notif STOPPED");
		break;
	default:
		EMSG("Unknown event %d", ev);
	}
}

static bool atomic_stm32_dbgmcu_mbx_notif(struct notif_driver *ndrv __unused,
					  enum notif_event ev __maybe_unused,
					  uint16_t guest_id __unused)
{
	assert(ev == NOTIF_EVENT_STARTED);

	return true;
}
DECLARE_KEEP_PAGER(atomic_stm32_dbgmcu_mbx_notif);

struct notif_driver stm32_dbgmcu_mbx_notif = {
	.atomic_cb = atomic_stm32_dbgmcu_mbx_notif,
	.yielding_cb = yielding_stm32_dbgmcu_mbx_notif,
};

static bool stm32_dbgmcu_mbx_early_request(void)
{
	vaddr_t reg_auth_ack = dbgmcu_d.base + DBGMCU_DBG_AUTH_ACK;

	return !!(io_read32(reg_auth_ack) & DBGMCU_DBG_AUTH_ACK_HOST);
}

static TEE_Result stm32_dbgmcu_mbx_parse_fdt(const void *fdt, int node)
{
	TEE_Result res = TEE_ERROR_GENERIC;
	struct dt_node_info dt_info = { };
	struct io_pa_va base = { };

	assert(fdt);

	fdt_fill_device_info(fdt, &dt_info, node);
	if (dt_info.status == DT_STATUS_DISABLED ||
	    dt_info.reg == DT_INFO_INVALID_REG ||
	    dt_info.clock == DT_INFO_INVALID_CLOCK)
		return TEE_ERROR_BAD_PARAMETERS;

	base.pa = dt_info.reg;
	dbgmcu_d.base = io_pa_or_va_secure(&base, dt_info.reg_size);

	res = interrupt_dt_get_by_name(fdt, node, "dbg_auth_wr",
				       &dbgmcu_d.itr_chip[DBGMCU_ITR_AUTH_WR],
				       &dbgmcu_d.itr_irq[DBGMCU_ITR_AUTH_WR]);
	if (res)
		return res;

	res = interrupt_dt_get_by_name(fdt, node, "dbg_auth_rd",
				       &dbgmcu_d.itr_chip[DBGMCU_ITR_AUTH_RD],
				       &dbgmcu_d.itr_irq[DBGMCU_ITR_AUTH_RD]);
	if (res)
		return res;

	return clk_dt_get_by_index(fdt, node, 0, &dbgmcu_d.clock);
}

static TEE_Result stm32_dbgmcu_mbx_probe(const void *fdt, int node,
					 const void *compat_data __unused)
{
	TEE_Result res = TEE_ERROR_GENERIC;

	/*
	 * Manage dependency on crypto, used for ADAC. It returns
	 * TEE_ERROR_DEFER_DRIVER_INIT if crypto is not yet initialized.
	 */
	res = dt_driver_get_crypto();
	if (res)
		return res;

	res = stm32_dbgmcu_mbx_parse_fdt(fdt, node);
	if (res)
		return res;

	res = clk_enable(dbgmcu_d.clock);
	if (res)
		return res;

	res = interrupt_create_handler(dbgmcu_d.itr_chip[DBGMCU_ITR_AUTH_WR],
				       dbgmcu_d.itr_irq[DBGMCU_ITR_AUTH_WR],
				       stm32_dbgmcu_mbx_itr_auth_wr,
				       &dbgmcu_d, 0,
				       &dbgmcu_d.itr[DBGMCU_ITR_AUTH_WR]);
	if (res)
		goto err_clk_disable;

	/* auth_read irq requires edge-triggered type as it is high on POR */
	res = interrupt_create_handler(dbgmcu_d.itr_chip[DBGMCU_ITR_AUTH_RD],
				       dbgmcu_d.itr_irq[DBGMCU_ITR_AUTH_RD],
				       stm32_dbgmcu_mbx_itr_auth_rd,
				       &dbgmcu_d, 0,
				       &dbgmcu_d.itr[DBGMCU_ITR_AUTH_RD]);
	if (res)
		goto err_itr_wr_free;

	if (IS_ENABLED(CFG_CORE_ASYNC_NOTIF))
		notif_register_driver(&stm32_dbgmcu_mbx_notif);

	io_write32(dbgmcu_d.base + DBGMCU_DBG_AUTH_DEV, 0xbadf00d0);

	/* Before enabling interrupts, handle early debug request */
	if (stm32_dbgmcu_mbx_early_request())
		stm32_dbgmcu_mbx_secure_debug();
	else
		DMSG("No early debug request");

	stm32_dbgmcu_mbx_enable_interrupts(false);

	IMSG("DBGMCU Authenticated Debug Mailbox online");

	return TEE_SUCCESS;

err_itr_wr_free:
	interrupt_remove_free_handler(dbgmcu_d.itr[DBGMCU_ITR_AUTH_WR]);

err_clk_disable:
	clk_disable(dbgmcu_d.clock);

	EMSG("DBGMCU Authenticated Debug Mailbox offline");

	return res;
}

static const struct dt_device_match stm32_dbgmcu_mbx_match_table[] = {
	{ .compatible = "st,stm32mp21-dbgmcu-mbx" },
	{ }
};

DEFINE_DT_DRIVER(stm32_dbgmcu_mbx_dt_driver) = {
	.name = "stm32-dbgmcu-mbx",
	.match_table = stm32_dbgmcu_mbx_match_table,
	.probe = stm32_dbgmcu_mbx_probe,
};
