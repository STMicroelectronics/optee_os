// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2017-2023, STMicroelectronics
 */

#include <assert.h>
#include <drivers/i2c.h>
#include <drivers/regulator.h>
#include <drivers/stm32_firewall.h>
#include <drivers/stm32_i2c.h>
#include <drivers/stm32mp1_stpmic1.h>
#include <drivers/stpmic1.h>
#include <drivers/stpmic1_regulator.h>
#include <dt-bindings/mfd/st,stpmic1.h>
#include <io.h>
#include <keep.h>
#include <kernel/boot.h>
#include <kernel/delay.h>
#include <kernel/dt.h>
#include <kernel/interrupt.h>
#include <kernel/notif.h>
#include <kernel/panic.h>
#include <kernel/pm.h>
#include <libfdt.h>
#include <mm/core_memprot.h>
#include <platform_config.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stm32_util.h>
#include <trace.h>
#include <util.h>

#define MODE_STANDBY                    8

#define PMIC_I2C_TRIALS			1
#define PMIC_I2C_TIMEOUT_BUSY_MS	5

#define PMIC_REGU_SUPPLY_NAME_LEN	12

#define PMIC_REGU_COUNT			14

enum {
	PMIC_REGU_FLAG_MASK_RESET = 0,
	PMIC_REGU_FLAG_COUNT
};

static_assert(IS_ENABLED(CFG_DRIVERS_REGULATOR));
static_assert(PMIC_REGU_FLAG_COUNT <= UINT_MAX);

/*
 * struct pmic_regulator_data - Platform specific data
 * @flags: Flags for platform property to apply
 * @regu_name: Regulator name ID in stpmic1 driver
 * @voltages_desc: Supported levels description
 * @voltages_level: Pointer to supported levels or NULL if not yet allocated
 */
struct pmic_regulator_data {
	unsigned int flags;
	char *regu_name;
	struct regulator_voltages_desc voltages_desc;
	int *voltages_level;
};

/* Expect a single PMIC instance */
static struct i2c_handle_s *i2c_handle;
static uint32_t pmic_i2c_addr;
static int pmic_status = -1;

struct pmic_it_handle_s {
	uint8_t pmic_reg;
	uint8_t pmic_bit;
	uint8_t notif_id;

	SLIST_ENTRY(pmic_it_handle_s) link;
};

static SLIST_HEAD(pmic_it_handle_head, pmic_it_handle_s) pmic_it_handle_list =
	SLIST_HEAD_INITIALIZER(pmic_it_handle_list);

/* CPU voltage supplier if found */
static char cpu_supply_name[PMIC_REGU_SUPPLY_NAME_LEN];

static bool pmic_is_secure(void)
{
	assert(pmic_status != -1);

	return pmic_status == DT_STATUS_OK_SEC;
}

bool stm32mp_with_pmic(void)
{
	return pmic_status != -1;
}

static void init_pmic_state(const void *fdt, int pmic_node)
{
	pmic_status = fdt_get_status(fdt, pmic_node);
}

static void priv_dt_properties(const void *fdt, int regu_node,
			       struct pmic_regulator_data *priv)
{
	const char *name = fdt_get_name(fdt, regu_node, NULL);

	assert(name);
	priv->regu_name = strdup(name);
	if (!priv->regu_name)
		panic();

	if (fdt_getprop(fdt, regu_node, "st,mask-reset", NULL))
		priv->flags |= PMIC_REGU_FLAG_MASK_RESET;
}

/*
 * @flags: Operations expected when entering a low power sequence
 * @voltage: Target voltage to apply during low power sequences
 */
struct regu_lp_config {
	uint8_t flags;
	struct stpmic1_lp_cfg cfg;
};

#define REGU_LP_FLAG_LOAD_PWRCTRL	BIT(0)
#define REGU_LP_FLAG_ON_IN_SUSPEND	BIT(1)
#define REGU_LP_FLAG_OFF_IN_SUSPEND	BIT(2)
#define REGU_LP_FLAG_SET_VOLTAGE	BIT(3)
#define REGU_LP_FLAG_MODE_STANDBY	BIT(4)

/*
 * struct regu_lp_state - Low power configuration for regulators
 * @name: low power state identifier string name
 * @cfg_count: number of regulator configuration instance in @cfg
 * @cfg: regulator configurations for low power state @name
 */
struct regu_lp_state {
	const char *name;
	size_t cfg_count;
	struct regu_lp_config *cfg;
};

enum regu_lp_state_id {
	REGU_LP_STATE_DISK = 0,
	REGU_LP_STATE_STANDBY,
	REGU_LP_STATE_MEM,
	REGU_LP_STATE_MEM_LOWVOLTAGE,
	REGU_LP_STATE_MEM_LOWVOLTAGE_CPUOFF,
	REGU_LP_STATE_COUNT
};

static struct regu_lp_state regu_lp_state[REGU_LP_STATE_COUNT] = {
	[REGU_LP_STATE_DISK] = { .name = "standby-ddr-off", },
	[REGU_LP_STATE_STANDBY] = { .name = "standby-ddr-sr", },
	[REGU_LP_STATE_MEM] = { .name = "lp-stop", },
	[REGU_LP_STATE_MEM_LOWVOLTAGE] = { .name = "lplv-stop", },
	[REGU_LP_STATE_MEM_LOWVOLTAGE_CPUOFF] = { .name = "lplv-stop2", },
};

static unsigned int regu_lp_state2idx(const char *name)
{
	unsigned int i = 0;

	for (i = 0; i < ARRAY_SIZE(regu_lp_state); i++)
		if (!strcmp(name, regu_lp_state[i].name))
			return i;

	panic();
}

static void dt_get_regu_low_power_config(const void *fdt, const char *regu_name,
					 int regu_node, const char *lp_state)
{
	unsigned int state_idx = regu_lp_state2idx(lp_state);
	struct regu_lp_state *state = regu_lp_state + state_idx;
	const fdt32_t *cuint = NULL;
	int regu_state_node = 0;
	struct regu_lp_config *regu_cfg = NULL;

	state->cfg_count++;
	state->cfg = realloc(state->cfg,
			     state->cfg_count * sizeof(*state->cfg));
	if (!state->cfg)
		panic();

	regu_cfg = &state->cfg[state->cfg_count - 1];

	memset(regu_cfg, 0, sizeof(*regu_cfg));

	if (stpmic1_regu_has_lp_cfg(regu_name)) {
		if (stpmic1_lp_cfg(regu_name, &regu_cfg->cfg)) {
			DMSG("Cannot setup low power for regu %s", regu_name);
			panic();
		}
		/*
		 * Always copy active configuration (Control register)
		 * to PWRCTRL Control register, even if regu_state_node
		 * does not exist.
		 */
		regu_cfg->flags |= REGU_LP_FLAG_LOAD_PWRCTRL;
	}

	/* Parse regulator stte node if any */
	regu_state_node = fdt_subnode_offset(fdt, regu_node, lp_state);
	if (regu_state_node <= 0)
		return;

	if (fdt_getprop(fdt, regu_state_node,
			"regulator-on-in-suspend", NULL))
		regu_cfg->flags |= REGU_LP_FLAG_ON_IN_SUSPEND;

	if (fdt_getprop(fdt, regu_state_node,
			"regulator-off-in-suspend", NULL))
		regu_cfg->flags |= REGU_LP_FLAG_OFF_IN_SUSPEND;

	cuint = fdt_getprop(fdt, regu_state_node,
			    "regulator-suspend-microvolt", NULL);
	if (cuint) {
		uint32_t mv = fdt32_to_cpu(*cuint) / 1000U;

		if (stpmic1_lp_voltage_cfg(regu_name, mv, &regu_cfg->cfg)) {
			DMSG("Cannot set voltage for %s", regu_name);
			panic();
		}
		regu_cfg->flags |= REGU_LP_FLAG_SET_VOLTAGE;
	}

	cuint = fdt_getprop(fdt, regu_state_node,
			    "regulator-mode", NULL);
	if (cuint && fdt32_to_cpu(*cuint) == MODE_STANDBY)
		regu_cfg->flags |= REGU_LP_FLAG_MODE_STANDBY;
}

/*
 * int stm32mp_pmic_set_lp_config(char *lp_state)
 *
 * Load the low power configuration stored in regu_lp_state[].
 */
void stm32mp_pmic_apply_lp_config(const char *lp_state)
{
	unsigned int state_idx = regu_lp_state2idx(lp_state);
	struct regu_lp_state *state = &regu_lp_state[state_idx];
	size_t i = 0;

	if (stpmic1_powerctrl_on())
		panic();

	for (i = 0; i < state->cfg_count; i++) {
		struct stpmic1_lp_cfg *cfg = &state->cfg[i].cfg;

		if ((state->cfg[i].flags & REGU_LP_FLAG_LOAD_PWRCTRL) &&
		    stpmic1_lp_load_unpg(cfg))
			panic();

		if ((state->cfg[i].flags & REGU_LP_FLAG_ON_IN_SUSPEND) &&
		    stpmic1_lp_on_off_unpg(cfg, 1))
			panic();

		if ((state->cfg[i].flags & REGU_LP_FLAG_OFF_IN_SUSPEND) &&
		    stpmic1_lp_on_off_unpg(cfg, 0))
			panic();

		if ((state->cfg[i].flags & REGU_LP_FLAG_SET_VOLTAGE) &&
		    stpmic1_lp_voltage_unpg(cfg))
			panic();

		if ((state->cfg[i].flags & REGU_LP_FLAG_MODE_STANDBY) &&
		    stpmic1_lp_mode_unpg(cfg, 1))
			panic();
	}
}

/* Return a libfdt compliant status value */
static int save_cpu_supply_name(void)
{
	void *fdt = NULL;
	int node = 0;
	const fdt32_t *cuint = NULL;
	const char *name = NULL;

	fdt = get_embedded_dt();
	if (!fdt)
		panic();

	node = fdt_path_offset(fdt, "/cpus/cpu@0");
	if (node < 0)
		return -FDT_ERR_NOTFOUND;

	cuint = fdt_getprop(fdt, node, "cpu-supply", NULL);
	if (!cuint)
		return -FDT_ERR_NOTFOUND;

	node = fdt_node_offset_by_phandle(fdt, fdt32_to_cpu(*cuint));
	if (node < 0)
		return -FDT_ERR_NOTFOUND;

	name = fdt_get_name(fdt, node, NULL);
	assert(strnlen(name, sizeof(cpu_supply_name)) <
	       sizeof(cpu_supply_name));

	strncpy(cpu_supply_name, name, sizeof(cpu_supply_name));

	return 0;
}

const char *stm32mp_pmic_get_cpu_supply_name(void)
{
	return cpu_supply_name;
}

static TEE_Result pmic_set_state(struct regulator *regulator, bool enable)
{
	struct pmic_regulator_data *priv = regulator->priv;
	int ret = 0;

	stm32mp_get_pmic();

	if (enable)
		ret = stpmic1_regulator_enable(priv->regu_name);
	else
		ret = stpmic1_regulator_disable(priv->regu_name);

	stm32mp_put_pmic();

	if (ret)
		return TEE_ERROR_GENERIC;

	return TEE_SUCCESS;
}

static TEE_Result pmic_get_state(struct regulator *regulator, bool *enabled)
{
	struct pmic_regulator_data *priv = regulator->priv;

	stm32mp_get_pmic();
	*enabled = stpmic1_is_regulator_enabled(priv->regu_name);
	stm32mp_put_pmic();

	return TEE_SUCCESS;
}

static TEE_Result pmic_get_voltage(struct regulator *regulator, int *level_uv)
{
	struct pmic_regulator_data *priv = regulator->priv;
	int rc = 0;

	stm32mp_get_pmic();
	rc = stpmic1_regulator_voltage_get(priv->regu_name);
	stm32mp_put_pmic();

	if (rc < 0)
		return TEE_ERROR_GENERIC;

	*level_uv = rc * 1000;

	return TEE_SUCCESS;
}

static TEE_Result pmic_set_voltage(struct regulator *regulator, int level_uv)
{
	struct pmic_regulator_data *priv = regulator->priv;
	unsigned int level_mv = level_uv / 1000;
	int rc = 0;

	if (level_mv > UINT16_MAX)
		return TEE_ERROR_BAD_PARAMETERS;

	stm32mp_get_pmic();
	rc = stpmic1_regulator_voltage_set(priv->regu_name, level_mv);
	stm32mp_put_pmic();

	if (rc)
		return TEE_ERROR_GENERIC;

	return TEE_SUCCESS;
}

static size_t refine_levels_array(size_t count, int *levels_uv,
				  int min_uv, int max_uv)
{
	size_t n = 0;
	size_t m = 0;

	/* We need to sort the array has STPMIC1 driver does not */
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
	struct pmic_regulator_data *priv = regulator->priv;

	if (!priv->voltages_level) {
		const uint16_t *level_ref = NULL;
		size_t level_count = 0;
		int *levels2 = NULL;
		int *levels = NULL;
		size_t n = 0;

		/*
		 * Allocate and build a consised and ordered voltage list
		 * based on the voltage list provided by stpmic1 driver.
		 */
		stpmic1_regulator_levels_mv(priv->regu_name, &level_ref,
					    &level_count);

		levels = calloc(level_count, sizeof(*levels));
		if (!levels)
			return TEE_ERROR_OUT_OF_MEMORY;
		for (n = 0; n < level_count; n++)
			levels[n] = level_ref[n] * 1000;

		level_count = refine_levels_array(level_count, levels,
						  regulator->min_uv,
						  regulator->max_uv);

		/* Shrink levels array to not waste heap memory */
		levels2 = realloc(levels, sizeof(*levels) * level_count);
		if (!levels2) {
			free(levels);
			return TEE_ERROR_OUT_OF_MEMORY;
		}

		priv->voltages_desc.type = VOLTAGE_TYPE_FULL_LIST;
		priv->voltages_desc.num_levels = level_count;
		priv->voltages_level = levels2;
	}

	*out_desc = &priv->voltages_desc;
	*out_levels = priv->voltages_level;

	return TEE_SUCCESS;
}

static TEE_Result pmic_regu_init(struct regulator *regulator,
				 const void *fdt __unused, int node __unused)
{
	struct pmic_regulator_data *priv = regulator->priv;
	struct stpmic1_bo_cfg cfg = { };

	if (!priv->flags)
		return TEE_SUCCESS;

	stm32mp_get_pmic();

	if (priv->flags & PMIC_REGU_FLAG_MASK_RESET) {
		if (stpmic1_bo_mask_reset_cfg(priv->regu_name, &cfg) ||
		    stpmic1_bo_mask_reset_unpg(&cfg)) {
			EMSG("Mask reset failed for %s", priv->regu_name);
			return TEE_ERROR_GENERIC;
		}
	}

	if (regulator->flags & REGULATOR_PULL_DOWN) {
		if (stpmic1_bo_pull_down_cfg(priv->regu_name, &cfg) ||
		    stpmic1_bo_pull_down_unpg(&cfg)) {
			EMSG("Pull down failed for %s", priv->regu_name);
			return TEE_ERROR_GENERIC;
		}
	}

	stm32mp_put_pmic();

	return TEE_SUCCESS;
}

static const struct regulator_ops pmic_regu_ops = {
	.set_state = pmic_set_state,
	.get_state = pmic_get_state,
	.set_voltage = pmic_set_voltage,
	.get_voltage = pmic_get_voltage,
	.supported_voltages = pmic_list_voltages,
	.supplied_init = pmic_regu_init,
};
DECLARE_KEEP_PAGER_PM(pmic_regu_ops);

static const struct regulator_ops pmic_sw_ops = {
	.set_state = pmic_set_state,
	.get_state = pmic_get_state,
	.supplied_init = pmic_regu_init,
};
DECLARE_KEEP_PAGER_PM(pmic_sw_ops);

/*
 * STPMIC1 regulator names, used in the DT as regulator node name and
 * provider node <name>-supply property,
 */
static const char * const pmic_regu_name_ids[] = {
	"buck1", "buck2", "buck3", "buck4",
	"ldo1", "ldo2", "ldo3", "ldo4", "ldo5", "ldo6",
	"vref_ddr", "boost", "pwr_sw1", "pwr_sw2"
};

/* Preallocated regulator instances */
static struct regulator pmic_regulators[ARRAY_SIZE(pmic_regu_name_ids)];
static struct pmic_regulator_data pmic_regu_cfg[ARRAY_SIZE(pmic_regu_name_ids)];

static TEE_Result release_voltage_lists(void)
{
	size_t n = 0;

	/* Voltage list will be rebuilt at runtime if needed at least once */
	for (n = 0; n < ARRAY_SIZE(pmic_regulators); n++) {
		struct pmic_regulator_data *priv = pmic_regulators[n].priv;

		if (priv && priv->voltages_level) {
			free(priv->voltages_level);
			priv->voltages_level = NULL;
		}
	}

	return TEE_SUCCESS;
}

release_init_resource(release_voltage_lists);

struct regulator *stm32mp_pmic_get_regulator(const char *name)
{
	size_t i = 0;

	if (!name)
		return NULL;

	for (i = 0; i < ARRAY_SIZE(pmic_regu_name_ids); i++)
		if (!strcmp(pmic_regu_name_ids[i], name) &&
		    pmic_regulators[i].ops)
			return pmic_regulators + i;

	return NULL;
}

static TEE_Result register_pmic_regulator(const void *fdt,
					  const char *regu_name, int regu_node,
					  int regulators_node)
{
	TEE_Result res = TEE_ERROR_GENERIC;
	struct regu_dt_desc desc = { };
	size_t i = 0;

	for (i = 0; i < ARRAY_SIZE(pmic_regu_name_ids); i++)
		if (!strcmp(pmic_regu_name_ids[i], regu_name))
			break;
	if (i >= ARRAY_SIZE(pmic_regu_name_ids)) {
		EMSG("Unknown regulator name %s", regu_name);
		panic();
	}

	desc = (struct regu_dt_desc){
		.name = pmic_regu_name_ids[i],
		.supply_name = pmic_regu_name_ids[i],
		.regulator = pmic_regulators + i,
		.priv = pmic_regu_cfg + i,
	};

	priv_dt_properties(fdt, regu_node, pmic_regu_cfg + i);

	/*
	 * pwr_sw1 and pwr_sw2 are regulator switches hence have no
	 * set_voltage.get_voltage handler.
	 */
	if (!strncmp(regu_name, "pwr_sw", 6))
		desc.ops = &pmic_sw_ops;
	else
		desc.ops = &pmic_regu_ops;

	res = regulator_dt_register(fdt, regu_node, regulators_node, &desc);
	if (res)
		EMSG("Failed to register %s, error: %#"PRIx32, regu_name, res);

	return res;
}

static void parse_regulator_fdt_nodes(const void *fdt, int pmic_node)
{
	int regulators_node = 0;
	int regu_node = 0;

	regulators_node = fdt_subnode_offset(fdt, pmic_node, "regulators");
	if (regulators_node < 0)
		panic();

	fdt_for_each_subnode(regu_node, fdt, regulators_node) {
		int status = fdt_get_status(fdt, regu_node);
		const char *regu_name = NULL;
		size_t n = 0;

		assert(status >= 0);
		if (status == DT_STATUS_DISABLED)
			continue;

		regu_name = fdt_get_name(fdt, regu_node, NULL);

		assert(stpmic1_regulator_is_valid(regu_name));

		for (n = 0; n < ARRAY_SIZE(regu_lp_state); n++)
			dt_get_regu_low_power_config(fdt, regu_name, regu_node,
						     regu_lp_state[n].name);

		if (register_pmic_regulator(fdt, regu_name, regu_node,
					    regulators_node))
			panic();
	}

	if (save_cpu_supply_name())
		DMSG("No CPU supply provided");
}

/*
 * PMIC and resource initialization
 */

static void initialize_pmic_i2c(const void *fdt, int pmic_node)
{
	const fdt32_t *cuint = NULL;

	cuint = fdt_getprop(fdt, pmic_node, "reg", NULL);
	if (!cuint) {
		EMSG("PMIC configuration failed on reg property");
		panic();
	}

	pmic_i2c_addr = fdt32_to_cpu(*cuint) << 1;
	if (pmic_i2c_addr > UINT16_MAX) {
		EMSG("PMIC configuration failed on i2c address translation");
		panic();
	}

	stm32mp_get_pmic();

	if (!stm32_i2c_is_device_ready(i2c_handle, pmic_i2c_addr,
				       PMIC_I2C_TRIALS,
				       PMIC_I2C_TIMEOUT_BUSY_MS))
		panic();

	stpmic1_bind_i2c(i2c_handle, pmic_i2c_addr);

	stm32mp_put_pmic();
}

/* stm32mp_get/put_pmic allows secure atomic sequences to use non secure PMIC */
void stm32mp_get_pmic(void)
{
	if (!pmic_is_secure())
		stm32_i2c_resume(i2c_handle);
}

void stm32mp_put_pmic(void)
{
	if (!pmic_is_secure())
		stm32_i2c_suspend(i2c_handle);
}

static void init_pmic_secure_state(void)
{
	TEE_Result res = TEE_SUCCESS;
	const struct stm32_firewall_cfg sec_cfg[] = {
		{ FWLL_SEC_RW | FWLL_MASTER(0) },
		{ }, /* Null terminated */
	};

	res = stm32_firewall_check_access(i2c_handle->base.pa,
					  0, sec_cfg);

	if (!res)
		pmic_status = DT_STATUS_OK_SEC;
	else
		pmic_status = DT_STATUS_OK_NSEC;
}

static TEE_Result initialize_pmic(const void *fdt, int pmic_node)
{
	unsigned long pmic_version = 0;

	init_pmic_state(fdt, pmic_node);

	init_pmic_secure_state();

	initialize_pmic_i2c(fdt, pmic_node);

	stm32mp_get_pmic();

	if (stpmic1_get_version(&pmic_version))
		panic("Failed to access PMIC");

	DMSG("PMIC version = 0x%02lx", pmic_version);
	stm32mp_put_pmic();

	parse_regulator_fdt_nodes(fdt, pmic_node);

	return TEE_SUCCESS;
}

static void pmic_event_handler(void)
{
	uint8_t read_val = 0U;
	unsigned int i = 0U;
	uint8_t latch1 = 0U;
	uint8_t latch2 = 0U;
	uint8_t latch3 = 0U;
	uint8_t latch4 = 0U;

	FMSG("Stpmic1 irq handler");

	stm32mp_get_pmic();

	do {
		for (i = 0U; i < 4U; i++) {
			if (stpmic1_register_read(ITLATCH1_REG + i, &read_val))
				panic();

			if (read_val) {
				struct pmic_it_handle_s *prv = NULL;

				FMSG("Stpmic1 irq pending %u: %#"PRIx8, i,
				     read_val);

				if (stpmic1_register_write(ITCLEARLATCH1_REG +
							   i, read_val))
					panic();

				SLIST_FOREACH(prv, &pmic_it_handle_list, link)
					if ((prv->pmic_reg == ITCLEARMASK1_REG +
					     i) &&
					    (read_val & BIT(prv->pmic_bit))) {
						FMSG("STPMIC1 send notif %u",
						     prv->notif_id);

						notif_send_it(prv->notif_id, 0);
				}
			}
		}

		stpmic1_register_read(ITLATCH1_REG, &latch1);
		stpmic1_register_read(ITLATCH2_REG, &latch2);
		stpmic1_register_read(ITLATCH3_REG, &latch3);
		stpmic1_register_read(ITLATCH4_REG, &latch4);

	} while ((latch1 != 0) || (latch2 != 0) || (latch3 != 0) ||
		 (latch4 != 0));

	stm32mp_put_pmic();
}

static void enable_stpmic1_interrupt(void)
{
	struct pmic_it_handle_s *prv = NULL;
	stm32mp_get_pmic();

	/* Enable requested interrupt */
	SLIST_FOREACH(prv, &pmic_it_handle_list, link) {
		if (stpmic1_register_update(prv->pmic_reg,
					    BIT(prv->pmic_bit),
					    BIT(prv->pmic_bit)))
			panic();
	}

	stm32mp_put_pmic();
}

static void yielding_stm32mp1_stpmic1_notif(struct notif_driver *ndrv __unused,
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

static bool atomic_stm32mp1_stpmic1_notif(struct notif_driver *ndrv __unused,
					  enum notif_event ev __maybe_unused,
					  uint16_t guest_id __unused)
{
	assert(ev == NOTIF_EVENT_STARTED);

	return true;
}
DECLARE_KEEP_PAGER(atomic_stm32mp1_stpmic1_notif);

/* Structure below is not const to prevent it spreads in the unpaged sections */
static struct notif_driver stm32mp1_stpmic1_notif = {
	.atomic_cb = atomic_stm32mp1_stpmic1_notif,
	.yielding_cb = yielding_stm32mp1_stpmic1_notif,
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

static TEE_Result stm32_pmic_init_it(const void *fdt, int node)
{
	TEE_Result res = TEE_ERROR_GENERIC;
	const uint32_t *notif_ids = NULL;
	struct itr_chip *itr_chip = NULL;
	size_t itr_num = 0;
	int nb_notif = 0;

	if (IS_ENABLED(CFG_CORE_ASYNC_NOTIF))
		notif_register_driver(&stm32mp1_stpmic1_notif);

	res = interrupt_dt_get(fdt, node, &itr_chip, &itr_num);
	/*
	 * Allow to not handle interrupts.
	 * On STM32MP15 interrupts can be handled by linux.
	 */
	if (IS_ENABLED(CFG_STM32MP15) && (res == TEE_ERROR_ITEM_NOT_FOUND))
		return TEE_SUCCESS;

	if (res)
		return res;

	notif_ids = fdt_getprop(fdt, node, "st,notif-it-id", &nb_notif);
	if (!notif_ids)
		return TEE_ERROR_ITEM_NOT_FOUND;

	if (nb_notif > 0) {
		struct pmic_it_handle_s *prv = NULL;
		unsigned int i = 0;
		const uint32_t *pmic_its = NULL;
		int nb_it = 0;

		pmic_its = fdt_getprop(fdt, node, "st,pmic-it-id", &nb_it);
		if (!pmic_its)
			return TEE_ERROR_ITEM_NOT_FOUND;

		if (nb_it != nb_notif)
			panic("st,notif-it-id incorrect description");

		for (i = 0; i < (nb_notif / sizeof(uint32_t)); i++) {
			uint8_t pmic_it = 0;

			prv = calloc(1, sizeof(*prv));
			if (!prv)
				panic("pmic: Could not allocate pmic it");

			pmic_it = fdt32_to_cpu(pmic_its[i]);

			assert(pmic_it <= IT_SWIN_R);

			prv->pmic_reg = ITCLEARMASK1_REG + pmic_it / U(8);
			prv->pmic_bit = pmic_it % U(8);
			prv->notif_id = fdt32_to_cpu(notif_ids[i]);

			SLIST_INSERT_HEAD(&pmic_it_handle_list, prv, link);

			FMSG("STPMIC1 forwards irq reg:%u bit:%u as notif:%u",
			     prv->pmic_reg, prv->pmic_bit, prv->notif_id);
		}
	}

	res = interrupt_create_handler(itr_chip, itr_num, pmic_it_handler,
				       NULL, 0, NULL);
	if (res)
		panic("pmic: Could not create interrupt handler");

	stm32_i2c_interrupt_access_lockdeps(i2c_handle, itr_chip, itr_num);

	enable_stpmic1_interrupt();

	interrupt_enable(itr_chip, itr_num);
	if (fdt_getprop(fdt, node, "wakeup-source", NULL)) {
		if (interrupt_can_set_wake(itr_chip))
			interrupt_set_wake(itr_chip, itr_num, true);
		else
			DMSG("PMIC wakeup source ignored");
	}

	return TEE_SUCCESS;
}

static TEE_Result stm32_pmic_probe(const void *fdt, int node,
				   const void *compat_data __unused)
{
	struct stm32_i2c_dev *stm32_i2c_dev = NULL;
	struct i2c_dev *i2c_dev = NULL;
	TEE_Result res = TEE_SUCCESS;

	res = i2c_dt_get_dev(fdt, node, &i2c_dev);
	if (res)
		return res;

	stm32_i2c_dev = container_of(i2c_dev, struct stm32_i2c_dev, i2c_dev);
	i2c_handle = stm32_i2c_dev->handle;

	res = initialize_pmic(fdt, node);
	if (res) {
		DMSG("Unexpectedly failed to get I2C bus: %#"PRIx32, res);
		panic();
	}

	return stm32_pmic_init_it(fdt, node);
}

static const struct dt_device_match stm32_pmic_match_table[] = {
	{ .compatible = "st,stpmic1" },
	{ }
};

DEFINE_DT_DRIVER(stm32_pmic_dt_driver) = {
	.name = "st,stpmic1",
	.match_table = stm32_pmic_match_table,
	.probe = stm32_pmic_probe,
};
