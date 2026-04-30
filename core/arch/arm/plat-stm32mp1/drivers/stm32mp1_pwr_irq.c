// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2020-2024, STMicroelectronics
 */

#include <config.h>
#include <drivers/gpio.h>
#include <drivers/regulator.h>
#include <drivers/stm32_gpio.h>
#include <drivers/stm32mp1_pwr.h>
#include <dt-bindings/interrupt-controller/irq.h>
#include <initcall.h>
#include <io.h>
#include <kernel/dt.h>
#include <kernel/dt_driver.h>
#include <kernel/interrupt.h>
#include <kernel/notif.h>
#include <kernel/panic.h>
#include <kernel/spinlock.h>
#include <libfdt.h>
#include <platform_config.h>
#include <stdio.h>
#include <stm32_util.h>
#include <sys/queue.h>

#define VERBOSE_PWR FMSG

#define PWR_NB_WAKEUPPINS	U(6)

/* PWR Registers */
#define WKUPCR			0x20
#define WKUPFR			0x24
#define MPUWKUPENR		0x28

/* WKUPCR bits definition */
#define WKUP_EDGE_SHIFT		8
#define WKUP_PULL_SHIFT		16
#define WKUP_PULL_MASK		GENMASK_32(1, 0)

enum wkup_pull_setting {
	WKUP_NO_PULL = 0,
	WKUP_PULL_UP,
	WKUP_PULL_DOWN,
	WKUP_PULL_RESERVED
};

struct stm32_pwr_data {
	vaddr_t base;
	struct itr_chip *pwr_irq_chip;
	struct itr_handler *parent_hdl;
	unsigned int spinlock;
	uint8_t itr_enable_bitmask;
	uint8_t itr_mask_bitmask;
};

/* WAKEUP pins */
#define GPIO_BANK(port)	 ((port) - 'A')
#define GPIO_PORT(bank)	 ((bank) + 'A')

struct stm32_pwr_pin_map {
	uint8_t bank;
	uint8_t pin;
};

#ifdef CFG_STM32MP13
static const struct stm32_pwr_pin_map pin_map[PWR_NB_WAKEUPPINS] = {
	{ .bank = GPIO_BANK('F'), .pin = 8, },
	{ .bank = GPIO_BANK('I'), .pin = 3, },
	{ .bank = GPIO_BANK('C'), .pin = 13, },
	{ .bank = GPIO_BANK('I'), .pin = 1, },
	{ .bank = GPIO_BANK('I'), .pin = 2, },
	{ .bank = GPIO_BANK('A'), .pin = 3, },
};
#endif

#ifdef CFG_STM32MP15
static const struct stm32_pwr_pin_map pin_map[PWR_NB_WAKEUPPINS] = {
	{ .bank = GPIO_BANK('A'), .pin = 0, },
	{ .bank = GPIO_BANK('A'), .pin = 2, },
	{ .bank = GPIO_BANK('C'), .pin = 13, },
	{ .bank = GPIO_BANK('I'), .pin = 8, },
	{ .bank = GPIO_BANK('I'), .pin = 11, },
	{ .bank = GPIO_BANK('C'), .pin = 1, },
};
#endif

static struct stm32_pwr_data *pwr_data;

static enum itr_return pwr_it_handler(struct itr_handler *handler)
{
	struct stm32_pwr_data *priv = (struct stm32_pwr_data *)handler->data;
	uint32_t wkupfr = 0;
	bool handled = false;
	unsigned int i = 0;

	interrupt_mask(priv->parent_hdl->chip, priv->parent_hdl->it);

	wkupfr = io_read32(priv->base + WKUPFR);

	for (i = 0; i < PWR_NB_WAKEUPPINS; i++) {
		if (wkupfr & BIT(i)) {
			VERBOSE_PWR("handle wkup irq:%u", i);
			handled = true;

			/* Acknowledge the interrupt */
			io_setbits32(priv->base + WKUPCR, BIT(i));

			if (io_read32(priv->base + MPUWKUPENR) & BIT(i))
				interrupt_call_handlers(priv->pwr_irq_chip, i);
		}
	}

	interrupt_unmask(priv->parent_hdl->chip, priv->parent_hdl->it);

	if (handled)
		return ITRR_HANDLED;

	return ITRR_NONE;
}
DECLARE_KEEP_PAGER(pwr_it_handler);

static TEE_Result
stm32_pwr_irq_set_pull_config(size_t it, enum wkup_pull_setting config)
{
	struct stm32_pwr_data *priv = pwr_data;
	uint32_t exceptions = 0;

	VERBOSE_PWR("irq:%zu pull config:0%#"PRIx32, it, config);

	if (config >= WKUP_PULL_RESERVED) {
		EMSG("bad irq pull config");
		return TEE_ERROR_GENERIC;
	}

	exceptions = cpu_spin_lock_xsave(&priv->spinlock);

	io_mask32(priv->base + WKUPCR,
		  (config & WKUP_PULL_MASK) << (WKUP_PULL_SHIFT + it * 2),
		  (WKUP_PULL_MASK) << (WKUP_PULL_SHIFT + it * 2));

	cpu_spin_unlock_xrestore(&priv->spinlock, exceptions);

	return TEE_SUCCESS;
}

static void stm32mp1_pwr_itr_enable_nolock(size_t it)
{
	struct stm32_pwr_data *priv = pwr_data;

	VERBOSE_PWR("Pwr irq enable");

	/* Clear flag before enable to avoid false interrupt */
	io_setbits32(priv->base + WKUPCR, BIT(it));
	io_setbits32(priv->base + MPUWKUPENR, BIT(it));
}

static void stm32mp1_pwr_itr_disable_nolock(size_t it)
{
	struct stm32_pwr_data *priv = pwr_data;

	VERBOSE_PWR("Pwr irq disable");

	io_clrbits32(priv->base + MPUWKUPENR, BIT(it));
}

static TEE_Result stm32_pwr_irq_set_trig(size_t it, unsigned int flags)
{
	struct stm32_pwr_data *priv = pwr_data;
	uint32_t exceptions = 0;
	int en = 0;

	VERBOSE_PWR("irq:%zu %s edge", it,
		    flags & PWR_WKUP_FLAG_FALLING ? "falling" : "rising");

	exceptions = cpu_spin_lock_xsave(&priv->spinlock);

	en = io_read32(priv->base + MPUWKUPENR) & BIT(it);
	/*
	 * Reference manual request to disable the wakeup pin while
	 * changing the edge detection setting.
	 */
	if (en)
		stm32mp1_pwr_itr_disable_nolock(it);

	if (flags & PWR_WKUP_FLAG_FALLING)
		io_setbits32(priv->base + WKUPCR, BIT(WKUP_EDGE_SHIFT + it));
	else
		io_clrbits32(priv->base + WKUPCR, BIT(WKUP_EDGE_SHIFT + it));

	if (en)
		stm32mp1_pwr_itr_enable_nolock(it);

	cpu_spin_unlock_xrestore(&priv->spinlock, exceptions);

	return TEE_SUCCESS;
}

/* Enable an interrupt */
static void stm32mp1_pwr_op_enable(struct itr_chip *chip __unused, size_t it)
{
	struct stm32_pwr_data *priv = pwr_data;
	uint32_t exceptions = 0;

	exceptions = cpu_spin_lock_xsave(&priv->spinlock);
	priv->itr_enable_bitmask |= BIT(it);
	stm32mp1_pwr_itr_enable_nolock(it);
	interrupt_enable(priv->parent_hdl->chip, priv->parent_hdl->it);
	cpu_spin_unlock_xrestore(&priv->spinlock, exceptions);
}

/* Disable an interrupt */
static void stm32mp1_pwr_op_disable(struct itr_chip *chip __unused, size_t it)
{
	struct stm32_pwr_data *priv = pwr_data;
	uint32_t exceptions = 0;

	exceptions = cpu_spin_lock_xsave(&priv->spinlock);
	priv->itr_enable_bitmask &= ~BIT(it);
	stm32mp1_pwr_itr_disable_nolock(it);
	if (!priv->itr_enable_bitmask)
		interrupt_disable(priv->parent_hdl->chip,
				  priv->parent_hdl->it);
	cpu_spin_unlock_xrestore(&priv->spinlock, exceptions);
}

/* Mask an interrupt, may be called from an interrupt context */
static void stm32mp1_pwr_op_mask(struct itr_chip *chip __unused, size_t it)
{
	struct stm32_pwr_data *priv = pwr_data;
	uint32_t exceptions = 0;

	exceptions = cpu_spin_lock_xsave(&priv->spinlock);
	priv->itr_mask_bitmask |= BIT(it);
	interrupt_mask(priv->parent_hdl->chip, priv->parent_hdl->it);
	cpu_spin_unlock_xrestore(&priv->spinlock, exceptions);
}

/* Unmask an interrupt, may be called from an interrupt context */
static void stm32mp1_pwr_op_unmask(struct itr_chip *chip __unused, size_t it)
{
	struct stm32_pwr_data *priv = pwr_data;
	uint32_t exceptions = 0;

	exceptions = cpu_spin_lock_xsave(&priv->spinlock);
	priv->itr_mask_bitmask &= ~BIT(it);
	if (!priv->itr_mask_bitmask)
		interrupt_unmask(priv->parent_hdl->chip, priv->parent_hdl->it);
	cpu_spin_unlock_xrestore(&priv->spinlock, exceptions);
}

/* Raise per-cpu interrupt (optional) */
static void stm32mp1_pwr_op_raise_pi(struct itr_chip *chip __unused,
				     size_t it __unused)
{
	struct stm32_pwr_data *priv = pwr_data;

	/* nothing to do here, only forward to parent */

	if (interrupt_can_raise_pi(priv->parent_hdl->chip))
		interrupt_raise_pi(priv->parent_hdl->chip,
				   priv->parent_hdl->it);
}

/* Raise a SGI (optional) */
static void stm32mp1_pwr_op_raise_sgi(struct itr_chip *chip __unused,
				      size_t it __unused, uint32_t cpu_mask)
{
	struct stm32_pwr_data *priv = pwr_data;

	/* nothing to do here, only forward to parent */

	if (interrupt_can_raise_sgi(priv->parent_hdl->chip))
		interrupt_raise_sgi(priv->parent_hdl->chip,
				    priv->parent_hdl->it, cpu_mask);
}

/* Set interrupt/cpu affinity (optional) */
static void stm32mp1_pwr_op_set_affinity(struct itr_chip *chip __unused,
					 size_t it __unused, uint8_t cpu_mask)
{
	struct stm32_pwr_data *priv = pwr_data;

	/* nothing to do here, only forward to parent */

	if (interrupt_can_set_affinity(priv->parent_hdl->chip))
		interrupt_set_affinity(priv->parent_hdl->chip,
				       priv->parent_hdl->it, cpu_mask);
}

/* Enable/disable power-management wake-on of an interrupt (optional) */
static void stm32mp1_pwr_op_set_wake(struct itr_chip *chip __unused,
				     size_t it __unused, bool on)
{
	struct stm32_pwr_data *priv = pwr_data;

	/*
	 * TODO:
	 * Today, even with clients that call interrupt_set_wake(),
	 * this driver incorrectly set the wakeup in MPUWKUPENR as part of
	 * enable() and unmask().
	 * This driver must be reworked to separate:
	 * - interrupt handling (simply forward to GIC);
	 * - wakeup-source from client to handle MPUWKUPENR.
	 * Client drivers should be reworked to request the wakeup.
	 * For the moment, only forward to parent.
	 */

	if (interrupt_can_set_wake(priv->parent_hdl->chip))
		interrupt_set_wake(priv->parent_hdl->chip,
				   priv->parent_hdl->it, on);
}

static const struct itr_ops stm32mp1_pwr_itr_ops = {
	.enable		= stm32mp1_pwr_op_enable,
	.disable	= stm32mp1_pwr_op_disable,
	.mask		= stm32mp1_pwr_op_mask,
	.unmask		= stm32mp1_pwr_op_unmask,
	.raise_pi	= stm32mp1_pwr_op_raise_pi,
	.raise_sgi	= stm32mp1_pwr_op_raise_sgi,
	.set_affinity	= stm32mp1_pwr_op_set_affinity,
	.set_wake	= stm32mp1_pwr_op_set_wake,
};
DECLARE_KEEP_PAGER(stm32mp1_pwr_itr_ops);

static struct itr_chip stm32mp1_pwr_itr_chip = {
	.ops = &stm32mp1_pwr_itr_ops,
	.name = "stm32mp1-pwr-irq",
};

static TEE_Result stm32mp1_pwr_itr_dt_get(struct dt_pargs *args,
					  void *priv_data __unused,
					  struct itr_desc *itr_desc_p)
{
	TEE_Result res = TEE_ERROR_GENERIC;
	unsigned int trigger_mode = 0;
	unsigned int itr_num = 0;
	struct gpio *gpio = NULL;
	unsigned int bank = 0;
	unsigned int pin = 0;

	VERBOSE_PWR("Pwr IRQ add");

	assert(args->args_count == 2);

	itr_num = args->args[0];
	trigger_mode = args->args[1];

	assert(itr_num < PWR_NB_WAKEUPPINS);

	switch (trigger_mode) {
	case IRQ_TYPE_EDGE_RISING:
		res = stm32_pwr_irq_set_trig(itr_num, PWR_WKUP_FLAG_RISING);
		break;
	case IRQ_TYPE_EDGE_FALLING:
		res = stm32_pwr_irq_set_trig(itr_num, PWR_WKUP_FLAG_FALLING);
		break;
	default:
		EMSG("Unsupported flags %#x", trigger_mode);
		res = TEE_ERROR_BAD_PARAMETERS;
		break;
	}
	if (res)
		return res;

	res = gpio_dt_cfg_by_index(args->fdt, args->phandle_node, itr_num,
				   "wakeup", GPIO_IN, &gpio);
	if (res)
		return res;

	bank = stm32_gpio_chip_bank_id(gpio->chip);
	pin = gpio->pin;
	if (bank != pin_map[itr_num].bank || pin != pin_map[itr_num].pin) {
		EMSG("Invalid PWR WKUP%d on GPIO%c%"PRIu8" expected GPIO%c%"
		     PRIu8, itr_num + 1,
		     GPIO_PORT(bank), pin, GPIO_PORT(pin_map[itr_num].bank),
		     pin_map[itr_num].pin);
		panic();
	}

	/* Use the same pull up configuration than for the gpio */
	if (gpio->dt_flags & GPIO_PULL_UP)
		res = stm32_pwr_irq_set_pull_config(itr_num, WKUP_PULL_UP);
	else if (gpio->dt_flags & GPIO_PULL_DOWN)
		res = stm32_pwr_irq_set_pull_config(itr_num, WKUP_PULL_DOWN);
	else
		res = stm32_pwr_irq_set_pull_config(itr_num, WKUP_NO_PULL);
	if (res) {
		gpio_put(gpio);
		return res;
	}

	itr_desc_p->chip = &stm32mp1_pwr_itr_chip;
	itr_desc_p->itr_num = itr_num;

	return TEE_SUCCESS;
}

static TEE_Result stm32mp1_pwr_irq_probe(const void *fdt, int node,
					 const void *compat_data __unused)
{
	TEE_Result res = TEE_ERROR_GENERIC;
	struct stm32_pwr_data *priv = NULL;
	struct itr_chip *itr_chip = NULL;
	size_t itr_num = DT_INFO_INVALID_INTERRUPT;

	VERBOSE_PWR("Init PWR IRQ");

	pwr_data = calloc(1, sizeof(*pwr_data));
	if (!pwr_data)
		return TEE_ERROR_OUT_OF_MEMORY;

	priv = pwr_data;
	priv->base = stm32_pwr_base();
	priv->pwr_irq_chip = &stm32mp1_pwr_itr_chip;

	res = itr_chip_dt_only_init(&stm32mp1_pwr_itr_chip);
	if (res)
		panic();

	res = interrupt_dt_get(fdt, node, &itr_chip, &itr_num);
	if (res)
		goto err;

	res = interrupt_create_handler(itr_chip, itr_num, pwr_it_handler,
				       pwr_data, ITRF_TRIGGER_LEVEL,
				       &priv->parent_hdl);
	if (res)
		panic("Could not get wake-up pin IRQ");

	res = interrupt_register_provider(fdt, node, stm32mp1_pwr_itr_dt_get,
					  pwr_data);
	if (res)
		panic("Can't register provider");

	/* Default clear and disable all wakeup interrupts */
	io_setbits32(priv->base + WKUPCR,
		     GENMASK_32(PWR_NB_WAKEUPPINS - 1, 0));
	io_clrbits32(priv->base + MPUWKUPENR,
		     GENMASK_32(PWR_NB_WAKEUPPINS - 1, 0));

	interrupt_enable(itr_chip, itr_num);

	VERBOSE_PWR("Init pwr irq done");

	return TEE_SUCCESS;
err:
	free(pwr_data);
	pwr_data = NULL;

	return res;
}

static const struct dt_device_match pwr_irq_match_table[] = {
	{ .compatible = "st,stm32mp1-pwr-irq" },
	{ }
};

DEFINE_DT_DRIVER(stm32mp1_pwr_irq_dt_driver) = {
	.name = "stm32mp1-pwr-irq",
	.match_table = pwr_irq_match_table,
	.probe = &stm32mp1_pwr_irq_probe,
};
