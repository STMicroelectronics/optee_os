// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2017-2018, STMicroelectronics
 */

#include <compiler.h>
#include <console.h>
#include <drivers/clk.h>
#include <drivers/clk_dt.h>
#include <drivers/serial.h>
#include <drivers/stm32_gpio.h>
#include <drivers/stm32_uart.h>
#include <io.h>
#include <keep.h>
#include <kernel/delay.h>
#include <kernel/dt.h>
#include <kernel/panic.h>
#include <kernel/pm.h>
#include <libfdt.h>
#include <stm32_util.h>
#include <util.h>

#define UART_REG_CR1			0x00	/* Control register 1 */
#define UART_REG_CR2			0x04	/* Control register 2 */
#define UART_REG_CR3			0x08	/* Control register 3 */
#define UART_REG_BRR			0x0c	/* Baud rate register */
#define UART_REG_RQR			0x18	/* Request register */
#define UART_REG_ISR			0x1c	/* Interrupt & status reg. */
#define UART_REG_ICR			0x20	/* Interrupt flag clear reg. */
#define UART_REG_RDR			0x24	/* Receive data register */
#define UART_REG_TDR			0x28	/* Transmit data register */
#define UART_REG_PRESC			0x2c	/* Prescaler register */

#define PUTC_TIMEOUT_US			1000
#define FLUSH_TIMEOUT_US		16000

/*
 * Uart Interrupt & status register bits
 *
 * Bit 5 RXNE: Read data register not empty/RXFIFO not empty
 * Bit 6 TC: Transmission complete
 * Bit 7 TXE/TXFNF: Transmit data register empty/TXFIFO not full
 * Bit 23 TXFE: TXFIFO empty
 */
#define USART_ISR_RXNE_RXFNE		BIT(5)
#define USART_ISR_TC			BIT(6)
#define USART_ISR_TXE_TXFNF		BIT(7)
#define USART_ISR_TXFE			BIT(23)

/*
 * UART Configuration register 1
 *
 * Bit 0 UE: UART Enable
 * Bit 2 RE: Reception Enable
 * Bit 3 TE: Transmission Enable
 * Bit 15 OVER8: Oversampling by 8
 * Bit 29 FIFOEN: UART Fifo Enable
 */
#define UART_REG_CR1_UE			BIT(0)
#define UART_REG_CR1_RE			BIT(2)
#define UART_REG_CR1_TE			BIT(3)
#define UART_REG_CR1_OVER8		BIT(15)
#define UART_REG_CR1_FIFOEN		BIT(29)

#define UART_MAX_PRESC			256
#define UART_REG_BRR_M_SHIFT		4
#define UART_REG_BRR_MASK		GENMASK_32(15, 0)

static bool loc_chip_clk_is_enabled(struct serial_chip *chip)
{
	struct stm32_uart_pdata *pd = container_of(chip,
						   struct stm32_uart_pdata,
						   chip);

	if (pd->clock && pd->clock->ops->is_enabled)
		return pd->clock->ops->is_enabled(pd->clock);

	/* For early console, assume the clock is enable */
	return true;
}

static vaddr_t loc_chip_to_base(struct serial_chip *chip)
{
	struct stm32_uart_pdata *pd = NULL;

	pd = container_of(chip, struct stm32_uart_pdata, chip);

	return io_pa_or_va(&pd->base, 1);
}

static void loc_flush(struct serial_chip *chip)
{
	vaddr_t base = loc_chip_to_base(chip);
	uint64_t timeout = timeout_init_us(FLUSH_TIMEOUT_US);

	if (!loc_chip_clk_is_enabled(chip))
		return;

	while (!(io_read32(base + UART_REG_ISR) & USART_ISR_TC))
		if (timeout_elapsed(timeout))
			return;
}

static void loc_putc(struct serial_chip *chip, int ch)
{
	vaddr_t base = loc_chip_to_base(chip);
	uint64_t timeout = timeout_init_us(PUTC_TIMEOUT_US);

	if (!loc_chip_clk_is_enabled(chip))
		return;

	while (!(io_read32(base + UART_REG_ISR) & USART_ISR_TXE_TXFNF))
		if (timeout_elapsed(timeout))
			return;

	io_write32(base + UART_REG_TDR, ch);
}

static bool loc_have_rx_data(struct serial_chip *chip)
{
	vaddr_t base = loc_chip_to_base(chip);

	return io_read32(base + UART_REG_ISR) & USART_ISR_RXNE_RXFNE;
}

static int loc_getchar(struct serial_chip *chip)
{
	vaddr_t base = loc_chip_to_base(chip);

	while (!loc_have_rx_data(chip))
		;

	return io_read32(base + UART_REG_RDR) & 0xff;
}

static const struct serial_ops stm32_uart_serial_ops = {
	.flush = loc_flush,
	.putc = loc_putc,
	.have_rx_data = loc_have_rx_data,
	.getchar = loc_getchar,

};
DECLARE_KEEP_PAGER(stm32_uart_serial_ops);

void stm32_uart_init(struct stm32_uart_pdata *pd, vaddr_t base)
{
	pd->base.pa = base;
	pd->chip.ops = &stm32_uart_serial_ops;
}

static TEE_Result stm32_uart_update_baudrate(struct stm32_uart_pdata *pd,
					     char *params)
{
	uint64_t clk_rate = 0;
	uint32_t brr = 0;
	uint32_t baudrate = 0;
	uint32_t mantissa = 0;
	uint32_t uartdiv = 0;
	uint32_t fraction = 0;
	unsigned int i = 0;
	int oversampling = 0;
	uint32_t over8 = 0;
	vaddr_t uart_base = io_pa_or_va(&pd->base, 1);
	uint32_t presc[] = { 1, 2, 6, 8, 10, 12, 16, 32, 64, 128, 256 };

	clk_rate = clk_get_rate(pd->clock);
	if (!clk_rate || clk_enable(pd->clock))
		panic();

	baudrate = strtoul(params, NULL, 10);

	for (i = 0; i < ARRAY_SIZE(presc); i++) {
		uartdiv = UDIV_ROUND_NEAREST(clk_rate, baudrate * presc[i]);
		if (uartdiv < 16) {
			oversampling = 8;
			uartdiv = uartdiv * 2;
			over8 = UART_REG_CR1_OVER8;
		} else {
			oversampling = 16;
		}
		mantissa = SHIFT_U32(uartdiv / oversampling,
				     UART_REG_BRR_M_SHIFT);
		fraction = uartdiv % oversampling;
		brr = mantissa | fraction;
		if (brr <= UART_REG_BRR_MASK)
			break;
		if (i == ARRAY_SIZE(presc))
			return TEE_ERROR_GENERIC;
	}
	io_write32(uart_base + UART_REG_BRR, brr);
	io_write32(uart_base + UART_REG_PRESC, presc[i]);
	io_write32(uart_base + UART_REG_CR1, UART_REG_CR1_UE |
			   UART_REG_CR1_RE | UART_REG_CR1_TE |
			   UART_REG_CR1_FIFOEN | over8);

	return TEE_SUCCESS;
}

static TEE_Result parse_dt(void *fdt, int node, struct stm32_uart_pdata *pd)
{
	struct dt_node_info info = { };
	TEE_Result res = TEE_ERROR_GENERIC;

	res = clk_dt_get_by_index(fdt, node, 0, &pd->clock);
	if (res)
		return res;

	res = pinctrl_get_state_by_name(fdt, node, "default", &pd->pinctrl);
	if (res)
		return res;

	fdt_fill_device_info(fdt, &info, node);

	assert(info.reg != DT_INFO_INVALID_REG &&
	       info.reg_size != DT_INFO_INVALID_REG_SIZE);

	pd->chip.ops = &stm32_uart_serial_ops;
	pd->base.pa = info.reg;

	assert(cpu_mmu_enabled());
	pd->base.va = (vaddr_t)phys_to_virt(pd->base.pa, MEM_AREA_IO_SEC,
					    info.reg_size);

	return TEE_SUCCESS;
}

static void setup_resources(struct stm32_uart_pdata *pd)
{
	if (clk_enable(pd->clock))
		panic();

	if (pinctrl_apply_state(pd->pinctrl))
		panic();
}

static bool uart_is_for_console(void *fdt, int node, char **params)
{
	static int uart_console_node = -1;
	TEE_Result res = TEE_ERROR_GENERIC;

	if (uart_console_node < 0) {
		res = get_console_node_from_dt(fdt, &uart_console_node, NULL,
					       params);
		if (res == TEE_ERROR_ITEM_NOT_FOUND)
			return false;
		if (res != TEE_SUCCESS) {
			EMSG("Error finding a console UART device");
			return false;
		}
	}

	return uart_console_node == node;
}

static TEE_Result stm32_uart_pm(enum pm_op op, uint32_t pm_hint,
				const struct pm_callback_handle *pm_handle)
{
	struct stm32_uart_pdata *pd = pm_handle->handle;
	vaddr_t uart_base = io_pa_or_va(&pd->base, 1);
	TEE_Result res = TEE_ERROR_BAD_PARAMETERS;

	if (!PM_HINT_IS_STATE(pm_hint, CONTEXT))
		return TEE_SUCCESS;

	if (pd->is_console && !pd->clock->ops->is_enabled(pd->clock)) {
		/*
		 * Maybe the console clock is disabled by non-secure
		 * or disabled after wake-up from standby,
		 * in this case do nothing.
		 */
		return TEE_SUCCESS;
	}

	switch (op) {
	case PM_OP_SUSPEND:
		pd->brr = io_read32(uart_base + UART_REG_BRR);
		pd->presc = io_read32(uart_base + UART_REG_PRESC);
		pd->cr1 = io_read32(uart_base + UART_REG_CR1);
		pd->cr2 = io_read32(uart_base + UART_REG_CR2);
		pd->cr3 = io_read32(uart_base + UART_REG_CR3);
		res = TEE_SUCCESS;
		break;
	case PM_OP_RESUME:
		/* Disable UART to set CR2, CR3, BRR and PRESC saved values. */
		io_write32(uart_base + UART_REG_CR1, 0);
		io_write32(uart_base + UART_REG_CR2, pd->cr2);
		io_write32(uart_base + UART_REG_CR3, pd->cr3);
		io_write32(uart_base + UART_REG_BRR, pd->brr);
		io_write32(uart_base + UART_REG_PRESC, pd->presc);
		/*
		 * Set CR1 saved value.
		 * It also enables UART, if UART was enabled before suspend.
		 */
		io_write32(uart_base + UART_REG_CR1, pd->cr1);
		res = TEE_SUCCESS;
		break;
	default:
		break;
	}

	return res;
}
DECLARE_KEEP_PAGER_PM(stm32_uart_pm);

static TEE_Result stm32_uart_probe(const void *fdt, int node,
				   const void *compt_data __unused)
{
	struct stm32_uart_pdata *pd = NULL;
	static char *params;
	TEE_Result res = TEE_SUCCESS;

	pd = calloc(1, sizeof(*pd));
	if (!pd)
		return TEE_ERROR_OUT_OF_MEMORY;

	res = parse_dt((void *)fdt, node, pd);
	if (res)
		goto out;

	setup_resources(pd);

	if (uart_is_for_console((void *)fdt, node, &params) &&
	    stm32_uart_update_baudrate(pd, params) == TEE_SUCCESS) {
		vaddr_t uart_base = io_pa_or_va(&pd->base, 1);

		register_serial_console(&pd->chip);

		pd->is_console = true;
		pd->brr = io_read32(uart_base + UART_REG_BRR);
		pd->presc = io_read32(uart_base + UART_REG_PRESC);
		pd->cr1 = io_read32(uart_base + UART_REG_CR1);
		pd->cr2 = io_read32(uart_base + UART_REG_CR2);
		pd->cr3 = io_read32(uart_base + UART_REG_CR3);
	}

	/* This clock ops is required for PM */
	assert(pd->clock->ops->is_enabled);
	register_pm_core_service_cb(stm32_uart_pm, pd, "stm32-uart");

out:
	if (res)
		free(pd);

	return res;
}

static const struct dt_device_match stm32_uart_match_table[] = {
	{ .compatible = "st,stm32h7-uart" },
	{ }
};

DEFINE_DT_DRIVER(stm32_uart_dt_driver) = {
	.name = "stm32_uart",
	.type = DT_DRIVER_UART,
	.match_table = stm32_uart_match_table,
	.probe = &stm32_uart_probe,
};
