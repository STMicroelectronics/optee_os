// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2022-2024, STMicroelectronics
 */

#include <config.h>
#include <drivers/regulator.h>
#include <drivers/stm32_i2c.h>
#include <drivers/stpmic2.h>
#include <keep.h>
#include <kernel/boot.h>
#include <kernel/dt.h>
#include <kernel/interrupt.h>
#include <kernel/notif.h>
#include <kernel/panic.h>
#include <kernel/pm.h>
#include <libfdt.h>
#include <platform_config.h>
#include <stdbool.h>
#include <stm32_util.h>
#include <stm32mp_pm.h>
#include <trace.h>
#include <util.h>

/* I2C transfer trial configuration */
#define PMIC_I2C_TRIALS			U(1)
#define PMIC_I2C_TIMEOUT_BUSY_MS	U(5)

static bool stm32_pmic2;

#define STPMIC2_LP_STATE_OFF		BIT(0)
#define STPMIC2_LP_STATE_ON		BIT(1)
#define STPMIC2_LP_STATE_UNMODIFIED	BIT(2)
#define STPMIC2_LP_STATE_SET_VOLT	BIT(3)
#define STPMIC2_LP_STATE_INVALID	-1


/*
 * struct pmic_regu - STPMIC2 regulator instance
 *
 * @pmic: Handle to PMIC device
 * @id: ID in PMIC device of the regulator controllers
 * @bypass_uv: 0 if not used, else is the voltage level to switch to bypass mode
 * @lp_state: Regulator mode during PM low power state, per PM states
 * @lp_level_uv: Regulator voltage level during PM low power state, per PM state
 * @last_lp_mode: Last low power mode selected
 * @levels_desc: Description of the supported voltage levels
 * @levels: Voltage level value array related to description @levels_desc
 * @power_control_en: PWRCTRL input line is enabled
 * @forced_off: Regulator was forced off at suspend
 */
struct pmic_regu {
	struct stpmic2 *pmic;
	uint32_t id;
	int bypass_uv;
	uint8_t *lp_state;
	int *lp_level_uv;
	int last_lp_mode;
	struct regulator_voltages_desc levels_desc;
	int *levels;
	bool power_control_en;
	bool forced_off;
};

/*
 * struct pmic_compat_data - pmic specificity helper
 * @desc_table: description table of each regulator
 * @desc_len: len of the description table
 * @ref_id: value of PMIC_REF_ID in PRODUCT_ID_SR
 */
struct pmic_compat_data {
	const struct regu_dt_desc *desc_table;
	size_t desc_len;
	uint8_t ref_id;
};

/*
 * struct regu_dt_property - DT property helper
 * @name: DT property string name
 * @prop: Property decimal identifier
 */
struct regu_dt_property {
	const char *name;
	enum stpmic2_prop_id prop;
};

static const struct regu_dt_property prop_table[] = {
	{
		.name = "st,mask-reset",
		.prop = STPMIC2_MASK_RESET,
	},
	{
		.name = "st,pwrctrl-enable",
		.prop = STPMIC2_PWRCTRL_EN,
	},
	{
		.name = "st,pwrctrl-reset",
		.prop = STPMIC2_PWRCTRL_RS,
	},
	{
		.name = "st,pwrctrl-sel",
		.prop = STPMIC2_PWRCTRL_SEL,
	},
	{
		.name = "st,alternate-input-source",
		.prop = STPMIC2_ALTERNATE_INPUT_SOURCE,
	},
};

static void lock_unlock(bool lock_not_unlock)
{
	static struct mutex pmic_mu = MUTEX_INITIALIZER;

	if (thread_get_id_may_fail() != THREAD_ID_INVALID) {
		if (lock_not_unlock)
			mutex_lock(&pmic_mu);
		else
			mutex_unlock(&pmic_mu);
	}
}

static void lock_pmic_access(void)
{
	lock_unlock(true);
}

static void unlock_pmic_access(void)
{
	lock_unlock(false);
}

/*
 * Regulator operation handler
 */
static TEE_Result pmic_set_state(struct regulator *regulator, bool enable)
{
	struct pmic_regu *regu = regulator->priv;
	TEE_Result res = TEE_ERROR_GENERIC;

	FMSG("%s: set state to %u", regulator_name(regulator), enable);

	lock_pmic_access();
	res = stpmic2_regulator_set_state(regu->pmic, regu->id, enable);
	unlock_pmic_access();

	return res;
}

static TEE_Result pmic_get_state(struct regulator *regulator, bool *enabled)
{
	struct pmic_regu *regu = regulator->priv;
	TEE_Result res = TEE_ERROR_GENERIC;

	FMSG("%s: get state", regulator_name(regulator));

	lock_pmic_access();
	res = stpmic2_regulator_get_state(regu->pmic, regu->id, enabled);
	unlock_pmic_access();

	return res;
}

static TEE_Result pmic_get_voltage(struct regulator *regulator, int *level_uv)
{
	struct pmic_regu *regu = regulator->priv;
	TEE_Result res = TEE_ERROR_GENERIC;
	uint16_t val = 0;

	lock_pmic_access();

	if (regu->bypass_uv) {
		uint8_t arg = 0;

		/* If the regul is in bypass mode, return bypass value */
		res = stpmic2_regulator_get_prop(regu->pmic, regu->id,
						 STPMIC2_BYPASS, &arg);
		if (res)
			goto out;

		if (arg == PROP_BYPASS_SET) {
			*level_uv = regu->bypass_uv;
			goto out;
		}
	}

	res = stpmic2_regulator_get_voltage(regu->pmic, regu->id, &val);
	if (res)
		goto out;

	*level_uv = (int)val * 1000;

	FMSG("%s: get voltage: %d uV", regulator_name(regulator), *level_uv);
out:
	unlock_pmic_access();

	return res;
}

static TEE_Result pmic_set_voltage(struct regulator *regulator, int level_uv)
{
	struct pmic_regu *regu = regulator->priv;
	TEE_Result res = TEE_ERROR_GENERIC;

	FMSG("%s: set volt to %d mV", regulator_name(regulator), level_uv);

	lock_pmic_access();

	if (level_uv == regu->bypass_uv) {
		res = stpmic2_regulator_set_prop(regu->pmic, regu->id,
						 STPMIC2_BYPASS,
						 PROP_BYPASS_SET);
		/* Do not set voltage in the register */
		goto out;
	}

	res = stpmic2_regulator_set_voltage(regu->pmic, regu->id,
					    level_uv / 1000);
	if (res)
		goto out;

	/* disable bypass after set voltage ; wait settling time ? */
	if (regu->bypass_uv && level_uv != regu->bypass_uv)
		res = stpmic2_regulator_set_prop(regu->pmic, regu->id,
						 STPMIC2_BYPASS,
						 PROP_BYPASS_RESET);
out:
	unlock_pmic_access();

	return res;
}

static size_t refine_levels_array(size_t count, int *levels_uv,
				  int min_uv, int max_uv)
{
	size_t n = 0;
	size_t m = 0;

	qsort_int(levels_uv, count);

	/* Remove duplicates and return optimized count */
	for (n = 1; n < count; n++) {
		if (levels_uv[m] != levels_uv[n]) {
			if (m + 1 != n)
				levels_uv[m + 1] = levels_uv[n];
			m++;
		}
	}
	count = m + 1;

	/* Find max voltage index */
	for (n = count; n; n--)
		if (levels_uv[n - 1] <= max_uv)
			break;
	count = n;

	for (n = 0; n < count; n++)
		if (levels_uv[n] >= min_uv)
			break;
	count -= n;

	memmove(levels_uv, levels_uv + n, count * sizeof(*levels_uv));

	return count;
}

static TEE_Result pmic_list_voltages(struct regulator *regulator,
				     struct regulator_voltages_desc **out_desc,
				     const int **out_levels)
{
	struct pmic_regu *regu = regulator->priv;

	FMSG("%s: list volt", regulator_name(regulator));

	if (!regu->levels) {
		TEE_Result res = TEE_ERROR_GENERIC;
		const uint16_t *level_ref = NULL;
		size_t level_count = 0;
		size_t count_ref = 0;
		int *levels2 = NULL;
		int *levels = NULL;
		size_t n = 0;

		/*
		 * Allocate and build a consise and ordered voltage list
		 * based on the voltage list provided by stpmic2 driver.
		 * Also add bypass voltage to the list.
		 */
		res = stpmic2_regulator_levels_mv(regu->pmic, regu->id,
						  &level_ref, &count_ref);
		if (res)
			return res;

		if (regu->bypass_uv)
			level_count = count_ref + 1;
		else
			level_count = count_ref;

		levels = calloc(level_count, sizeof(*levels));
		if (!levels)
			return TEE_ERROR_OUT_OF_MEMORY;

		for (n = 0; n < count_ref; n++)
			levels[n] = level_ref[n] * 1000;

		if (regu->bypass_uv)
			levels[n] = regu->bypass_uv;

		level_count = refine_levels_array(level_count, levels,
						  regulator->min_uv,
						  regulator->max_uv);

		/* Shrink levels array to not waste heap memory */
		levels2 = realloc(levels, sizeof(*levels) * level_count);
		if (!levels2) {
			free(levels);
			return TEE_ERROR_OUT_OF_MEMORY;
		}

		regu->levels_desc.type = VOLTAGE_TYPE_FULL_LIST;
		regu->levels_desc.num_levels = level_count;
		regu->levels = levels2;
	}

	*out_desc = &regu->levels_desc;
	*out_levels = regu->levels;

	return TEE_SUCCESS;
}

TEE_Result stm32_pmic2_apply_pm_state(struct regulator *regulator, uint8_t mode)
{
	struct pmic_regu *regu = regulator->priv;
	uint8_t state = regu->lp_state[mode];
	int lp_level_uv = regu->lp_level_uv[mode];
	TEE_Result res = TEE_ERROR_GENERIC;

	FMSG("%s: suspend state:%#"PRIx8" %d uV",
	     regulator_name(regulator), state, lp_level_uv);

	/*
	 * If the LP state is controlled by the consumer (power control line
	 * disabled), and the suspend state was requested as OFF in device tree,
	 * then disable the regulator and put a message.
	 */
	if (!regu->power_control_en && (state & STPMIC2_LP_STATE_OFF)) {
		bool enabled;

		res = stpmic2_regulator_get_state(regu->pmic, regu->id,
						  &enabled);
		if (res)
			return res;

		if (enabled) {
			res = stpmic2_regulator_set_state(regu->pmic, regu->id,
							  false);
			if (res)
				return res;

			IMSG("regulator %s forced OFF",
			     regulator_name(regulator));

			regu->forced_off = true;
		} else {
			regu->forced_off = false;
		}

		return TEE_SUCCESS;
	}

	if (mode == regu->last_lp_mode)
		return TEE_SUCCESS;

	if (state & STPMIC2_LP_STATE_OFF) {
		res = stpmic2_lp_set_state(regu->pmic, regu->id, false);
		if (res)
			return res;
	}

	if (state & STPMIC2_LP_STATE_ON) {
		res = stpmic2_lp_set_state(regu->pmic, regu->id, true);
		if (res)
			return res;

		if (state & STPMIC2_LP_STATE_SET_VOLT &&
		    regulator->ops->set_voltage)  {
			res = stpmic2_lp_set_voltage(regu->pmic, regu->id,
						     lp_level_uv / 1000);
			if (res)
				return res;
		}
	}

	regu->last_lp_mode = mode;

	return TEE_SUCCESS;
}

TEE_Result stm32_pmic2_resume_regulator(struct regulator *regulator)
{
	TEE_Result res = TEE_ERROR_GENERIC;
	struct pmic_regu *regu = regulator->priv;

	if (regu->forced_off) {
		/* Re-enable a regulator that was forced off in suspend */
		res = stpmic2_regulator_set_state(regu->pmic, regu->id, true);
		if (res)
			return res;

		regu->forced_off = false;
	}

	return TEE_SUCCESS;
}

static TEE_Result pmic_init(struct regulator *regulator)
{
	/* default configuration for STPMIC regulator */
	regulator->ramp_delay_uv_per_us = U(2200);
	regulator->enable_ramp_delay_us = U(1000);

	return TEE_SUCCESS;
}

static TEE_Result pmic_supplied_init(struct regulator *regulator,
				     const void *fdt, int node)
{
	struct pmic_regu *regu = regulator->priv;
	const struct regu_dt_property *p = NULL;
	TEE_Result res = TEE_ERROR_GENERIC;

	if (regulator->flags & REGULATOR_PULL_DOWN) {
		res = stpmic2_regulator_set_prop(regu->pmic, regu->id,
						 STPMIC2_PULL_DOWN, 0);
		if (res)
			return res;
	}

	if (regulator->flags & REGULATOR_OVER_CURRENT) {
		res = stpmic2_regulator_set_prop(regu->pmic, regu->id,
						 STPMIC2_OCP, 0);
		if (res)
			return res;
	}

	for (p = prop_table; p < (prop_table + ARRAY_SIZE(prop_table)); p++) {
		const fdt32_t *cuint = NULL;
		uint32_t value = 0;

		cuint = fdt_getprop(fdt, node, p->name, NULL);
		if (!cuint)
			continue;

		if (p->prop == STPMIC2_PWRCTRL_EN)
			regu->power_control_en = true;

		value = fdt32_to_cpu(*cuint);
		FMSG("%s: %d, %#"PRIx32, regulator_name(regulator), p->prop,
		     value);

		res = stpmic2_regulator_set_prop(regu->pmic, regu->id,
						 p->prop, value);
		if (res)
			return res;
	}

	res = plat_pmic2_supplied_init(regulator);
	if (res) {
		free(regu);
		return res;
	}

	return TEE_SUCCESS;
}

const struct regulator_ops pmic2_regu_ops = {
	.set_state = pmic_set_state,
	.get_state = pmic_get_state,
	.set_voltage = pmic_set_voltage,
	.get_voltage = pmic_get_voltage,
	.supported_voltages = pmic_list_voltages,
	.init = pmic_init,
	.supplied_init = pmic_supplied_init,
};

const struct regulator_ops pmic2_switch_ops = {
	.set_state = pmic_set_state,
	.get_state = pmic_get_state,
	.init = pmic_init,
	.supplied_init = pmic_supplied_init,
};

static void pmic_parse_regu_node(struct regu_dt_desc *regu_desc,
				 const void *fdt, int node)
{
	struct pmic_regu *regu = regu_desc->priv;
	const fdt32_t *cuint = NULL;
	const char __maybe_unused *regu_name = regu_desc->name;

	cuint = fdt_getprop(fdt, node, "st,regulator-bypass-microvolt", NULL);
	if (!cuint)
		return;

	regu->bypass_uv = fdt32_to_cpu(*cuint);
	FMSG("%s: bypass= %#"PRIx32"uV", regu_name, regu->bypass_uv);
}

static void parse_low_power_mode(const void *fdt, int node,
				 struct regu_dt_desc *regu_desc, int mode)
{
	struct pmic_regu *regu = regu_desc->priv;
	const fdt32_t *cuint = NULL;
	const char __maybe_unused *regu_name = regu_desc->name;

	regu->last_lp_mode = STPMIC2_LP_STATE_INVALID;
	regu->lp_state[mode] = 0;

	if (fdt_getprop(fdt, node, "regulator-off-in-suspend", NULL)) {
		FMSG("%s: mode:%d OFF", regu_name, mode);
		regu->lp_state[mode] |= STPMIC2_LP_STATE_OFF;
	} else if (fdt_getprop(fdt, node, "regulator-on-in-suspend", NULL)) {
		FMSG("%s: mode:%d ON", regu_name, mode);
		regu->lp_state[mode] |= STPMIC2_LP_STATE_ON;
	} else {
		regu->lp_state[mode] |= STPMIC2_LP_STATE_UNMODIFIED;
	}

	cuint = fdt_getprop(fdt, node, "regulator-suspend-microvolt", NULL);
	if (cuint) {
		int level_uv = (int)fdt32_to_cpu(*cuint);

		FMSG("%s: mode:%d suspend to %d uV", regu_name, mode, level_uv);

		regu->lp_state[mode] |= STPMIC2_LP_STATE_SET_VOLT;
		regu->lp_level_uv[mode] = level_uv;
	}
}

static void parse_low_power_modes(const void *fdt, int node,
				  struct regu_dt_desc *regu_desc)
{
	unsigned int mode = 0;

	for (mode = 0; mode < plat_pmic2_get_lp_mode_count(); mode++) {
		const char *lp_mode_name = plat_pmic2_get_lp_mode_name(mode);

		if (lp_mode_name) {
			int n = 0;

			/* Get the configs from regulator_state_node subnode */
			n = fdt_subnode_offset(fdt, node, lp_mode_name);
			if (n >= 0)
				parse_low_power_mode(fdt, n, regu_desc, mode);
		}
	}
}

static TEE_Result register_pmic_regulator(const void *fdt, struct stpmic2 *pmic,
					  const char *regu_name, int node,
					  int parent_node)
{
	TEE_Result res = TEE_ERROR_GENERIC;
	const struct pmic_compat_data *cdata = pmic->compat_data;
	struct regu_dt_desc desc = { };
	struct pmic_regu *regu = NULL;
	uint32_t index = 0;
	uint8_t id = 0;
	const char *desc_name = regu_name;

	FMSG("Stpmic2 register %s", desc_name);

	/* In case of PMIC1L PMIC2L */
	if (!strcmp(desc_name, "buck1")) {
		bool high = false;

		res = stpmic2_is_buck1_high_voltage(pmic, &high);
		if (res) {
			EMSG("Failed to get %s id", desc_name);
			return res;
		}

		if (high) {
			FMSG("Buck1 in high voltage range");
			desc_name = "buck1h";
		}
	}

	/* Find the regulator platform description */
	for (index = 0; index < cdata->desc_len; index++)
		if (!strcmp(cdata->desc_table[index].name, desc_name))
			break;
	assert(index < cdata->desc_len);

	desc = cdata->desc_table[index];

	/* Find the pmic2 regulator id */
	res = stpmic2_regulator_get_id(desc_name, &id);
	if (res) {
		EMSG("Failed to get %s id", desc_name);
		return res;
	}

	regu = calloc(1, sizeof(*regu));
	if (!regu)
		return TEE_ERROR_OUT_OF_MEMORY;

	regu->pmic = pmic;
	regu->id = id;

	regu->lp_state = calloc(plat_pmic2_get_lp_mode_count(),
				sizeof(*regu));
	if (!regu->lp_state) {
		free(regu);
		return TEE_ERROR_OUT_OF_MEMORY;
	}

	regu->lp_level_uv = calloc(plat_pmic2_get_lp_mode_count(),
				   sizeof(*regu));
	if (!regu->lp_level_uv) {
		free(regu->lp_state);
		free(regu);
		return TEE_ERROR_OUT_OF_MEMORY;
	}

	desc.priv = regu;

	pmic_parse_regu_node(&desc, fdt, node);

	parse_low_power_modes(fdt, node, &desc);

	res = regulator_dt_register(fdt, node, parent_node, &desc);
	if (res) {
		EMSG("Failed to register %s", regu_name);
		free(regu->lp_level_uv);
		free(regu->lp_state);
		free(regu);
		return res;
	}

	return TEE_SUCCESS;
}

static TEE_Result parse_regulator_fdt_nodes(const void *fdt, int node,
					    struct stpmic2 *pmic)
{
	int regulators_node = 0;
	int regu_node = 0;

	regulators_node = fdt_subnode_offset(fdt, node, "regulators");
	if (regulators_node < 0)
		panic();

	fdt_for_each_subnode(regu_node, fdt, regulators_node) {
		TEE_Result res = TEE_ERROR_GENERIC;
		int status = fdt_get_status(fdt, regu_node);
		const char *regu_name = NULL;

		assert(status >= 0);
		if (status == DT_STATUS_DISABLED)
			continue;

		regu_name = fdt_get_name(fdt, regu_node, NULL);
		assert(regu_name);

		res = register_pmic_regulator(fdt, pmic, regu_name, regu_node,
					      regulators_node);
		if (res) {
			EMSG("Failed to register %s", regu_name);
			return res;
		}
	}

	return TEE_SUCCESS;
}

#ifdef CFG_STM32_PWR_IRQ

/* We expect a single instance of STPMIC2, if any */
static struct stpmic2 *stpmic2;

static void pmic_event_handler(void)
{
	assert(stpmic2);

	if (stpmic2_handle_irq(stpmic2))
		panic("STPMIC2 interrupts");
}

static void enable_stpmic2_interrupt(struct stpmic2 *pmic)
{
	struct pmic_it_handle_s *prv = NULL;

	SLIST_FOREACH(prv, &pmic->it_list, link) {
		if (stpmic2_set_irq_mask(pmic, prv->pmic_it, false)) {
			EMSG("Failed to enable STPMIC2 interrupt %u",
			     prv->pmic_it);
			panic();
		}
	}

	/* Unmask all over-current interrupts */
	if (stpmic2_register_write(pmic, INT_MASK_R3, 0x00))
		panic();

	if (stpmic2_register_write(pmic, INT_MASK_R4, 0x00))
		panic();
}

static void yielding_stm32mp2_stpmic2_notif(struct notif_driver *ndrv __unused,
					    enum notif_event ev)
{
	switch (ev) {
	case NOTIF_EVENT_DO_BOTTOM_HALF:
		pmic_event_handler();
		break;
	case NOTIF_EVENT_STOPPED:
		/*
		 * Treat events pending by the time NOTIF_EVENT_SOPPED
		 * is handled by OP-TEE notif framework.
		 */
		pmic_event_handler();
		break;
	default:
		EMSG("Unknown event %d", ev);
		panic();
	}
}

static void atomic_stm32mp2_stpmic2_notif(struct notif_driver *ndrv __unused,
					  enum notif_event ev __maybe_unused,
					  uint16_t guest_id __unused)
{
	assert(ev == NOTIF_EVENT_STARTED);
}
DECLARE_KEEP_PAGER(atomic_stm32mp2_stpmic2_notif);

/* Structure below is not const to prevent it spreads in the unpaged sections */
static struct notif_driver stm32mp2_stpmic2_notif = {
	.atomic_cb = atomic_stm32mp2_stpmic2_notif,
	.yielding_cb = yielding_stm32mp2_stpmic2_notif,
};

static enum itr_return pmic_it_handler(struct itr_handler *handler __unused)
{
	if (notif_async_is_started(0))
		notif_send_async(NOTIF_VALUE_DO_BOTTOM_HALF, 0);
	else
		pmic_event_handler();

	return ITRR_HANDLED;
}
DECLARE_KEEP_PAGER(pmic_it_handler);

enum itr_return stpmic2_irq_callback(struct stpmic2 *pmic, uint8_t it_id)
{
	struct pmic_it_handle_s *prv = NULL;

	assert(pmic == stpmic2);

	FMSG("Stpmic2 it id %d", (int)it_id);

	SLIST_FOREACH(prv, &pmic->it_list, link)
		if (prv->pmic_it == it_id) {
			FMSG("STPMIC2 send notif %u", prv->notif_id);

			notif_send_it(prv->notif_id, 0);

			return ITRR_HANDLED;
		}

	return ITRR_NONE;
}

static TEE_Result initialize_pmic2_irq(const void *fdt, int node,
				       struct stpmic2 *pmic)
{
	TEE_Result res = TEE_ERROR_GENERIC;
	const uint32_t *notif_ids = NULL;
	struct itr_chip *itr_chip = NULL;
	size_t itr_num = 0;
	int nb_notif = 0;

	FMSG("Init stpmic2 irq");

	res = interrupt_dt_get(fdt, node, &itr_chip, &itr_num);
	if (res)
		return res;

	res = interrupt_create_handler(itr_chip, itr_num, pmic_it_handler,
				       NULL, 0, NULL);
	if (res)
		panic("pmic: Could not create interrupt handler");

	SLIST_INIT(&pmic->it_list);

	if (IS_ENABLED(CFG_CORE_ASYNC_NOTIF))
		notif_register_driver(&stm32mp2_stpmic2_notif);

	notif_ids = fdt_getprop(fdt, node, "st,notif-it-id", &nb_notif);

	if (nb_notif > 0) {
		struct pmic_it_handle_s *prv = NULL;
		unsigned int i = 0;
		const uint32_t *pmic_its = NULL;
		int nb_it = 0;

		pmic_its = fdt_getprop(fdt, node, "st,pmic-it-id", &nb_it);
		if (!pmic_its) {
			EMSG("Missing property st,pmic-it-id on node %s",
			     fdt_get_name(fdt, node, NULL));
			panic();
		}

		if (nb_it != nb_notif)
			panic("st,notif-it-id incorrect description");

		for (i = 0; i < (nb_notif / sizeof(uint32_t)); i++) {
			uint8_t pmic_it = 0;

			prv = calloc(1, sizeof(*prv));
			if (!prv)
				panic("pmic: Could not allocate pmic it");

			pmic_it = fdt32_to_cpu(pmic_its[i]);

			assert(pmic_it <= IT_LDO8_OCP);

			prv->pmic_it = pmic_it;
			prv->notif_id = fdt32_to_cpu(notif_ids[i]);

			SLIST_INSERT_HEAD(&pmic->it_list, prv, link);

			FMSG("STPMIC2 forwards pmic_it:%u as notif:%u",
			     prv->pmic_it, prv->notif_id);
		}
	}

	stm32_i2c_interrupt_access_lockdeps(pmic->pmic_i2c_handle, itr_chip,
					    itr_num);

	stpmic2 = pmic;

	enable_stpmic2_interrupt(pmic);

	interrupt_enable(itr_chip, itr_num);
	if (fdt_getprop(fdt, node, "wakeup-source", NULL)) {
		if (interrupt_can_set_wake(itr_chip))
			interrupt_set_wake(itr_chip, itr_num, true);
		else
			DMSG("PMIC wakeup source ignored");
	}

	return TEE_SUCCESS;
}
#endif

static void initialize_pmic2_i2c(const void *fdt, int node,
				 struct stpmic2 *pmic, struct i2c_handle_s *i2c)
{
	const fdt32_t *cuint = NULL;
	uint32_t pmic_i2c_addr = 0;

	cuint = fdt_getprop(fdt, node, "reg", NULL);
	if (!cuint)
		panic("PMIC configuration failed on reg property");

	pmic_i2c_addr = fdt32_to_cpu(*cuint) << 1;
	if (pmic_i2c_addr > UINT16_MAX)
		panic("PMIC configuration failed on i2c address translation");

	if (!stm32_i2c_is_device_ready(i2c, pmic_i2c_addr, PMIC_I2C_TRIALS,
				       PMIC_I2C_TIMEOUT_BUSY_MS))
		panic("PMIC2 I2C init failed");

	pmic->pmic_i2c_handle = i2c;
	pmic->pmic_i2c_addr = pmic_i2c_addr;
}

/*
 * PMIC and resource initialization
 */
static void initialize_pmic2(const void *fdt, int node,
			     struct stpmic2 *pmic, struct i2c_handle_s *i2c)
{
	const struct pmic_compat_data __maybe_unused *cdata = pmic->compat_data;
	uint8_t ver = 0;
	uint8_t pid = 0;

	FMSG("Initialize stpmic2");

	initialize_pmic2_i2c(fdt, node, pmic, i2c);

	if (stpmic2_get_product_id(pmic, &pid) ||
	    stpmic2_get_version(pmic, &ver))
		panic("Failed to access PMIC");

	pmic->ref_id = ((pid & PMIC_REF_ID_MASK) >> PMIC_REF_ID_SHIFT);

	/* NVM version A stands for NVM_ID=1 */
	IMSG("PMIC STPMIC REFID:%"PRIu8".%c V%"PRIu8".%"PRIu8,
	     pmic->ref_id,
	     'A' + (pid & PMIC_NVM_ID_MASK) - U(1),
	     (ver & MAJOR_VERSION_MASK) >> MAJOR_VERSION_SHIFT,
	     ver & MINOR_VERSION_MASK);

	assert(pmic->ref_id == cdata->ref_id);

	stpmic2_dump_regulators(pmic);
}

static TEE_Result stm32_pmic2_probe(const void *fdt, int node,
				    const void *compat_data)
{
	struct stm32_i2c_dev *stm32_i2c_dev = NULL;
	struct i2c_dev *i2c_dev = NULL;
	struct stpmic2 *pmic = NULL;
	TEE_Result res = TEE_SUCCESS;

	FMSG("Probe stpmic2");

	res = i2c_dt_get_dev(fdt, node, &i2c_dev);
	if (res)
		return res;

	stm32_i2c_dev = container_of(i2c_dev, struct stm32_i2c_dev, i2c_dev);

	pmic = calloc(1, sizeof(*pmic));
	if (!pmic)
		panic();

	pmic->compat_data = compat_data;

	initialize_pmic2(fdt, node, pmic, stm32_i2c_dev->handle);

#ifdef CFG_STM32_PWR_IRQ
	res = initialize_pmic2_irq(fdt, node, pmic);
	if (res) {
		free(pmic);
		return res;
	}
#endif

	res = parse_regulator_fdt_nodes(fdt, node, pmic);
	if (res)
		panic();

	stm32_pmic2 = true;

	return TEE_SUCCESS;
}

#define DEFINE_REGU(_name) { \
		.name = (_name), \
		.ops = &pmic2_regu_ops, \
	}

#define DEFINE_SWITCH(_name) { \
		.name = (_name), \
		.ops = &pmic2_switch_ops, \
	}

const struct regu_dt_desc pmic25_reguls[] = {
	DEFINE_REGU("buck1"),
	DEFINE_REGU("buck2"),
	DEFINE_REGU("buck3"),
	DEFINE_REGU("buck4"),
	DEFINE_REGU("buck5"),
	DEFINE_REGU("buck6"),
	DEFINE_REGU("buck7"),
	DEFINE_REGU("ldo1"),
	DEFINE_REGU("ldo2"),
	DEFINE_REGU("ldo3"),
	DEFINE_REGU("ldo4"),
	DEFINE_REGU("ldo5"),
	DEFINE_REGU("ldo6"),
	DEFINE_REGU("ldo7"),
	DEFINE_REGU("ldo8"),
	DEFINE_SWITCH("refddr"),
};
DECLARE_KEEP_PAGER_PM(pmic25_reguls);

const struct regu_dt_desc pmic1l_reguls[] = {
	DEFINE_REGU("buck1"),
	DEFINE_REGU("buck1h"),
	DEFINE_REGU("buck2"),
	DEFINE_REGU("ldo2"),
	DEFINE_REGU("ldo3"),
	DEFINE_REGU("ldo4"),
	DEFINE_REGU("ldo5"),
	DEFINE_SWITCH("gpo1"),
	DEFINE_SWITCH("gpo2"),
};
DECLARE_KEEP_PAGER_PM(pmic1l_reguls);

const struct regu_dt_desc pmic2l_reguls[] = {
	DEFINE_REGU("buck1"),
	DEFINE_REGU("buck1h"),
	DEFINE_REGU("buck2"),
	DEFINE_REGU("buck3"),
	DEFINE_REGU("ldo1"),
	DEFINE_REGU("ldo2"),
	DEFINE_REGU("ldo3"),
	DEFINE_REGU("ldo4"),
	DEFINE_REGU("ldo5"),
	DEFINE_REGU("ldo6"),
	DEFINE_REGU("ldo7"),
	DEFINE_SWITCH("gpo1"),
	DEFINE_SWITCH("gpo2"),
	DEFINE_SWITCH("gpo3"),
	DEFINE_SWITCH("gpo4"),
	DEFINE_SWITCH("gpo5"),
};
DECLARE_KEEP_PAGER_PM(pmic2l_reguls);

static const struct pmic_compat_data stm32_pmic25_cdata = {
	.desc_table = pmic25_reguls,
	.desc_len = ARRAY_SIZE(pmic25_reguls),
	.ref_id =  PMIC_REF_ID_STPMIC25,
};

static const struct pmic_compat_data stm32_pmic1l_cdata = {
	.desc_table = pmic1l_reguls,
	.desc_len = ARRAY_SIZE(pmic1l_reguls),
	.ref_id =  PMIC_REF_ID_STPMIC1L,
};

static const struct pmic_compat_data stm32_pmic2l_cdata = {
	.desc_table = pmic2l_reguls,
	.desc_len = ARRAY_SIZE(pmic2l_reguls),
	.ref_id =  PMIC_REF_ID_STPMIC2L,
};

static const struct dt_device_match stm32_pmic2_match_table[] = {
	{
		.compatible = "st,stpmic2",
		.compat_data = &stm32_pmic25_cdata,
	},
	{
		.compatible = "st,stpmic1l",
		.compat_data = &stm32_pmic1l_cdata,
	},
	{
		.compatible = "st,stpmic2l",
		.compat_data = &stm32_pmic2l_cdata,
	},
	{ }
};

DEFINE_DT_DRIVER(stm32_pmic2_dt_driver) = {
	.name = "stm32_pmic2",
	.match_table = stm32_pmic2_match_table,
	.probe = stm32_pmic2_probe,
};

bool stm32_stpmic2_is_present(void)
{
	return stm32_pmic2;
}

