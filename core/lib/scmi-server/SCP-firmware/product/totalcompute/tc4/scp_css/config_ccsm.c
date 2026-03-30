/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "clock_soc.h"
#include "scp_mmap.h"
#include "tc4_core.h"

#include <mod_ccsm.h>

#include <fwk_element.h>
#include <fwk_id.h>
#include <fwk_macros.h>
#include <fwk_module.h>

/*! The slowest rate at which the PLL hardware can operate. */
#define TC4_PLL_MIN_RATE (50UL * FWK_MHZ)

/*! The fastest rate at which the PLL hardware can operate. */
#define TC4_PLL_MAX_RATE (4UL * FWK_GHZ)

/*! The maximum precision that can be used when setting the PLL rate. */
#define TC4_PLL_MIN_INTERVAL (1UL * FWK_KHZ)

static const struct mod_ccsm_clock_rate_reg_value register_table_sc_pll[] = {
    { .clock_rate_hz = 768 * FWK_MHZ,
      .pll_settings_0 = 0x030104d0,
      .pll_settings_1 = 0x00000d2e },
    { .clock_rate_hz = 946 * FWK_MHZ,
      .pll_settings_0 = 0x02010470,
      .pll_settings_1 = 0x00000e55 },
    { .clock_rate_hz = 1000 * FWK_MHZ,
      .pll_settings_0 = 0x020104b0,
      .pll_settings_1 = 0x00000fff },
    { .clock_rate_hz = 1088 * FWK_MHZ,
      .pll_settings_0 = 0x02010520,
      .pll_settings_1 = 0x00000b01 },
    { .clock_rate_hz = 1153 * FWK_MHZ,
      .pll_settings_0 = 0x02010570,
      .pll_settings_1 = 0x00000a0c },
    { .clock_rate_hz = 1260 * FWK_MHZ,
      .pll_settings_0 = 0x010103f0,
      .pll_settings_1 = 0x00000d70 },
    { .clock_rate_hz = 1383 * FWK_MHZ,
      .pll_settings_0 = 0x01010460,
      .pll_settings_1 = 0x00000126 },
    { .clock_rate_hz = 1419 * FWK_MHZ,
      .pll_settings_0 = 0x01010470,
      .pll_settings_1 = 0x00000e55 },
    { .clock_rate_hz = 1537 * FWK_MHZ,
      .pll_settings_0 = 0x010104d0,
      .pll_settings_1 = 0x00000dfe },
    { .clock_rate_hz = 1632 * FWK_MHZ,
      .pll_settings_0 = 0x01010520,
      .pll_settings_1 = 0x00000b01 },
    { .clock_rate_hz = 1660 * FWK_MHZ,
      .pll_settings_0 = 0x01010540,
      .pll_settings_1 = 0x000001b4 },
    { .clock_rate_hz = 1703 * FWK_MHZ,
      .pll_settings_0 = 0x01010560,
      .pll_settings_1 = 0x00000490 },
    { .clock_rate_hz = 1844 * FWK_MHZ,
      .pll_settings_0 = 0x010105d0,
      .pll_settings_1 = 0x000006de },
    { .clock_rate_hz = 1893 * FWK_MHZ,
      .pll_settings_0 = 0x010105f0,
      .pll_settings_1 = 0x00000e97 },
    { .clock_rate_hz = 1937 * FWK_MHZ,
      .pll_settings_0 = 0x01010620,
      .pll_settings_1 = 0x00000242 },
    { .clock_rate_hz = 1959 * FWK_MHZ,
      .pll_settings_0 = 0x01010630,
      .pll_settings_1 = 0x00000418 },
    { .clock_rate_hz = 2044 * FWK_MHZ,
      .pll_settings_0 = 0x01010670,
      .pll_settings_1 = 0x00000900 },
    { .clock_rate_hz = 2152 * FWK_MHZ,
      .pll_settings_0 = 0x010106d0,
      .pll_settings_1 = 0x0000008d },
    { .clock_rate_hz = 2176 * FWK_MHZ,
      .pll_settings_0 = 0x010106e0,
      .pll_settings_1 = 0x00000402 },
    { .clock_rate_hz = 2271 * FWK_MHZ,
      .pll_settings_0 = 0x01010730,
      .pll_settings_1 = 0x00000105 },
    { .clock_rate_hz = 2351 * FWK_MHZ,
      .pll_settings_0 = 0x01010770,
      .pll_settings_1 = 0x000001e0 },
    { .clock_rate_hz = 2385 * FWK_MHZ,
      .pll_settings_0 = 0x01010780,
      .pll_settings_1 = 0x00000d70 },
    { .clock_rate_hz = 2612 * FWK_MHZ,
      .pll_settings_0 = 0x00010420,
      .pll_settings_1 = 0x000002ba },
    { .clock_rate_hz = 2650 * FWK_MHZ,
      .pll_settings_0 = 0x00010430,
      .pll_settings_1 = 0x00000222 },
    { .clock_rate_hz = 2742 * FWK_MHZ,
      .pll_settings_0 = 0x00010450,
      .pll_settings_1 = 0x0000076c },
    { .clock_rate_hz = 3047 * FWK_MHZ,
      .pll_settings_0 = 0x000104d0,
      .pll_settings_1 = 0x0000030c },
};

static const struct mod_ccsm_clock_rate default_clock_cpu_group_little = {
    1537 * FWK_MHZ,
    1383 * FWK_MHZ
};

static const struct mod_ccsm_clock_rate default_clock_cpu_group_mid = {
    1893 * FWK_MHZ,
    1703 * FWK_MHZ
};

static const struct mod_ccsm_clock_rate default_clock_cpu_group_big = {
    2176 * FWK_MHZ,
    1958 * FWK_MHZ
};

static const struct mod_ccsm_clock_rate default_clock_cpu_group_gpu = {
    1000 * FWK_MHZ,
    0 * FWK_MHZ
};

static const struct mod_ccsm_dm_config dm_config_default = {
    .strategy = MOD_CCSM_DM_NOM_ONLY,
    .dd = MOD_CCSM_DM_ARM_DD,
    .soff = MOD_CCSM_DM_SW_SOFF_STOP,
    .transition_pause = 100,
    .mitigation_duration = 10000
};

static const struct mod_ccsm_mod_config mod_config_default = {
    .numerator_oc = 1,
    .numerator_regular = 1,
    .denominator = 1
};

static const struct mod_ccsm_clock_rate rate_table_cpu_group_little[] = {
    { /* Super Underdrive */
      .nominal_clock_rate_hz = 768 * FWK_MHZ,
      .fallback_clock_rate_hz = 0 },
    { /* Underdrive */
      .nominal_clock_rate_hz = 1153 * FWK_MHZ,
      .fallback_clock_rate_hz = 0 },
    { /* Nominal */
      .nominal_clock_rate_hz = 1537 * FWK_MHZ,
      .fallback_clock_rate_hz = 1383 * FWK_MHZ },
    { /* Overdrive */
      .nominal_clock_rate_hz = 1844 * FWK_MHZ,
      .fallback_clock_rate_hz = 1660 * FWK_MHZ },
    { /* Super Overdrive */
      .nominal_clock_rate_hz = 2152 * FWK_MHZ,
      .fallback_clock_rate_hz = 1937 * FWK_MHZ },
};

static const struct mod_ccsm_clock_rate rate_table_cpu_group_mid[] = {
    { /* Super Underdrive */
      .nominal_clock_rate_hz = 946 * FWK_MHZ,
      .fallback_clock_rate_hz = 0 },
    { /* Underdrive */
      .nominal_clock_rate_hz = 1419 * FWK_MHZ,
      .fallback_clock_rate_hz = 0 },
    { /* Nominal */
      .nominal_clock_rate_hz = 1893 * FWK_MHZ,
      .fallback_clock_rate_hz = 1703 * FWK_MHZ },
    { /* Overdrive */
      .nominal_clock_rate_hz = 2271 * FWK_MHZ,
      .fallback_clock_rate_hz = 2044 * FWK_MHZ },
    { /* Super Overdrive */
      .nominal_clock_rate_hz = 2650 * FWK_MHZ,
      .fallback_clock_rate_hz = 2385 * FWK_MHZ },
};

static const struct mod_ccsm_clock_rate rate_table_cpu_group_big[] = {
    { /* Super Underdrive */
      .nominal_clock_rate_hz = 1088 * FWK_MHZ,
      .fallback_clock_rate_hz = 0 },
    { /* Underdrive */
      .nominal_clock_rate_hz = 1632 * FWK_MHZ,
      .fallback_clock_rate_hz = 0 },
    { /* Nominal */
      .nominal_clock_rate_hz = 2176 * FWK_MHZ,
      .fallback_clock_rate_hz = 1958 * FWK_MHZ },
    { /* Overdrive */
      .nominal_clock_rate_hz = 2612 * FWK_MHZ,
      .fallback_clock_rate_hz = 2351 * FWK_MHZ },
    { /* Super Overdrive */
      .nominal_clock_rate_hz = 3047 * FWK_MHZ,
      .fallback_clock_rate_hz = 2742 * FWK_MHZ },
};

static const struct mod_ccsm_clock_rate rate_table_cpu_group_gpu[] = {
    { .nominal_clock_rate_hz = 1000 * FWK_MHZ,
      .fallback_clock_rate_hz = 0 * FWK_MHZ },
    { .nominal_clock_rate_hz = 1260 * FWK_MHZ,
      .fallback_clock_rate_hz = 0 * FWK_MHZ },
};

static const struct fwk_element ccsm_element_table[] = {
    [CLOCK_CCSM_IDX_CPU_GROUP_LITTLE] =
        {
            .name = "CPU_CCSM_" TC4_GROUP_LITTLE_NAME,
            .data = &((struct mod_ccsm_dev_config){
                .rate_lookup_table_is_provided = true,
                .minimum_clock_rate_fallback_enable_hz = 1700 * FWK_MHZ,
                .min_clock_rate_hz = TC4_PLL_MIN_RATE,
                .max_clock_rate_hz = TC4_PLL_MAX_RATE,
                .clock_rate_step_hz = TC4_PLL_MIN_INTERVAL,
    /* FVP has been hardcoded to use the little CCSM
     * as the clock selector is not currently available.
     * This differs from the default configuration which
     * uses the DSU CCSM for the little core.
     * TODO: When the clock selectors have been introduced
     * in the FVP, use them to select the clock
     * specified here */
#if defined(PLAT_FVP)
                .base_address = SCP_CCSM_LIT,
#else
                .base_address = SCP_CCSM_DSU,
#endif
                .default_rates_table = &default_clock_cpu_group_little,
                .droop_mitigation_default = &dm_config_default,
                .modulator_default = &mod_config_default,
                .pll_0_static_reg_value = 0x00003830,
                .pll_1_static_reg_value = 0x00003830,
                .modulator_count = 1,
                .register_rate_table = register_table_sc_pll,
                .register_rate_count = FWK_ARRAY_SIZE(register_table_sc_pll),
                .rate_table = rate_table_cpu_group_little,
                .rate_count = FWK_ARRAY_SIZE(rate_table_cpu_group_little),
                .timer_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_TIMER, 0),
            }),
        },
    [CLOCK_CCSM_IDX_CPU_GROUP_MID] =
        {
            .name = "CPU_CCSM_" TC4_GROUP_MID_NAME,
            .data = &((struct mod_ccsm_dev_config){
                .rate_lookup_table_is_provided = true,
                .minimum_clock_rate_fallback_enable_hz = 1700 * FWK_MHZ,
                .min_clock_rate_hz = TC4_PLL_MIN_RATE,
                .max_clock_rate_hz = TC4_PLL_MAX_RATE,
                .clock_rate_step_hz = TC4_PLL_MIN_INTERVAL,
                .base_address = SCP_CCSM_MID,
                .default_rates_table = &default_clock_cpu_group_mid,
                .droop_mitigation_default = &dm_config_default,
                .modulator_default = &mod_config_default,
                .pll_0_static_reg_value = 0x00003830,
                .pll_1_static_reg_value = 0x00003830,
                .modulator_count = 1,
                .register_rate_table = register_table_sc_pll,
                .register_rate_count = FWK_ARRAY_SIZE(register_table_sc_pll),
                .rate_table = rate_table_cpu_group_mid,
                .rate_count = FWK_ARRAY_SIZE(rate_table_cpu_group_mid),
                .timer_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_TIMER, 0),
            }),
        },
    [CLOCK_CCSM_IDX_CPU_GROUP_BIG] =
        {
            .name = "CPU_CCSM_" TC4_GROUP_BIG_NAME,
            .data = &((struct mod_ccsm_dev_config){
                .rate_lookup_table_is_provided = true,
                .minimum_clock_rate_fallback_enable_hz = 1700 * FWK_MHZ,
                .min_clock_rate_hz = TC4_PLL_MIN_RATE,
                .max_clock_rate_hz = TC4_PLL_MAX_RATE,
                .clock_rate_step_hz = TC4_PLL_MIN_INTERVAL,
                .base_address = SCP_CCSM_BIG,
                .default_rates_table = &default_clock_cpu_group_big,
                .droop_mitigation_default = &dm_config_default,
                .modulator_default = &mod_config_default,
                .pll_0_static_reg_value = 0x00003830,
                .pll_1_static_reg_value = 0x00003830,
                .modulator_count = 1,
                .register_rate_table = register_table_sc_pll,
                .register_rate_count = FWK_ARRAY_SIZE(register_table_sc_pll),
                .rate_table = rate_table_cpu_group_big,
                .rate_count = FWK_ARRAY_SIZE(rate_table_cpu_group_big),
                .timer_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_TIMER, 0),
            }),
        },
    [CLOCK_CCSM_IDX_GPU] =
        {
            .name = "CCSM_GPU",
            .data = &((struct mod_ccsm_dev_config){
                .rate_lookup_table_is_provided = true,
                .minimum_clock_rate_fallback_enable_hz = 1700 * FWK_MHZ,
                .min_clock_rate_hz = TC4_PLL_MIN_RATE,
                .max_clock_rate_hz = TC4_PLL_MAX_RATE,
                .clock_rate_step_hz = TC4_PLL_MIN_INTERVAL,
                .base_address = SCP_CCSM_GPUTOP,
                .default_rates_table = &default_clock_cpu_group_gpu,
                .droop_mitigation_default = &dm_config_default,
                .modulator_default = &mod_config_default,
                .pll_0_static_reg_value = 0x00003830,
                .pll_1_static_reg_value = 0x00003830,
                .modulator_count = 1,
                .register_rate_table = register_table_sc_pll,
                .register_rate_count = FWK_ARRAY_SIZE(register_table_sc_pll),
                .rate_table = rate_table_cpu_group_gpu,
                .rate_count = FWK_ARRAY_SIZE(rate_table_cpu_group_gpu),
                .timer_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_TIMER, 0),
            }),
        },
    [CLOCK_CCSM_IDX_COUNT] = { 0 }, /* Termination description. */
};

static const struct fwk_element *ccsm_get_element_table(fwk_id_t module_id)
{
    return ccsm_element_table;
};

const struct fwk_module_config config_ccsm = {
    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(ccsm_get_element_table),
};
