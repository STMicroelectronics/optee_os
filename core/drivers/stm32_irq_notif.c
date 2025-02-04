// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2023-2025, STMicroelectronics
 */

#include <kernel/dt.h>
#include <kernel/dt_driver.h>
#include <kernel/interrupt.h>
#include <kernel/notif.h>
#include <libfdt.h>
#include <malloc.h>
#include <tee_api_defines.h>
#include <tee_api_types.h>
#include <stddef.h>
#include <stdint.h>
#include <trace.h>

#define NOTIF_IT_VALUE_INVALID	(NOTIF_IT_VALUE_MAX + U(1))

struct stm32_irq_notif_data {
	uint32_t it_notif_id;
};

static enum itr_return stm32_irq_notif_handler(struct itr_handler *handler)
{
	struct stm32_irq_notif_data *priv = handler->data;

	FMSG("stm32 irq notifier handler");

	if (priv->it_notif_id != NOTIF_IT_VALUE_INVALID)
		notif_send_it(priv->it_notif_id, 0);

	return ITRR_HANDLED;
}

static TEE_Result stm32_irq_notif_dt_probe(const void *fdt, int node,
					   const void *compat __unused)
{
	struct stm32_irq_notif_data *priv = NULL;
	struct itr_chip *itr_chip = NULL;
	TEE_Result res = TEE_ERROR_GENERIC;
	const fdt32_t *cuint = NULL;
	size_t itr_num = 0;

	res = interrupt_dt_get(fdt, node, &itr_chip, &itr_num);
	if (res)
		return res;

	priv = calloc(1, sizeof(*priv));
	if (!priv)
		return TEE_ERROR_OUT_OF_MEMORY;

	priv->it_notif_id = NOTIF_IT_VALUE_INVALID;

	cuint = fdt_getprop(fdt, node, "st,notif-it-id", NULL);
	if (cuint)
		priv->it_notif_id = fdt32_to_cpu(*cuint);

	res = interrupt_create_handler(itr_chip, itr_num,
				       stm32_irq_notif_handler,
				       priv, 0, NULL);
	if (res) {
		free(priv);
		return res;
	}

	interrupt_enable(itr_chip, itr_num);
	if (fdt_getprop(fdt, node, "wakeup-source", NULL)) {
		if (interrupt_can_set_wake(itr_chip))
			interrupt_set_wake(itr_chip, itr_num, true);
		else
			DMSG("irq notifier wakeup source ignored");
	}

	return TEE_SUCCESS;
}

static const struct dt_device_match stm32_irq_notif_match_table[] = {
	{ .compatible = "st,stm32-irq-notifier" },
	{ }
};

DEFINE_DT_DRIVER(stm32_irq_notif) = {
	.name = "stm32-irq-notifier",
	.match_table = stm32_irq_notif_match_table,
	.probe = stm32_irq_notif_dt_probe,
};
