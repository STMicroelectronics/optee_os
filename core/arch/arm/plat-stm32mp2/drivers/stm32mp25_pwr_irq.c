// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2023-2024, STMicroelectronics
 */

#include <config.h>
#include <drivers/gpio.h>
#include <drivers/stm32_gpio.h>
#include <drivers/stm32mp25_pwr.h>
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

#define PWR_IRQ_MAX_PROP_LENGTH	U(18)

#define PWR_NB_WAKEUPPINS	U(6)

/* PWR Registers */
#define PWR_WKUPCR1		0x60
/* WKUPCR bit definition */
#define WKUPC			BIT(0)
#define WKUPP			BIT(8)
#define WKUPPUPD		GENMASK_32(13, 12)
#define WKUPENCPU1		BIT(16)
#define WKUPENCPU2		BIT(17)
#define WKUPF			BIT(31)

#define WKUPPUPD_MASK		GENMASK_32(1, 0)
#define WKUPPUPD_SHIFT		U(12)

enum wkup_pull_setting {
	WKUP_NO_PULL = 0,
	WKUP_PULL_UP,
	WKUP_PULL_DOWN,
	WKUP_PULL_RESERVED
};

struct pwr_wkup_data {
	struct itr_chip *pwr_irq_chip;
	struct itr_handler *parent_hdl;
	unsigned int spinlock;
	uint8_t itr_enable_bitmask;
	uint8_t itr_mask_bitmask;
};

static struct pwr_wkup_data *pwr_wkup_d;

static inline vaddr_t pwr_wkupcr_base(uint32_t pin)
{
	return stm32_pwr_base() + PWR_WKUPCR1 + pin * 4;
}

/* WAKEUP pins */
#define GPIO_BANK(port)	((port) - 'A')
#define GPIO_PORT(bank)	((bank) + 'A')

struct stm32_pwr_pin_map {
	uint8_t bank;
	uint8_t pin;
};

static const struct stm32_pwr_pin_map pin_map[PWR_NB_WAKEUPPINS] = {
	{ .bank = GPIO_BANK('A'), .pin = 0, },
	{ .bank = GPIO_BANK('H'), .pin = 5, },
	{ .bank = GPIO_BANK('G'), .pin = 1, },
	{ .bank = GPIO_BANK('I'), .pin = 6, },
	{ .bank = GPIO_BANK('G'), .pin = 2, },
	{ .bank = GPIO_BANK('G'), .pin = 3, },
};

static enum itr_return pwr_it_handler(struct itr_handler *handler)
{
	struct pwr_wkup_data *priv = handler->data;
	bool handled = false;
	unsigned int i = 0;

	interrupt_mask(priv->parent_hdl->chip, priv->parent_hdl->it);

	for (i = 0; i < PWR_NB_WAKEUPPINS; i++) {
		vaddr_t wkupcr_va = pwr_wkupcr_base(i);

		if (io_read32(wkupcr_va) & WKUPF) {
			VERBOSE_PWR("handle wkup irq:%u", i);
			handled = true;

			/* Ack the interrupt */
			io_setbits32(wkupcr_va, WKUPC);

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
	struct pwr_wkup_data *priv = pwr_wkup_d;
	uint32_t exceptions = 0;

	VERBOSE_PWR("irq:%zu pull config:%#x", it, config);

	if (config >= WKUP_PULL_RESERVED) {
		EMSG("bad irq pull config");
		return TEE_ERROR_GENERIC;
	}

	exceptions = cpu_spin_lock_xsave(&priv->spinlock);

	io_mask32(pwr_wkupcr_base(it),
		  (config & WKUPPUPD_MASK) << (WKUPPUPD_SHIFT),
		  (WKUPPUPD_MASK) << (WKUPPUPD_SHIFT));

	cpu_spin_unlock_xrestore(&priv->spinlock, exceptions);

	return TEE_SUCCESS;
}

static void stm32mp25_pwr_itr_enable_nolock(size_t it)
{
	VERBOSE_PWR("Pwr irq enable %zu", it);

	/* Clear flag before enable to avoid false interrupt */
	io_setbits32(pwr_wkupcr_base(it), WKUPC);
	io_setbits32(pwr_wkupcr_base(it), WKUPENCPU1);
}

static void stm32mp25_pwr_itr_disable_nolock(size_t it)
{
	VERBOSE_PWR("Pwr irq disable %zu", it);

	io_clrbits32(pwr_wkupcr_base(it), WKUPENCPU1);
}

static TEE_Result stm32_pwr_irq_set_trig(size_t it, unsigned int flags)
{
	struct pwr_wkup_data *priv = pwr_wkup_d;
	uint32_t exceptions = 0;
	int en = 0;

	VERBOSE_PWR("irq:%zu %s edge", it,
		    (flags & PWR_WKUP_FLAG_FALLING) ? "falling" : "rising");

	exceptions = cpu_spin_lock_xsave(&priv->spinlock);

	en = io_read32(pwr_wkupcr_base(it)) & WKUPENCPU1;
	/*
	 * Reference manual request to disable the wakeup pin while
	 * changing the edge detection setting.
	 */
	if (en)
		stm32mp25_pwr_itr_disable_nolock(it);

	if (flags & PWR_WKUP_FLAG_FALLING)
		io_setbits32(pwr_wkupcr_base(it), WKUPP);
	else
		io_clrbits32(pwr_wkupcr_base(it), WKUPP);

	if (en)
		stm32mp25_pwr_itr_enable_nolock(it);

	cpu_spin_unlock_xrestore(&priv->spinlock, exceptions);

	return TEE_SUCCESS;
}

/* Enable an interrupt */
static void stm32mp25_pwr_op_enable(struct itr_chip *chip __unused, size_t it)
{
	struct pwr_wkup_data *priv = pwr_wkup_d;
	uint32_t exceptions = 0;

	exceptions = cpu_spin_lock_xsave(&priv->spinlock);
	priv->itr_enable_bitmask |= BIT(it);
	stm32mp25_pwr_itr_enable_nolock(it);
	interrupt_enable(priv->parent_hdl->chip, priv->parent_hdl->it);
	cpu_spin_unlock_xrestore(&priv->spinlock, exceptions);
}

/* Disable an interrupt */
static void stm32mp25_pwr_op_disable(struct itr_chip *chip __unused, size_t it)
{
	struct pwr_wkup_data *priv = pwr_wkup_d;
	uint32_t exceptions = 0;

	exceptions = cpu_spin_lock_xsave(&priv->spinlock);
	priv->itr_enable_bitmask &= ~BIT(it);
	stm32mp25_pwr_itr_disable_nolock(it);
	if (!priv->itr_enable_bitmask)
		interrupt_disable(priv->parent_hdl->chip,
				  priv->parent_hdl->it);
	cpu_spin_unlock_xrestore(&priv->spinlock, exceptions);
}

/* Mask an interrupt, may be called from an interrupt context */
static void stm32mp25_pwr_op_mask(struct itr_chip *chip __unused, size_t it)
{
	struct pwr_wkup_data *priv = pwr_wkup_d;
	uint32_t exceptions = 0;

	exceptions = cpu_spin_lock_xsave(&priv->spinlock);
	priv->itr_mask_bitmask |= BIT(it);
	interrupt_mask(priv->parent_hdl->chip, priv->parent_hdl->it);
	cpu_spin_unlock_xrestore(&priv->spinlock, exceptions);
}

/* Unmask an interrupt, may be called from an interrupt context */
static void stm32mp25_pwr_op_unmask(struct itr_chip *chip __unused, size_t it)
{
	struct pwr_wkup_data *priv = pwr_wkup_d;
	uint32_t exceptions = 0;

	exceptions = cpu_spin_lock_xsave(&priv->spinlock);
	priv->itr_mask_bitmask &= ~BIT(it);
	if (!priv->itr_mask_bitmask)
		interrupt_unmask(priv->parent_hdl->chip, priv->parent_hdl->it);
	cpu_spin_unlock_xrestore(&priv->spinlock, exceptions);
}

/* Raise per-cpu interrupt (optional) */
static void stm32mp25_pwr_op_raise_pi(struct itr_chip *chip __unused,
				      size_t it __unused)
{
	struct pwr_wkup_data *priv = pwr_wkup_d;

	/* nothing to do here, only forward to parent */

	if (interrupt_can_raise_pi(priv->parent_hdl->chip))
		interrupt_raise_pi(priv->parent_hdl->chip,
				   priv->parent_hdl->it);
}

/* Raise a SGI (optional) */
static void stm32mp25_pwr_op_raise_sgi(struct itr_chip *chip __unused,
				       size_t it __unused, uint32_t cpu_mask)
{
	struct pwr_wkup_data *priv = pwr_wkup_d;

	/* nothing to do here, only forward to parent */

	if (interrupt_can_raise_sgi(priv->parent_hdl->chip))
		interrupt_raise_sgi(priv->parent_hdl->chip,
				    priv->parent_hdl->it, cpu_mask);
}

/* Set interrupt/cpu affinity (optional) */
static void stm32mp25_pwr_op_set_affinity(struct itr_chip *chip __unused,
					  size_t it __unused, uint8_t cpu_mask)
{
	struct pwr_wkup_data *priv = pwr_wkup_d;

	/* nothing to do here, only forward to parent */

	if (interrupt_can_set_affinity(priv->parent_hdl->chip))
		interrupt_set_affinity(priv->parent_hdl->chip,
				       priv->parent_hdl->it, cpu_mask);
}

/* Enable/disable power-management wake-on of an interrupt (optional) */
static void stm32mp25_pwr_op_set_wake(struct itr_chip *chip __unused,
				      size_t it __unused, bool on)
{
	struct pwr_wkup_data *priv = pwr_wkup_d;

	/*
	 * TODO:
	 * Today, even with clients that call interrupt_set_wake(),
	 * this driver incorrectly set the wakeup in WKUPENCPU1 as part of
	 * enable() and unmask().
	 * This driver must be reworked to separate:
	 * - interrupt handling (simply forward to GIC);
	 * - wakeup-source from client to handle WKUPENCPU1.
	 * Client drivers should be reworked to request the wakeup.
	 * For the moment, only forward to parent.
	 */

	if (interrupt_can_set_wake(priv->parent_hdl->chip))
		interrupt_set_wake(priv->parent_hdl->chip,
				   priv->parent_hdl->it, on);
}

static const struct itr_ops stm32mp2_pwr_itr_ops = {
	.enable		= stm32mp25_pwr_op_enable,
	.disable	= stm32mp25_pwr_op_disable,
	.mask		= stm32mp25_pwr_op_mask,
	.unmask		= stm32mp25_pwr_op_unmask,
	.raise_pi	= stm32mp25_pwr_op_raise_pi,
	.raise_sgi	= stm32mp25_pwr_op_raise_sgi,
	.set_affinity	= stm32mp25_pwr_op_set_affinity,
	.set_wake	= stm32mp25_pwr_op_set_wake,
};
DECLARE_KEEP_PAGER(stm32mp2_pwr_itr_ops);

static struct itr_chip stm32mp2_pwr_itr_chip = {
	.ops = &stm32mp2_pwr_itr_ops,
	.name = "stm32mp2-pwr-irq",
};

static TEE_Result stm32mp2_pwr_itr_dt_get(struct dt_pargs *args,
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

	itr_desc_p->chip = &stm32mp2_pwr_itr_chip;
	itr_desc_p->itr_num = itr_num;

	return TEE_SUCCESS;
}

TEE_Result stm32mp25_pwr_irq_probe(const void *fdt, int node)
{
	TEE_Result res = TEE_ERROR_GENERIC;
	struct pwr_wkup_data *priv = NULL;
	struct itr_chip *itr_chip = NULL;
	size_t itr_num = DT_INFO_INVALID_INTERRUPT;

	VERBOSE_PWR("Init PWR IRQ");

	pwr_wkup_d = calloc(1, sizeof(*pwr_wkup_d));
	if (!pwr_wkup_d)
		return TEE_ERROR_OUT_OF_MEMORY;

	priv = pwr_wkup_d;
	priv->pwr_irq_chip = &stm32mp2_pwr_itr_chip;

	res = itr_chip_dt_only_init(&stm32mp2_pwr_itr_chip);
	if (res)
		panic();

	res = interrupt_dt_get(fdt, node, &itr_chip, &itr_num);
	if (res)
		panic("No interrupt defined in PWR");

	res = interrupt_create_handler(itr_chip, itr_num, pwr_it_handler,
				       pwr_wkup_d, ITRF_TRIGGER_LEVEL,
				       &priv->parent_hdl);
	if (res)
		panic("Could not get wake-up pin IRQ");

	res = interrupt_register_provider(fdt, node, stm32mp2_pwr_itr_dt_get,
					  pwr_wkup_d);
	if (res)
		panic("Can't register provider");

	interrupt_enable(itr_chip, itr_num);

	VERBOSE_PWR("Init pwr irq done");

	return TEE_SUCCESS;
}
