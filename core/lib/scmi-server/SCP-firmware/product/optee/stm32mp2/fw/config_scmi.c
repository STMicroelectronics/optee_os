/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, STMicroelectronics and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <scmi_agents.h>

#include <mod_clock.h>
#include <mod_optee_mbx.h>
#include <mod_scmi.h>

#include <fwk_assert.h>
#include <fwk_element.h>
#include <fwk_id.h>
#include <fwk_mm.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>

#include <arch_main.h>

#ifdef CFG_SCPFW_MOD_DVFS
#include <mod_dvfs.h>
#endif
#ifdef CFG_SCPFW_MOD_OPTEE_CLOCK
#    include <mod_optee_clock.h>
#endif
#ifdef CFG_SCPFW_MOD_OPTEE_RESET
#    include <mod_optee_reset.h>
#endif
#ifdef CFG_SCPFW_MOD_MSG_SMT
#    include <mod_msg_smt.h>
#endif
#ifdef CFG_SCPFW_MOD_OPTEE_SMT
#include <mod_optee_smt.h>
#endif
#ifdef CFG_SCPFW_MOD_OPTEE_VOLTD_REGULATOR
#include <mod_optee_voltd_regulator.h>
#endif
#ifdef CFG_SCPFW_MOD_POWER_DOMAIN
#include <mod_power_domain.h>
#include <mod_stm32_pd.h>
#endif
#ifdef CFG_SCPFW_MOD_PSU
#include <mod_psu.h>
#endif
#ifdef CFG_SCPFW_MOD_PSU_OPTEE_REGULATOR
#include <mod_psu_optee_regulator.h>
#endif
#ifdef CFG_SCPFW_MOD_RESET_DOMAIN
#    include <mod_reset_domain.h>
#endif
#ifdef CFG_SCPFW_MOD_SCMI_CLOCK
#    include <mod_scmi_clock.h>
#endif
#ifdef CFG_SCPFW_MOD_SCMI_PERF
#include <mod_scmi_perf.h>
#endif
#ifdef CFG_SCPFW_MOD_SCMI_POWER_DOMAIN
#include <mod_scmi_power_domain.h>
#endif
#ifdef CFG_SCPFW_MOD_SCMI_RESET_DOMAIN
#    include <mod_scmi_reset_domain.h>
#endif
#ifdef CFG_SCPFW_MOD_SCMI_VOLTAGE_DOMAIN
#include <mod_scmi_voltage_domain.h>
#endif
#ifdef CFG_SCPFW_MOD_STM32_OPP
#include <mod_stm32_opp.h>
#endif
#ifdef CFG_SCPFW_MOD_VOLTAGE_DOMAIN
#include <mod_voltage_domain.h>
#endif

#include <drivers/regulator.h>
#include <drivers/stm32_cpu_opp.h>
#include <kernel/panic.h>
#include <scmi_agent_configuration.h>
#include <util.h>

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

/* SCMI agent and services (channels) */
static struct mod_scmi_agent *scmi_agent_table;
static struct mod_scmi_config scmi_data;
static struct fwk_element *scmi_service_elt;

/* SCMI channel mailbox/shmem */
#ifdef CFG_SCPFW_MOD_MSG_SMT
static struct fwk_element *msg_smt_elt;
static struct mod_msg_smt_channel_config *msg_smt_data;
#endif
#ifdef CFG_SCPFW_MOD_OPTEE_SMT
static struct fwk_element *optee_smt_elt;
static struct mod_optee_smt_channel_config *optee_smt_data;
#endif
static struct fwk_element *optee_mbx_elt;
static struct mod_optee_mbx_channel_config *optee_mbx_data;

#ifdef CFG_SCPFW_MOD_SCMI_CLOCK
/* SCMI clock generic */
static struct fwk_element *scmi_clock_elt;
static struct mod_scmi_clock_agent_config *scmi_clk_agent_cfg;
#endif

#ifdef CFG_SCPFW_MOD_CLOCK
/*
 * Clocks and optee/clock, same number/indices.
 * Elements and configuration data.
 */
static struct fwk_element *optee_clock_elt;
static struct mod_optee_clock_config *optee_clock_cfg;
static struct fwk_element *clock_elt;
static struct mod_clock_dev_config *clock_data;
#endif

#ifdef CFG_SCPFW_MOD_DVFS
/* 1 DVFS (elt & data) per DVFS exposed */
static struct mod_dvfs_domain_config *dvfs_data;
static struct fwk_element *dvfs_elt;
/* 1 STM32 OPP (conf & elt) per DVFS exposed */
static struct mod_stm32_opp_config *stm32_opp_cfg;
static struct fwk_element *stm32_opp_elt;
/* A unique scmi_perf instance refers to perf domains data (DVFS instance) */
static struct mod_scmi_perf_domain_config *scmi_perf_domain_data;
struct fwk_module_config config_scmi_perf;

/* PSU and optee/psu-regu, used with DVFS. Elements and configuration data */
static struct fwk_element *psu_optee_regu_elt;
static struct mod_psu_optee_regulator_dev_config *psu_optee_regu_data;
static struct fwk_element *psu_elt;
static struct mod_psu_element_cfg *psu_data;
#endif

#ifdef CFG_SCPFW_MOD_RESET_DOMAIN
/* SCMI reset domains and optee reset controller */
static struct mod_scmi_reset_domain_agent *scmi_reset_agent_tbl;
static struct fwk_element *optee_reset_elt;
static struct mod_optee_reset_dev_config *optee_reset_data;
static struct fwk_element *reset_elt;
static struct mod_reset_domain_dev_config *reset_data;
#endif

#ifdef CFG_SCPFW_MOD_VOLTAGE_DOMAIN
/* SCMI voltage domains and optee regulators */
static struct mod_scmi_voltd_agent *scmi_voltd_agent_tbl;
static struct fwk_element *optee_regu_elt;
static struct mod_optee_voltd_regulator_dev_config *optee_regu_data;
static struct fwk_element *voltd_elt;
static struct mod_voltd_dev_config *voltd_data;
#endif

#ifdef CFG_SCPFW_MOD_POWER_DOMAIN
static struct fwk_element *pd_elt;
static struct fwk_element *stm32_pd_elt;
#endif

/* Config data for scmi module */
static const struct fwk_element *get_scmi_service_table(fwk_id_t module_id)
{
    return scmi_service_elt; /* scmi_service_elt filled during initialization */
}

struct fwk_module_config config_scmi = {
    .data = (void *)&scmi_data, /* scmi_data filled during initialization */
    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(get_scmi_service_table),
};

/* Config data for optee_mbx module */
static const struct fwk_element *optee_mbx_get_element_table(fwk_id_t module_id)
{
    return (const struct fwk_element *)optee_mbx_elt;
}

struct fwk_module_config config_optee_mbx = {
    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(optee_mbx_get_element_table),
};

#ifdef CFG_SCPFW_MOD_MSG_SMT
/* Config data for msg_smt module */
static const struct fwk_element *msg_smt_get_element_table(fwk_id_t module_id)
{
    fwk_assert(fwk_id_get_module_idx(module_id) == FWK_MODULE_IDX_MSG_SMT);
    return (const struct fwk_element *)msg_smt_elt;
}

struct fwk_module_config config_msg_smt = {
    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(msg_smt_get_element_table),
};
#endif

#ifdef CFG_SCPFW_MOD_OPTEE_SMT
/* Config data for optee_smt module */
static const struct fwk_element *optee_smt_get_element_table(fwk_id_t module_id)
{
    fwk_assert(fwk_id_get_module_idx(module_id) == FWK_MODULE_IDX_OPTEE_SMT);
    return (const struct fwk_element *)optee_smt_elt;
};

struct fwk_module_config config_optee_smt = {
    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(optee_smt_get_element_table),
};
#endif

/* Config data for scmi_clock, clock and optee_clock modules */
#ifdef CFG_SCPFW_MOD_SCMI_CLOCK
static const struct fwk_element *scmi_clock_get_element_table(
    fwk_id_t module_id)
{
    return (const struct fwk_element *)scmi_clock_elt;
}

struct fwk_module_config config_scmi_clock = {
    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(scmi_clock_get_element_table),
    .data = &((const struct mod_scmi_clock_config){
        .max_pending_transactions = 0,
    }),
};
#endif

#ifdef CFG_SCPFW_MOD_CLOCK
static const struct fwk_element *clock_get_element_table(fwk_id_t module_id)
{
    fwk_assert(fwk_id_get_module_idx(module_id) == FWK_MODULE_IDX_CLOCK);
    return (const struct fwk_element *)clock_elt;
}

struct fwk_module_config config_clock = {
    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(clock_get_element_table),
};

static const struct fwk_element *optee_clock_get_element_table(
    fwk_id_t module_id)
{
    fwk_assert(fwk_id_get_module_idx(module_id) == FWK_MODULE_IDX_OPTEE_CLOCK);
    return (const struct fwk_element *)optee_clock_elt;
}

struct fwk_module_config config_optee_clock = {
    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(optee_clock_get_element_table),
};
#endif

/* Config data for scmi_reset_domain, reset_domain and optee_reset modules */
#ifdef CFG_SCPFW_MOD_RESET_DOMAIN
struct fwk_module_config config_scmi_reset_domain = {
    .data = &((struct mod_scmi_reset_domain_config){
        .agent_table = NULL, /* Allocated during initialization */
        .agent_count = 0, /* Set during initialization */
    }),
};

static const struct fwk_element *reset_get_element_table(fwk_id_t module_id)
{
    fwk_assert(fwk_id_get_module_idx(module_id) == FWK_MODULE_IDX_RESET_DOMAIN);
    return (const struct fwk_element *)reset_elt;
}

struct fwk_module_config config_reset_domain = {
    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(reset_get_element_table),
};

static const struct fwk_element *optee_reset_get_element_table(
    fwk_id_t module_id)
{
    fwk_assert(fwk_id_get_module_idx(module_id) == FWK_MODULE_IDX_OPTEE_RESET);
    return (const struct fwk_element *)optee_reset_elt;
}

struct fwk_module_config config_optee_reset = {
    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(optee_reset_get_element_table),
};
#endif

#ifdef CFG_SCPFW_MOD_VOLTAGE_DOMAIN
/* Config data for scmi_voltage_domain, voltage_domain and optee_regu modules */
struct fwk_module_config config_scmi_voltage_domain = {
    .data = &((struct mod_scmi_voltd_config){
        .agent_table = NULL,			/* Allocated during initialization */
        .agent_count = 0,			/* Set during initialization */
    }),
};

static const struct fwk_element *voltd_get_element_table(fwk_id_t module_id)
{
    fwk_assert(fwk_id_get_module_idx(module_id) == FWK_MODULE_IDX_VOLTAGE_DOMAIN);
    return (const struct fwk_element *)voltd_elt;
}

struct fwk_module_config config_voltage_domain = {
    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(voltd_get_element_table),
};

static const struct fwk_element *optee_voltd_regulator_get_element_table(fwk_id_t module_id)
{
    fwk_assert(fwk_id_get_module_idx(module_id) == FWK_MODULE_IDX_OPTEE_VOLTD_REGULATOR);
    return (const struct fwk_element *)optee_regu_elt;
}

struct fwk_module_config config_optee_voltd_regulator = {
    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(optee_voltd_regulator_get_element_table),
};
#endif

#ifdef CFG_SCPFW_MOD_DVFS
/* Config data for scmi_perf and dvfs module */
struct fwk_module_config config_scmi_perf = {
    .data = &((struct mod_scmi_perf_config){
        .domains = NULL,				/* Allocated during initialization */
        .perf_doms_count = 0,			/* Set during initialization */
#ifdef BUILD_HAS_MOD_STATISTICS
        .stats_enabled = true,
#endif
    }),
};

static const struct fwk_element *dvfs_get_element_table(fwk_id_t module_id)
{
    fwk_assert(fwk_id_get_module_idx(module_id) == FWK_MODULE_IDX_DVFS);
    return (const struct fwk_element *)dvfs_elt;
}

struct fwk_module_config config_dvfs = {
    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(dvfs_get_element_table),
};

/* Config data for psu and optee/psu module */
static const struct fwk_element *psu_get_element_table(fwk_id_t module_id)
{
    fwk_assert(fwk_id_get_module_idx(module_id) == FWK_MODULE_IDX_PSU);
    return (const struct fwk_element *)psu_elt;
}

struct fwk_module_config config_psu = {
    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(psu_get_element_table),
};

static const struct fwk_element *psu_optee_regu_get_element_table(fwk_id_t module_id)
{
    fwk_assert(fwk_id_get_module_idx(module_id) == FWK_MODULE_IDX_PSU_OPTEE_REGULATOR);
    return (const struct fwk_element *)psu_optee_regu_elt ;
}

struct fwk_module_config config_psu_optee_regulator = {
    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(psu_optee_regu_get_element_table),
};

static const struct fwk_element *stm32_opp_get_element_table(fwk_id_t module_id)
{
    fwk_assert(fwk_id_get_module_idx(module_id) == FWK_MODULE_IDX_STM32_OPP);
    return (const struct fwk_element *)stm32_opp_elt;
}

struct fwk_module_config config_stm32_opp = {
    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(stm32_opp_get_element_table),
};
#endif

#ifdef CFG_SCPFW_MOD_POWER_DOMAIN
/* SCMI power domain module*/
struct fwk_module_config config_scmi_power_domain = { 0 };

/* Power domain module */
enum pd_static_dev_idx {
    PD_STATIC_DEV_IDX_NONE = UINT32_MAX
};

static const uint32_t pd_allowed_state_mask_table[] = {
    [MOD_PD_STATE_OFF] = MOD_PD_STATE_OFF_MASK,
    [MOD_PD_STATE_ON] = MOD_PD_STATE_OFF_MASK | MOD_PD_STATE_ON_MASK,
};

static const struct mod_power_domain_config power_domain_config = { 0 };

static const struct fwk_element *pd_get_element_table(fwk_id_t module_id)
{
    return (const struct fwk_element *)pd_elt;
}

struct fwk_module_config config_power_domain = {
    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(pd_get_element_table),
    .data = &power_domain_config,
};

/* STM32 power domain module */
static const struct fwk_element *stm32_pd_get_element_table(fwk_id_t module_id)
{
    fwk_assert(fwk_id_get_module_idx(module_id) == FWK_MODULE_IDX_STM32_PD);
    return (const struct fwk_element *)stm32_pd_elt;
}

struct fwk_module_config config_stm32_pd = {
    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(stm32_pd_get_element_table),
};
#endif

/*
 * Indices state when applying agents configuration
 * @channel_count: Number of channels (mailbox/shmem links) used
 * @clock_index: Current index for clock
 * @clock_count: Number of clocks
 * @optee_clock_index: Current index for optee/clock
 * @optee_clock_count: Number of optee/clock
 * @reset_index: Current index for reset controller and optee/reset
 * @reset_count: Number of reset controller (optee/reset) instances
 * @regu_index: Current index for voltd and optee/regulator
 * @regu_count: Number of voltd (optee/regulator) instances
 * @psu_index: Current index for PSU and optee/PSU instances
 * @psu_count: Number of PSU (optee/PSU) instances
 * @dvfs_index: Current index for DVFS instance
 * @dvfs_count: Number of DVFS instances
 * @pd_index: Current index for power domain instance
 * @pd_count: Number of power domain instances
 */
struct scpfw_resource_counter {
    size_t channel_count;
    size_t clock_index;
    size_t clock_count;
    size_t optee_clock_index;
    size_t optee_clock_count;
    size_t reset_index;
    size_t reset_count;
    size_t regu_index;
    size_t regu_count;
    size_t psu_index;
    size_t psu_count;
    size_t dvfs_index;
    size_t dvfs_count;
    size_t pd_index;
    size_t pd_count;
} scpfw_resource_counter;

/*
 * Count once for all the several instances and allocate global resources.
 * Global resources are clock, optee/clock, reset, optee/reset, regu,
 * optee/regu, psu, optee/psu, dvfs, perfd, ...;
 */
static void count_resources(struct scpfw_config *cfg)
{
    for (size_t i = 0; i < cfg->agent_count; i++) {
        struct scpfw_agent_config *agent_cfg = cfg->agent_config + i;

        scpfw_resource_counter.channel_count += agent_cfg->channel_count;

        for (size_t j = 0; j < agent_cfg->channel_count; j++) {
            struct scpfw_channel_config *channel_cfg =
                agent_cfg->channel_config + j;

#ifdef CFG_SCPFW_MOD_CLOCK
            /* Clocks for scmi_clock optee/clock and for DVFS */
            scpfw_resource_counter.clock_count += channel_cfg->clock_count;
            scpfw_resource_counter.optee_clock_count += channel_cfg->clock_count;
#ifdef CFG_SCPFW_MOD_DVFS
            scpfw_resource_counter.clock_count += channel_cfg->perfd_count;
#endif
#endif
#ifdef CFG_SCPFW_MOD_RESET_DOMAIN
            /* Reset for scmi_reset only */
            scpfw_resource_counter.reset_count += channel_cfg->reset_count;
#endif
#ifdef CFG_SCPFW_MOD_VOLTAGE_DOMAIN
            /* Regulators for smci_voltage_domain only */
            scpfw_resource_counter.regu_count += channel_cfg->voltd_count;
#endif
#ifdef CFG_SCPFW_MOD_DVFS
            /* DVFS and PSU DVFS only */
            scpfw_resource_counter.dvfs_count += channel_cfg->perfd_count;
            scpfw_resource_counter.psu_count += channel_cfg->perfd_count;
#endif
#ifdef CFG_SCPFW_MOD_POWER_DOMAIN
            /* Power domains */
            scpfw_resource_counter.pd_count += channel_cfg->pd_count;
#endif
        }
    }

#ifndef CFG_SCPFW_MOD_CLOCK
    fwk_assert(!scpfw_resource_counter.clock_count);
    fwk_assert(!scpfw_resource_counter.optee_clock_count);
#endif
#ifndef CFG_SCPFW_MOD_RESET_DOMAIN
    fwk_assert(!scpfw_resource_counter.reset_count);
#endif
#ifndef CFG_SCPFW_MOD_VOLTAGE_DOMAIN
    fwk_assert(!scpfw_resource_counter.regu_count);
#endif
#ifndef CFG_SCPFW_MOD_DVFS
    fwk_assert(!scpfw_resource_counter.dvfs_count);
#endif
#ifndef CFG_SCPFW_MOD_PSU
    fwk_assert(!scpfw_resource_counter.psu_count);
#endif
#ifndef CFG_SCPFW_MOD_POWER_DOMAIN
    fwk_assert(!scpfw_resource_counter.pd_count);
#endif
}

/*
 * Allocate all tables that may be needed. An optimized implementation would
 * allocate a single piece of memory and set the pointers accordingly.
 */
static void allocate_global_resources(struct scpfw_config *cfg)
{
    struct mod_scmi_reset_domain_config *scmi_reset_config __maybe_unused;
    struct mod_scmi_voltd_config *scmi_voltd_config __maybe_unused;
    struct mod_scmi_perf_config *scmi_perf_data __maybe_unused;

    size_t __maybe_unused scmi_agent_count;

    /*
     * @cfg does not consider agent #0,
     * that is the reserved platform/server agent.
     */
    scmi_agent_count = cfg->agent_count + 1;

#ifdef CFG_SCPFW_MOD_SCMI_CLOCK
    /* SCMI clock domains resources */
    scmi_clock_elt =
        fwk_mm_calloc(scmi_agent_count + 1, sizeof(*scmi_clock_elt));
    scmi_clk_agent_cfg =
        fwk_mm_calloc(scmi_agent_count, sizeof(*scmi_clk_agent_cfg));

    scmi_clock_elt[SCMI_AGENT_ID_RSV] = (struct fwk_element){
        .name = "",
        .data = scmi_clk_agent_cfg + SCMI_AGENT_ID_RSV,
    };
#endif

#ifdef CFG_SCPFW_MOD_CLOCK
    /* Clock domains resources */
    optee_clock_cfg = fwk_mm_calloc(
        scpfw_resource_counter.optee_clock_count, sizeof(*optee_clock_cfg));
    optee_clock_elt = fwk_mm_calloc(
        scpfw_resource_counter.optee_clock_count + 1, sizeof(*optee_clock_elt));

    clock_data =
        fwk_mm_calloc(scpfw_resource_counter.clock_count, sizeof(*clock_data));
    clock_elt = fwk_mm_calloc(
        scpfw_resource_counter.clock_count + 1, sizeof(*clock_elt));
#endif

#ifdef CFG_SCPFW_MOD_RESET_DOMAIN
    /* SCMI reset domains resources */
    scmi_reset_agent_tbl =
        fwk_mm_calloc(scmi_agent_count, sizeof(*scmi_reset_agent_tbl));
    scmi_reset_config = (void *)config_scmi_reset_domain.data;
    scmi_reset_config->agent_table = scmi_reset_agent_tbl;
    scmi_reset_config->agent_count = scmi_agent_count;

    optee_reset_data = fwk_mm_calloc(
        scpfw_resource_counter.reset_count, sizeof(*optee_reset_data));
    optee_reset_elt = fwk_mm_calloc(
        scpfw_resource_counter.reset_count + 1, sizeof(*optee_reset_elt));

    reset_data =
        fwk_mm_calloc(scpfw_resource_counter.reset_count, sizeof(*reset_data));
    reset_elt = fwk_mm_calloc(
        scpfw_resource_counter.reset_count + 1, sizeof(*reset_elt));
#endif

#ifdef CFG_SCPFW_MOD_DVFS
    /* PSU and related optee PSU regulator resources */
    psu_optee_regu_elt = fwk_mm_calloc(scpfw_resource_counter.dvfs_count + 1,
                                  sizeof(*psu_optee_regu_elt));
    psu_optee_regu_data = fwk_mm_calloc(scpfw_resource_counter.dvfs_count,
                                  sizeof(*psu_optee_regu_data));

    psu_elt = fwk_mm_calloc(scpfw_resource_counter.psu_count + 1,
                            sizeof(*psu_elt));
    psu_data = fwk_mm_calloc(scpfw_resource_counter.psu_count,
                             sizeof(*psu_data));

    /* DVFS and SCMI performance management resources */
    dvfs_elt = fwk_mm_calloc(scpfw_resource_counter.dvfs_count + 1,
                             sizeof(*dvfs_elt));
    dvfs_data = fwk_mm_calloc(scpfw_resource_counter.dvfs_count,
                              sizeof(*dvfs_data));

    /* STM32 OPP resources */
    stm32_opp_cfg = fwk_mm_calloc(scpfw_resource_counter.dvfs_count,
                                  sizeof(*stm32_opp_cfg));
    stm32_opp_elt = fwk_mm_calloc(scpfw_resource_counter.dvfs_count + 1,
                                  sizeof(*stm32_opp_elt));

    scmi_perf_domain_data = fwk_mm_calloc(scpfw_resource_counter.dvfs_count,
                                          sizeof(*scmi_perf_domain_data));

    /*
     * DVFS with SCMI performance management domains
     * 1 initial scmi_perf instance defines the number of DVFS's
     */
    scmi_perf_data = (void *)config_scmi_perf.data;
    scmi_perf_data->domains = (struct mod_scmi_perf_domain_config (*)[])
        scmi_perf_domain_data;
    scmi_perf_data->perf_doms_count = scpfw_resource_counter.dvfs_count;
#endif

#ifdef CFG_SCPFW_MOD_VOLTAGE_DOMAIN
    /* SCMI voltage domains resources */
    scmi_voltd_agent_tbl = fwk_mm_calloc(scmi_agent_count,
                                         sizeof(*scmi_voltd_agent_tbl));
    scmi_voltd_config = (void *)config_scmi_voltage_domain.data;
    scmi_voltd_config->agent_table = scmi_voltd_agent_tbl;
    scmi_voltd_config->agent_count = scmi_agent_count;

    optee_regu_data = fwk_mm_calloc(scpfw_resource_counter.regu_count,
                                    sizeof(*optee_regu_data));
    optee_regu_elt = fwk_mm_calloc(scpfw_resource_counter.regu_count + 1,
                                    sizeof(*optee_regu_elt));

    voltd_data = fwk_mm_calloc(scpfw_resource_counter.regu_count,
                               sizeof(*voltd_data));
    voltd_elt = fwk_mm_calloc(scpfw_resource_counter.regu_count + 1,
                              sizeof(*voltd_elt));
#endif

#ifdef CFG_SCPFW_MOD_POWER_DOMAIN
    /* <pd_count> power domains + 1 system domain + 1 empty cell */
    pd_elt = fwk_mm_calloc(scpfw_resource_counter.pd_count + 2,
                              sizeof(*pd_elt));
    stm32_pd_elt = fwk_mm_calloc(scpfw_resource_counter.pd_count + 1,
                              sizeof(*stm32_pd_elt));
#endif
}

enum mailbox_type {
    MAILBOX_TYPE_MSG_SMT,
    MAILBOX_TYPE_OPTEE_SMT,
    MAILBOX_TYPE_COUNT,
};

static enum mailbox_type module_idx_smt_table[SCMI_AGENT_ID_COUNT] = {
    /*
     * Agent NSEC0 is Cortex-A that communicates over OP-TEE's SCMI PTA
     * and OP-TEE shared memory hence using MSG_SMT module.
     */
    [SCMI_AGENT_ID_NSEC0] = MAILBOX_TYPE_MSG_SMT,
    /*
     * Agent NSEC1 is Cortex-M33 that communicates over STM32 IPCC and a piece
     * of memory outside OP-TEE shared memory hence using OPTEE_SMT module.
     */
    [SCMI_AGENT_ID_NSEC1] = MAILBOX_TYPE_OPTEE_SMT,
};

static void set_scmi_comm_resources(struct scpfw_config *cfg)
{
    unsigned int channel_index = 0;
    unsigned int __maybe_unused msg_smt_index = 0;
    unsigned int __maybe_unused optee_smt_index = 0;
    size_t scmi_agent_count = cfg->agent_count;

    /*
     * @cfg does not consider agent #0,
     * that is the reserved platform/server agent.
     */
    scmi_agent_table =
        fwk_mm_calloc(scmi_agent_count + 1, sizeof(*scmi_agent_table));

    scmi_service_elt = fwk_mm_calloc(
        scpfw_resource_counter.channel_count + 1, sizeof(*scmi_service_elt));

#ifdef CFG_SCPFW_MOD_MSG_SMT
    msg_smt_elt = fwk_mm_calloc(
        scpfw_resource_counter.channel_count + 1, sizeof(*msg_smt_elt));
    msg_smt_data = fwk_mm_calloc(
        scpfw_resource_counter.channel_count, sizeof(*msg_smt_data));
#endif

#ifdef CFG_SCPFW_MOD_OPTEE_SMT
    optee_smt_elt = fwk_mm_calloc(scpfw_resource_counter.channel_count + 1,
                                  sizeof(*optee_smt_elt));
    optee_smt_data = fwk_mm_calloc(scpfw_resource_counter.channel_count,
                                   sizeof(*optee_smt_data));
#endif

    optee_mbx_elt = fwk_mm_calloc(
        scpfw_resource_counter.channel_count + 1, sizeof(*optee_mbx_elt));
    optee_mbx_data = fwk_mm_calloc(
        scpfw_resource_counter.channel_count, sizeof(*optee_mbx_data));

    /* Set now the uniqnue scmi module instance configuration data */
    scmi_data = (struct mod_scmi_config){
        .agent_table = scmi_agent_table,
        .agent_count = scmi_agent_count,
        .protocol_count_max = 9,
        .vendor_identifier = "ST",
        .sub_vendor_identifier = "ST",
    };

    for (size_t i = 0; i < cfg->agent_count; i++) {
        struct scpfw_agent_config *agent_cfg = cfg->agent_config + i;
        size_t agent_index = agent_cfg->agent_id;

        scmi_agent_table[agent_index].type = SCMI_AGENT_TYPE_OSPM;
        scmi_agent_table[agent_index].name = agent_cfg->name;

        for (size_t j = 0; j < agent_cfg->channel_count; j++) {
            struct scpfw_channel_config *channel_cfg =
                agent_cfg->channel_config + j;
            struct mod_scmi_service_config *service_data;

            service_data = fwk_mm_calloc(1, sizeof(*service_data));

            scmi_service_elt[channel_index].name = channel_cfg->name;
            scmi_service_elt[channel_index].data = service_data;

            optee_mbx_elt[channel_index].name = channel_cfg->name;
            optee_mbx_elt[channel_index].data =
                (void *)(optee_mbx_data + channel_index);

            assert(agent_index < SCMI_AGENT_ID_COUNT);

            /* Make sure mailbox_idx stored in OPTEE is the same than SCP */
#if defined(CFG_SCPFW_MOD_OPTEE_SMT)
            assert(channel_index == channel_cfg->mailbox_idx);
#endif

            switch (module_idx_smt_table[agent_index]) {
#if defined(CFG_SCPFW_MOD_MSG_SMT)
            case MAILBOX_TYPE_MSG_SMT:
                *service_data = (struct mod_scmi_service_config){
                    .transport_id = (fwk_id_t)FWK_ID_ELEMENT_INIT(
                        FWK_MODULE_IDX_MSG_SMT, msg_smt_index),
                    .transport_api_id = (fwk_id_t)FWK_ID_API_INIT(
                        FWK_MODULE_IDX_MSG_SMT,
                        MOD_MSG_SMT_API_IDX_SCMI_TRANSPORT),
                    .scmi_agent_id = agent_cfg->agent_id,
                    .scmi_p2a_id = FWK_ID_NONE_INIT,
                };

                msg_smt_elt[msg_smt_index].name = channel_cfg->name;
                msg_smt_elt[msg_smt_index].data =
                    (void *)(msg_smt_data + msg_smt_index);

                msg_smt_data[msg_smt_index] =
                    (struct mod_msg_smt_channel_config){
                        .type = MOD_MSG_SMT_CHANNEL_TYPE_REQUESTER,
                        .mailbox_size = SCMI_SHMEM_SIZE,
                        .driver_id = (fwk_id_t)FWK_ID_ELEMENT_INIT(
                            FWK_MODULE_IDX_OPTEE_MBX, channel_index),
                        .driver_api_id = (fwk_id_t)FWK_ID_API_INIT(
                            FWK_MODULE_IDX_OPTEE_MBX, 0),
                    };

                optee_mbx_data[channel_index] =
                    (struct mod_optee_mbx_channel_config){
                        .driver_id = (fwk_id_t)FWK_ID_ELEMENT_INIT(
                            FWK_MODULE_IDX_MSG_SMT, msg_smt_index),
                        .driver_api_id = (fwk_id_t)FWK_ID_API_INIT(
                            FWK_MODULE_IDX_MSG_SMT,
                            MOD_MSG_SMT_API_IDX_DRIVER_INPUT),
                    };
                msg_smt_index++;
                break;
#endif
#if defined(CFG_SCPFW_MOD_OPTEE_SMT)
            case MAILBOX_TYPE_OPTEE_SMT:
                *service_data = (struct mod_scmi_service_config){
                    .transport_id = (fwk_id_t)
                        FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_OPTEE_SMT,
                                            optee_smt_index),
                    .transport_api_id = (fwk_id_t)
                        FWK_ID_API_INIT(FWK_MODULE_IDX_OPTEE_SMT,
                                        MOD_OPTEE_SMT_API_IDX_SCMI_TRANSPORT),
                    .scmi_agent_id = agent_cfg->agent_id,
                    .scmi_p2a_id = FWK_ID_NONE_INIT,
                };

                optee_smt_elt[optee_smt_index].name = channel_cfg->name;
                optee_smt_elt[optee_smt_index].data =
                    (void *)(optee_smt_data + optee_smt_index);

                assert(channel_cfg->shm.pa != 0);
                assert(channel_cfg->shm.size != 0);

                optee_smt_data[optee_smt_index] =
                    (struct mod_optee_smt_channel_config){
                    .type = MOD_OPTEE_SMT_CHANNEL_TYPE_REQUESTER,
                    .policies = MOD_SMT_POLICY_NONE,
                    .mailbox_address = channel_cfg->shm.pa,
                    .mailbox_size = SCMI_SHMEM_SIZE,
                    .driver_id = (fwk_id_t)
                        FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_OPTEE_MBX,
                                            channel_index),
                    .driver_api_id = (fwk_id_t)
                        FWK_ID_API_INIT(FWK_MODULE_IDX_OPTEE_MBX, 0),
                };

                optee_mbx_data[channel_index] =
                    (struct mod_optee_mbx_channel_config){
                    .driver_id =
                        (fwk_id_t)FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_OPTEE_SMT,
                                                      optee_smt_index),
                    .driver_api_id =
                        (fwk_id_t)FWK_ID_API_INIT(FWK_MODULE_IDX_OPTEE_SMT,
                            MOD_OPTEE_SMT_API_IDX_DRIVER_INPUT),
                };
                optee_smt_index++;
                break;
#endif
            default:
                panic("Incorrect SMT module_idx");
            }

            channel_index++;
        }
    }
};

static void set_resources(struct scpfw_config *cfg)
{
    for (size_t i = 0; i < cfg->agent_count; i++) {
        struct scpfw_agent_config *agent_cfg = cfg->agent_config + i;
        size_t agent_index = i + 1;

        if (agent_index != agent_cfg->agent_id) {
            panic("scpfw config expects agent ID is agent index");
        }

        for (size_t j = 0; j < agent_cfg->channel_count; j++) {
            struct scpfw_channel_config *channel_cfg =
                agent_cfg->channel_config + j;

#ifdef CFG_SCPFW_MOD_SCMI_CLOCK
            /*
             * Add first SCMI clock. We will add later the clocks used for DVFS
             */
            if (channel_cfg->clock_count) {
                size_t clock_index = scpfw_resource_counter.clock_index;
                size_t optee_clock_index = scpfw_resource_counter.optee_clock_index;
                struct mod_scmi_clock_device *dev = NULL;

                /* Set SCMI clocks array for the SCMI agent */
                dev = fwk_mm_calloc(
                    channel_cfg->clock_count,
                    sizeof(struct mod_scmi_clock_device));

                fwk_assert(!scmi_clk_agent_cfg[agent_index].device_table);
                scmi_clk_agent_cfg[agent_index] =
                    (struct mod_scmi_clock_agent_config){
                        .agent_device_count = channel_cfg->clock_count,
                        .device_table = dev,
                    };

                fwk_assert(!scmi_clock_elt[agent_index].data);
                scmi_clock_elt[agent_index] = (struct fwk_element){
                    .name = agent_cfg->name,
                    .data = scmi_clk_agent_cfg + agent_index,
                };

                /* Set clock and optee/clock elements and config data */
                for (size_t k = 0; k < channel_cfg->clock_count; k++) {
                    struct scmi_clock *clock_cfg = channel_cfg->clock + k;
                    struct mod_clock_dev_config cdata = {
                        .driver_id = (fwk_id_t)FWK_ID_ELEMENT_INIT(
                            FWK_MODULE_IDX_OPTEE_CLOCK, optee_clock_index),
                        .api_id = (fwk_id_t)FWK_ID_API_INIT(
                            FWK_MODULE_IDX_OPTEE_CLOCK, 0),
                        .pd_source_id = FWK_ID_NONE,
                    };

                    dev[k].element_id = (fwk_id_t)FWK_ID_ELEMENT_INIT(
                        FWK_MODULE_IDX_CLOCK, clock_index);

                    optee_clock_cfg[optee_clock_index].clk = clock_cfg->clk;
                    optee_clock_cfg[optee_clock_index].default_enabled =
                        clock_cfg->enabled;

                    optee_clock_elt[optee_clock_index].name = clock_cfg->name;
                    optee_clock_elt[optee_clock_index].data =
                        (void *)(optee_clock_cfg + optee_clock_index);

                    memcpy(clock_data + clock_index, &cdata, sizeof(cdata));

                    clock_elt[clock_index].name = clock_cfg->name;
                    clock_elt[clock_index].data =
                        (void *)(clock_data + clock_index);

                    clock_index++;
                    optee_clock_index++;
                }

                scpfw_resource_counter.clock_index = clock_index;
                scpfw_resource_counter.optee_clock_index = optee_clock_index;
            }
#endif

#ifdef CFG_SCPFW_MOD_RESET_DOMAIN
            if (channel_cfg->reset_count) {
                struct mod_scmi_reset_domain_device *dev = NULL;
                size_t reset_index = scpfw_resource_counter.reset_index;

                /* Set SCMI reset domains array for the SCMI agent */
                dev = fwk_mm_calloc(channel_cfg->reset_count, sizeof(*dev));

                fwk_assert(!scmi_reset_agent_tbl[agent_index].device_table);
                scmi_reset_agent_tbl[agent_index].agent_domain_count =
                    channel_cfg->reset_count;
                scmi_reset_agent_tbl[agent_index].device_table = dev;

                /* Set reset_domain and optee/reset elements and config data */
                for (size_t k = 0; k < channel_cfg->reset_count; k++) {
                    struct scmi_reset *reset_cfg = channel_cfg->reset + k;

                    dev[k].element_id = (fwk_id_t)FWK_ID_ELEMENT_INIT(
                        FWK_MODULE_IDX_RESET_DOMAIN, reset_index);

                    optee_reset_data[reset_index].rstctrl = reset_cfg->rstctrl;

                    optee_reset_elt[reset_index].name = reset_cfg->name;
                    optee_reset_elt[reset_index].data =
                        (void *)(optee_reset_data + reset_index);

                    reset_data[reset_index] =
                        (struct mod_reset_domain_dev_config){
                            .driver_id = (fwk_id_t)FWK_ID_ELEMENT_INIT(
                                FWK_MODULE_IDX_OPTEE_RESET, reset_index),
                            .driver_api_id = (fwk_id_t)FWK_ID_API_INIT(
                                FWK_MODULE_IDX_OPTEE_RESET, 0),
                            .modes = MOD_RESET_DOMAIN_AUTO_RESET |
                                MOD_RESET_DOMAIN_MODE_EXPLICIT_ASSERT |
                                MOD_RESET_DOMAIN_MODE_EXPLICIT_DEASSERT,
                        };

                    reset_elt[reset_index].name = reset_cfg->name;
                    reset_elt[reset_index].data =
                        (void *)(reset_data + reset_index);

                    reset_index++;
                }

                scpfw_resource_counter.reset_index = reset_index;
            }
#endif

#ifdef CFG_SCPFW_MOD_VOLTAGE_DOMAIN
            if (channel_cfg->voltd_count) {
                size_t regu_index = scpfw_resource_counter.regu_index;
                struct mod_scmi_voltd_device *dev = NULL;

                /* Set SCMI voltage domains array for the SCMI agent */
                dev = fwk_mm_calloc(channel_cfg->voltd_count,
                                    sizeof(struct mod_scmi_voltd_device));

                fwk_assert(!scmi_voltd_agent_tbl[agent_index].device_table);
                scmi_voltd_agent_tbl[agent_index].domain_count = channel_cfg->voltd_count;
                scmi_voltd_agent_tbl[agent_index].device_table = dev;

                /* Set voltage_domain and optee/regu elements and config data */
                for (size_t k = 0; k < channel_cfg->voltd_count; k++) {
                    struct scmi_voltd *voltd_cfg = channel_cfg->voltd + k;
                    static const char reserved[] = "reserved";
                    const char *name = NULL;
                    struct mod_voltd_dev_config vdata = {
                        .driver_id =
                            (fwk_id_t)FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_OPTEE_VOLTD_REGULATOR,
                                                          regu_index),
                        .api_id = (fwk_id_t)FWK_ID_API_INIT(FWK_MODULE_IDX_OPTEE_VOLTD_REGULATOR,
                                                            0)
                    };

                    if (voltd_cfg->regulator) {
                        name = regulator_name(voltd_cfg->regulator);
                    } else {
                        name = reserved;
                    }

                    dev[k].element_id =
                       (fwk_id_t)FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_VOLTAGE_DOMAIN, regu_index);

                    optee_regu_elt[regu_index].name = name;
                    optee_regu_elt[regu_index].data = (void *)(optee_regu_data + regu_index);
                    optee_regu_data[regu_index].regulator = (struct optee_regulator *)voltd_cfg->regulator;
                    optee_regu_data[regu_index].default_enabled = voltd_cfg->enabled;

                    memcpy(voltd_data + regu_index, &vdata, sizeof(vdata));

                    voltd_elt[regu_index].name = name;
                    voltd_elt[regu_index].data = (void *)(voltd_data + regu_index);
                    regu_index++;
                }

                scpfw_resource_counter.regu_index = regu_index;
            }
#endif

#ifdef CFG_SCPFW_MOD_DVFS
            if (channel_cfg->perfd_count) {
                size_t clock_index = scpfw_resource_counter.clock_index;
                size_t psu_index = scpfw_resource_counter.psu_index;
                size_t dvfs_index = scpfw_resource_counter.dvfs_index;

                for (size_t k = 0; k < channel_cfg->perfd_count; k++) {
                    struct scmi_perfd *perfd_cfg = channel_cfg->perfd + k;
                    struct mod_clock_dev_config cdata = {
                        .driver_id = (fwk_id_t)FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_STM32_OPP,
                                                                   dvfs_index),
                        .api_id = (fwk_id_t)FWK_ID_API_INIT(FWK_MODULE_IDX_STM32_OPP,
                                                            0),
                        .pd_source_id = FWK_ID_NONE,
                    };

                    /* dvfs instances: 1 instance per expose DVFS service */
                    dvfs_data[dvfs_index].psu_id =
                        (fwk_id_t)FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_PSU, psu_index);
                    dvfs_data[dvfs_index].clock_id =
                        (fwk_id_t)FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_CLOCK, clock_index);
                    dvfs_data[dvfs_index].latency = 1000;
                    dvfs_data[dvfs_index].sustained_idx = perfd_cfg->initial_opp;
                    dvfs_data[dvfs_index].opps =
                        fwk_mm_calloc(perfd_cfg->dvfs_opp_count,
                                      sizeof(struct mod_dvfs_opp));

                    for (size_t l = 0; l < perfd_cfg->dvfs_opp_count; l++) {
                        uint64_t power = 0;

                        /* Quick estimation of power consumpsion */
                        power = perfd_cfg->dvfs_opp_khz[l];
                        power *= perfd_cfg->dvfs_opp_mv[l];
                        power *= perfd_cfg->dvfs_opp_mv[l];
                        power /= 100 * 1000 * 1000;

                        dvfs_data[dvfs_index].opps[l] = (struct mod_dvfs_opp){
                            .level = perfd_cfg->dvfs_opp_khz[l] * 1000UL,
                            .frequency = perfd_cfg->dvfs_opp_khz[l],
                            .voltage = perfd_cfg->dvfs_opp_mv[l],
                            .power = (uint32_t)power,
                        };
                    }

                    dvfs_elt[dvfs_index].name = perfd_cfg->name;
                    dvfs_elt[dvfs_index].data = (void *)(dvfs_data + dvfs_index);

                    /* Module psu_optee module (elements and  configuration data) */
                    psu_optee_regu_elt[psu_index].name = perfd_cfg->name;
                    psu_optee_regu_elt[psu_index].data = (void *)(psu_optee_regu_data + psu_index);

                    /* Voltage is managed in OPP driver: set a PSU without regulator */
                    psu_optee_regu_data[psu_index].regulator = NULL;

                    /* Module psu (elements and configuration data) */
                    psu_elt[psu_index].name = perfd_cfg->name;
                    psu_elt[psu_index].data = (void *)(psu_data + psu_index);
                    psu_data[psu_index].driver_id =
                        (fwk_id_t)FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_PSU_OPTEE_REGULATOR, psu_index);
                    psu_data[psu_index].driver_api_id =
                        (fwk_id_t)FWK_ID_API_INIT(FWK_MODULE_IDX_PSU_OPTEE_REGULATOR, 0);

                    /* Module clock and associated STM32 OPP_clock driver */
                    fwk_assert(!clock_elt[clock_index].data);
                    clock_elt[clock_index].name = perfd_cfg->name;
                    clock_elt[clock_index].data = (void *)(clock_data + clock_index);

                    memcpy(clock_data + clock_index, &cdata, sizeof(cdata));

                    stm32_opp_cfg[dvfs_index].opp_id = perfd_cfg->opp_id;

                    stm32_opp_elt[dvfs_index].name = perfd_cfg->name;
                    stm32_opp_elt[dvfs_index].data = (void *)(stm32_opp_cfg + dvfs_index);

                    clock_index++;
                    psu_index++;
                    dvfs_index++;
                }

                scpfw_resource_counter.clock_index = clock_index;
                scpfw_resource_counter.psu_index = psu_index;
                scpfw_resource_counter.dvfs_index = dvfs_index;
            }
#endif

#ifdef CFG_SCPFW_MOD_POWER_DOMAIN
            if (channel_cfg->pd_count) {
                /* System domain */
                size_t system_index = channel_cfg->pd_count; /* Last element */
                struct fwk_element *pd_elt_sys = pd_elt + system_index;
                struct mod_power_domain_element_config *pd_elt_data_sys =
                    fwk_mm_calloc(1, sizeof(*pd_elt_data_sys));
                struct fwk_element *stm32_pd_elt_sys = stm32_pd_elt + system_index;

                /* Data are unused but framework doesn't accept NULL data */
                stm32_pd_elt_sys->name = "system";
                stm32_pd_elt_sys->data = (void *)1;

                pd_elt_sys->name = stm32_pd_elt_sys->name;

                pd_elt_data_sys->parent_idx = PD_STATIC_DEV_IDX_NONE;
                pd_elt_data_sys->driver_id =
                    (fwk_id_t)FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_STM32_PD, system_index);
                pd_elt_data_sys->api_id =
                    (fwk_id_t)FWK_ID_API_INIT(FWK_MODULE_IDX_STM32_PD, 0);
                pd_elt_data_sys->attributes.pd_type = MOD_PD_TYPE_DEVICE;
                pd_elt_data_sys->allowed_state_mask_table =
                    pd_allowed_state_mask_table;
                pd_elt_data_sys->allowed_state_mask_table_size =
                    FWK_ARRAY_SIZE(pd_allowed_state_mask_table);
                pd_elt_sys->data = pd_elt_data_sys;

                /* Power domains */
                for (size_t k = 0; k < channel_cfg->pd_count; k++) {
                    struct fwk_element *pd_elt_k = pd_elt + k;
                    struct mod_power_domain_element_config *pd_elt_data_k =
                        fwk_mm_calloc(1, sizeof(*pd_elt_data_k));
                    struct fwk_element *stm32_pd_elt_k =
                        stm32_pd_elt + k;
                    struct scmi_pd *scmi_pd = channel_cfg->pd + k;
                    struct mod_stm32_pd_config *mod_stm32_pd_config =
                        fwk_mm_calloc(1, sizeof(*mod_stm32_pd_config));

                    mod_stm32_pd_config->name = scmi_pd->name;
                    mod_stm32_pd_config->clk = scmi_pd->clk;
                    mod_stm32_pd_config->regu = scmi_pd->regu;

                    stm32_pd_elt_k->name = scmi_pd->name;
                    stm32_pd_elt_k->data = (void *)mod_stm32_pd_config;

                    pd_elt_k->name = scmi_pd->name;

                    pd_elt_data_k->parent_idx = system_index;
                    pd_elt_data_k->driver_id =
                        (fwk_id_t)FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_STM32_PD, k);
                    pd_elt_data_k->api_id =
                        (fwk_id_t)FWK_ID_API_INIT(FWK_MODULE_IDX_STM32_PD, 0);
                    pd_elt_data_k->attributes.pd_type = MOD_PD_TYPE_DEVICE;
                    pd_elt_data_k->allowed_state_mask_table =
                        pd_allowed_state_mask_table;
                    pd_elt_data_k->allowed_state_mask_table_size =
                        FWK_ARRAY_SIZE(pd_allowed_state_mask_table);
                    pd_elt_k->data = pd_elt_data_k;
                }
            }
#endif
        }
    }
}

int scmi_configure(struct scpfw_config *cfg)
{
    count_resources(cfg);
    allocate_global_resources(cfg);
    set_scmi_comm_resources(cfg);
    set_resources(cfg);

    return 0;
}
