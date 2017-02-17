/*
 * Copyright (c) 2016, GlobalLogic
 * Copyright (c) 2017, Linaro Limited
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <drivers/scif.h>
#include <io.h>
#include <util.h>

#define SCIF_SCFSR		(0x10)
#define SCIF_SCFTDR		(0x0C)
#define SCIF_SCFCR		(0x18)
#define SCIF_SCFDR		(0x1C)

#define SCFSR_TDFE		BIT(5)
#define SCFSR_TEND		BIT(6)

#define SCFDR_T_SHIFT		8

#define SCIF_TX_FIFO_SIZE	16

static vaddr_t chip_to_base(struct serial_chip *chip)
{
	struct scif_uart_data *pd =
		container_of(chip, struct scif_uart_data, chip);

	return io_pa_or_va(&pd->base);
}

static void scif_uart_flush(struct serial_chip *chip)
{
	vaddr_t base = chip_to_base(chip);

	while (!(read16(base + SCIF_SCFSR) & SCFSR_TEND))
		;
}

static void scif_uart_putc(struct serial_chip *chip, int ch)
{
	vaddr_t base = chip_to_base(chip);

	/* Wait until there is space in the FIFO */
	while ((read16(base + SCIF_SCFDR) >> SCFDR_T_SHIFT) >=
		SCIF_TX_FIFO_SIZE)
		;
	write8(ch, base + SCIF_SCFTDR);
	write16(read16(base + SCIF_SCFSR) & ~(SCFSR_TEND | SCFSR_TDFE),
		base + SCIF_SCFSR);
}

static const struct serial_ops scif_uart_ops = {
	.flush = scif_uart_flush,
	.putc = scif_uart_putc,
};

void scif_uart_init(struct scif_uart_data *pd, vaddr_t base)
{
	pd->base.pa = base;
	pd->chip.ops = &scif_uart_ops;

	/* Bootloader should initialize device for us */
	scif_uart_flush(&pd->chip);
}
