// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2017, Linaro Limited
 */

#include <compiler.h>
#include <console.h>
#include <drivers/cbmem_console.h>
#include <drivers/semihosting_console.h>
#include <drivers/ffa_console.h>
#include <drivers/serial.h>
#include <initcall.h>
#include <kernel/dt.h>
#include <kernel/dt_driver.h>
#include <kernel/panic.h>
#include <libfdt.h>
#include <mm/phys_mem.h>
#include <stdlib.h>
#include <string.h>
#include <string_ext.h>

static struct serial_chip *serial_console __nex_bss;

/* May be overridden by platform */
__weak void plat_console_init(void)
{
}

void console_init(void)
{
	if (IS_ENABLED(CFG_SEMIHOSTING_CONSOLE))
		semihosting_console_init(CFG_SEMIHOSTING_CONSOLE_FILE);
	else if (IS_ENABLED(CFG_FFA_CONSOLE))
		ffa_console_init();
	else
		plat_console_init();
}

#ifdef CFG_RAM_CONSOLE
/*
 * struct ram_console - RAM console to store messages in.
 * ram_serial: Serial chip instance for the RAM console
 * @mm: RAM console buffer allocate in TA RAM
 * @logs: Virtual address of the buffer referenced by @mm
 * @write_offset: Byte offset from @logs where to store next trace byte
 */
struct ram_console {
	struct serial_chip ram_serial;
	tee_mm_entry_t *mm;
	char *logs;
	size_t write_offset;
};

/* RAM console reference used to switch console in matching UART probe */
static struct ram_console *ram_console;

static void ram_console_putc(struct serial_chip *chip __unused, int ch)
{
	if (ram_console->write_offset < CFG_RAM_CONSOLE_SIZE) {
		*(ram_console->logs + ram_console->write_offset) = ch;
		ram_console->write_offset++;
	}
}

static void ram_console_flush(struct serial_chip *chip)
{
	size_t n = 0;

	if (ram_console && ram_console->write_offset) {
		if (chip->ops->putc)
			for (n = 0; n < ram_console->write_offset; n++)
				chip->ops->putc(chip, ram_console->logs[n]);

		if (chip->ops->flush)
			chip->ops->flush(chip);
		ram_console->write_offset = 0;
	}
}

static const struct serial_ops ram_console_ops = {
	.putc = ram_console_putc,
};
DECLARE_KEEP_PAGER(ram_console_ops);

void ram_console_init(void)
{
	ram_console = calloc(1, sizeof(*ram_console));
	if (!ram_console)
		return;

	ram_console->mm = phys_mem_ta_alloc(CFG_RAM_CONSOLE_SIZE);
	if (!ram_console->mm) {
		free(ram_console);
		ram_console = NULL;
		return;
	}
	ram_console->logs = phys_to_virt(tee_mm_get_smem(ram_console->mm),
					 MEM_AREA_TA_RAM, CFG_RAM_CONSOLE_SIZE);

	ram_console->ram_serial.ops = &ram_console_ops;
	serial_console = &ram_console->ram_serial;

	DMSG("RAM console registered");
}

static void release_ram_console(void)
{
	if (ram_console) {
		tee_mm_free(ram_console->mm);
		free(ram_console);
		ram_console = NULL;
		DMSG("RAM console released");
	}
}
#endif /*CFG_RAM_CONSOLE*/

void __weak console_putc(int ch)
{
	if (!serial_console)
		return;

	if (ch == '\n')
		serial_console->ops->putc(serial_console, '\r');
	serial_console->ops->putc(serial_console, ch);
}

void __weak console_flush(void)
{
	if (!serial_console || !serial_console->ops->flush)
		return;

	serial_console->ops->flush(serial_console);
}

void register_serial_console(struct serial_chip *chip)
{
#ifdef CFG_RAM_CONSOLE
	if (ram_console && chip)
		ram_console_flush(chip);
#endif /* CFG_RAM_CONSOLE */
	serial_console = chip;
#ifdef CFG_RAM_CONSOLE
	release_ram_console();
#endif /* CFG_RAM_CONSOLE */
}

#ifdef CFG_CONSOLE_RUNTIME_SET
static TEE_Result console_runtime_set(void)
{
	if (CFG_CONSOLE_RUNTIME_LOG_LEVEL == TRACE_MIN)
		IMSG("Disabling output console");

	trace_set_level(CFG_CONSOLE_RUNTIME_LOG_LEVEL);

	return TEE_SUCCESS;
}

boot_final(console_runtime_set);
#endif

#ifdef CFG_DT
static int find_chosen_node(void *fdt)
{
	int offset = 0;

	if (!fdt)
		return -1;

	offset = fdt_path_offset(fdt, "/secure-chosen");

	if (offset < 0)
		offset = fdt_path_offset(fdt, "/chosen");

	return offset;
}

TEE_Result get_console_node_from_dt(void *fdt, int *offs_out,
				    char **path_out, char **params_out)
{
	const struct fdt_property *prop;
	const char *uart;
	const char *parms = NULL;
	int offs;
	char *stdout_data;
	char *p;
	TEE_Result rc = TEE_ERROR_GENERIC;

	/* Probe console from secure DT and fallback to non-secure DT */
	offs = find_chosen_node(fdt);
	if (offs < 0) {
		DMSG("No console directive from DTB");
		return TEE_ERROR_ITEM_NOT_FOUND;
	}

	prop = fdt_get_property(fdt, offs, "stdout-path", NULL);
	if (!prop) {
		/*
		 * A secure-chosen or chosen node is present but defined
		 * no stdout-path property: no console expected
		 */
		IMSG("Switching off console");
		register_serial_console(NULL);
		return TEE_ERROR_ITEM_NOT_FOUND;
	}

	stdout_data = nex_strdup(prop->data);
	if (!stdout_data)
		panic();
	p = strchr(stdout_data, ':');
	if (p) {
		*p = '\0';
		parms = p + 1;
	}

	/* stdout-path may refer to an alias */
	uart = fdt_get_alias(fdt, stdout_data);
	if (!uart) {
		/* Not an alias, assume we have a node path */
		uart = stdout_data;
	}
	offs = fdt_path_offset(fdt, uart);
	if (offs >= 0) {
		if (offs_out)
			*offs_out = offs;
		if (params_out)
			*params_out = parms ? nex_strdup(parms) : NULL;
		if (path_out)
			*path_out = uart ? nex_strdup(uart) : NULL;

		rc = TEE_SUCCESS;
	}

	nex_free(stdout_data);

	return rc;
}

void configure_console_from_dt(void)
{
	const struct dt_driver *dt_drv;
	const struct serial_driver *sdrv;
	struct serial_chip *dev;
	char *uart = NULL;
	char *parms = NULL;
	void *fdt;
	int offs;

	fdt = get_dt();

	if (IS_ENABLED(CFG_CBMEM_CONSOLE) && cbmem_console_init_from_dt(fdt))
		return;

	if (get_console_node_from_dt(fdt, &offs, &uart, &parms))
		return;

	dt_drv = dt_find_compatible_driver(fdt, offs);
	if (!dt_drv || dt_drv->type != DT_DRIVER_UART)
		goto out;

	sdrv = (const struct serial_driver *)dt_drv->driver;
	if (!sdrv)
		goto out;

	dev = sdrv->dev_alloc();
	if (!dev)
		goto out;

	/*
	 * If the console is the same as the early console, dev_init() might
	 * clear pending data. Flush to avoid that.
	 */
	console_flush();
	if (sdrv->dev_init(dev, fdt, offs, parms) < 0) {
		sdrv->dev_free(dev);
		goto out;
	}

	IMSG("Switching console to device: %s", uart);
	register_serial_console(dev);
out:
	nex_free(uart);
	nex_free(parms);
}

#endif /* CFG_DT */
