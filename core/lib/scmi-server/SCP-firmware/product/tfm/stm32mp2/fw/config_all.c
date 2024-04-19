/*
 * Arm SCP/MCP Software
 * Copyright (c) 2022-2023, Linaro Limited and Contributors. All rights reserved.
 * Copyright (c) 2024, STMicroelectronics and the Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <scmi_agents.h>

#include <mod_clock.h>
#include <mod_tfm_mbx.h>
#include <mod_scmi.h>

#include <fwk_assert.h>
#include <fwk_element.h>
#include <fwk_id.h>
#include <fwk_log.h>
#include <fwk_mm.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>

#include <mod_clock.h>
#include <mod_msg_smt.h>
#include <mod_tfm_clock.h>
#include <mod_tfm_mbx.h>
#ifdef CFG_SCPFW_MOD_TFM_SMT
#include <mod_tfm_smt.h>
#endif
#include <mod_tfm_reset.h>
#include <mod_reset_domain.h>
#include <mod_scmi.h>
#include <mod_scmi_clock.h>
#include <mod_scmi_voltage_domain.h>
#include <mod_tfm_regu_consumer.h>
#include <mod_scmi_reset_domain.h>
#include <mod_power_domain.h>
#include <mod_scmi_system_power.h>
#include <mod_system_power.h>
#include <mod_stm32_pd.h>

#ifdef CFG_SCPFW_MOD_PSU
#    include <mod_psu.h>
#endif
#ifdef CFG_SCPFW_MOD_RESET_DOMAIN
#    include <mod_reset_domain.h>
#endif
#ifdef CFG_SCPFW_MOD_SCMI_CLOCK
#    include <mod_scmi_clock.h>
#endif
#ifdef CFG_SCPFW_MOD_SCMI_PERF
#    include <mod_scmi_perf.h>
#endif
#ifdef CFG_SCPFW_MOD_SCMI_POWER_DOMAIN
#    include <mod_scmi_power_domain.h>
#endif
#ifdef CFG_SCPFW_MOD_SCMI_RESET_DOMAIN
#    include <mod_scmi_reset_domain.h>
#endif
#ifdef CFG_SCPFW_MOD_SCMI_VOLTAGE_DOMAIN
#    include <mod_scmi_voltage_domain.h>
#endif
#ifdef CFG_SCPFW_MOD_STM32_OPP
#    include <mod_stm32_opp.h>
#endif
#ifdef CFG_SCPFW_MOD_VOLTAGE_DOMAIN
#    include <mod_voltage_domain.h>
#endif

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <arch_helpers.h>
#include <scmi_agent_configuration.h>

/* SCMI agent and services (channels) */
static struct mod_scmi_agent *scmi_agent_table;
static struct mod_scmi_config scmi_data;
static struct fwk_element *scmi_service_elt;
static struct mod_scmi_service_config *scmi_service_data;

/* SCMI channel mailbox/shmem */
#ifdef CFG_SCPFW_MOD_MSG_SMT
static struct fwk_element *msg_smt_elt;
static struct mod_msg_smt_channel_config *msg_smt_data;
#endif
static struct fwk_element *tfm_mbx_elt;
static struct mod_tfm_mbx_channel_config *tfm_mbx_data;
#ifdef CFG_SCPFW_MOD_TFM_SMT
static struct fwk_element *tfm_smt_elt;
static struct mod_tfm_smt_channel_config *tfm_smt_data;
#endif

#ifdef CFG_SCPFW_MOD_SCMI_CLOCK
/* SCMI clock generic */
static struct fwk_element *scmi_clock_elt;
static struct mod_scmi_clock_agent_config *scmi_clk_agent_cfg;
#endif

#ifdef CFG_SCPFW_MOD_CLOCK
/* Clocks and tfm/clock, same number/indices. Elements and configuration data */
static struct fwk_element *tfm_clock_elt;               /* tfm/clock elements */
static struct mod_tfm_clock_config *tfm_clock_cfg;      /* Config data for tfm/clock elements */
static struct fwk_element *clock_elt;                   /* Clock elements */
static struct mod_clock_dev_config *clock_data;         /* Config data for clock elements */
#endif

#ifdef CFG_SCPFW_MOD_RESET_DOMAIN
/* SCMI reset domains and tfm reset controller */
static struct mod_scmi_reset_domain_agent *scmi_reset_agent_tbl;
static struct fwk_element *tfm_reset_elt;
static struct mod_tfm_reset_dev_config *tfm_reset_data;
static struct fwk_element *reset_elt;
static struct mod_reset_domain_dev_config *reset_data;
#endif

#ifdef CFG_SCPFW_MOD_VOLTAGE_DOMAIN
/* SCMI voltage domains and tfm regulators */
static struct mod_scmi_voltd_agent *scmi_voltd_agent_tbl;
static struct fwk_element *tfm_regu_elt;
static struct mod_tfm_regu_consumer_dev_config *tfm_regu_cfg;
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

/* Config data for tfm_mbx module */
static const struct fwk_element *tfm_mbx_get_element_table(fwk_id_t module_id)
{
    return (const struct fwk_element *)tfm_mbx_elt;
}

struct fwk_module_config config_tfm_mbx = {
    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(tfm_mbx_get_element_table),
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

#ifdef CFG_SCPFW_MOD_TFM_SMT
/* Config data for tfm_smt module */
static const struct fwk_element *tfm_smt_get_element_table(fwk_id_t module_id)
{
    fwk_assert(fwk_id_get_module_idx(module_id) == FWK_MODULE_IDX_TFM_SMT);
    return (const struct fwk_element *)tfm_smt_elt;
};

struct fwk_module_config config_tfm_smt = {
    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(tfm_smt_get_element_table),
};
#endif

/* Config data for scmi_clock, clock and tfm_clock modules */
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
    .data = &((struct mod_clock_config){
       .pd_transition_notification_id = FWK_ID_NONE_INIT,
       .pd_pre_transition_notification_id = FWK_ID_NONE_INIT
    }),
};

static const struct fwk_element *tfm_clock_get_element_table(fwk_id_t module_id)
{
    fwk_assert(fwk_id_get_module_idx(module_id) == FWK_MODULE_IDX_TFM_CLOCK);
    return (const struct fwk_element *)tfm_clock_elt;
}

struct fwk_module_config config_tfm_clock = {
    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(tfm_clock_get_element_table),
};
#endif

/* Config data for scmi_reset_domain, reset_domain and tfm_reset modules */
#ifdef CFG_SCPFW_MOD_RESET_DOMAIN
struct fwk_module_config config_scmi_reset_domain = {
    .data = &((struct mod_scmi_reset_domain_config){
        .agent_table = NULL,                    /* Allocated during initialization */
        .agent_count = 0,                       /* Set during initialization */
    }),
};

static const struct fwk_element *reset_get_element_table(fwk_id_t module_id)
{
    fwk_assert(fwk_id_get_module_idx(module_id) == FWK_MODULE_IDX_RESET_DOMAIN);
    return (const struct fwk_element *)reset_elt;
}

struct fwk_module_config config_reset_domain = {
    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(reset_get_element_table),
    .data = &((struct mod_reset_domain_config){
        .notification_id =
        FWK_ID_NOTIFICATION_INIT(FWK_MODULE_IDX_RESET_DOMAIN,
                                 MOD_RESET_DOMAIN_NOTIFICATION_AUTORESET)
    }),
};

static const struct fwk_element *tfm_reset_get_element_table(fwk_id_t module_id)
{
    fwk_assert(fwk_id_get_module_idx(module_id) == FWK_MODULE_IDX_TFM_RESET);
    return (const struct fwk_element *)tfm_reset_elt;
}

struct fwk_module_config config_tfm_reset = {
    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(tfm_reset_get_element_table),
};
#endif

#ifdef CFG_SCPFW_MOD_VOLTAGE_DOMAIN
/* Config data for scmi_voltage_domain, voltage_domain and tfm_regu modules */
struct fwk_module_config config_scmi_voltage_domain = {
    .data = &((struct mod_scmi_voltd_config){
        .agent_table = NULL,                    /* Allocated during initialization */
        .agent_count = 0,                       /* Set during initialization */
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

static const struct fwk_element *tfm_regu_consumer_get_element_table(fwk_id_t module_id)
{
    fwk_assert(fwk_id_get_module_idx(module_id) == FWK_MODULE_IDX_TFM_REGU_CONSUMER);
    return (const struct fwk_element *)tfm_regu_elt;
}

struct fwk_module_config config_tfm_regu_consumer = {
    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(tfm_regu_consumer_get_element_table),
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

static const struct fwk_element *pd_get_element_table(fwk_id_t module_id)
{
    return (const struct fwk_element *)pd_elt;
}

struct fwk_module_config config_power_domain = {
    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(pd_get_element_table),
    .data = &((struct mod_power_domain_config){0}),
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

#ifdef CFG_SCPFW_MOD_SCMI_SYSTEM_POWER
const struct fwk_module_config config_scmi_system_power = {
    .data = &((struct mod_scmi_system_power_config){
        .system_view = MOD_SCMI_SYSTEM_VIEW_FULL,
        .system_suspend_state = MOD_SYSTEM_POWER_POWER_STATE_SLEEP0,
        .alarm_id = FWK_ID_NONE_INIT
    }),
};
#endif

/*
 * Indices state when applying agents configuration
 * @channel_count: Number of channels (mailbox/shmem links) used
 * @clock_index: Current index for clock and tfm/clock (same indices)
 * @clock_count: Number of clocks (also number of tfm/clocks)
 * @reset_index: Current index for reset controller and tfm/reset
 * @reset_count: Number of reset controller (tfm/reset) instances
 * @regu_index: Current index for voltd and tfm/regulator
 * @regu_count: Number of voltd (tfm/regulator) instances
 */
struct scpfw_resource_counter {
    size_t channel_count;
    size_t clock_index;
    size_t clock_count;
    size_t reset_index;
    size_t reset_count;
    size_t regu_index;
    size_t regu_count;
    size_t pd_index;
    size_t pd_count;
} scpfw_resource_counter;

/*
 * Count once for all the several instances and allocate global resources.
 * Global resources are clock, tfm/clock, reset, tfm/reset, regu,
 * tfm/regu, psu, tfm/psu, dvfs, perfd, ...;
 */
static void count_resources(struct scpfw_config *cfg)
{
    size_t i, j;

    for (i = 0; i < cfg->agent_count; i++) {
        struct scpfw_agent_config *agent_cfg = cfg->agent_config + i;

        scpfw_resource_counter.channel_count += agent_cfg->channel_count;

#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
        /* Only Ns agent has async notification */
        if (agent_cfg->agent_id == STM32MP25_AGENT_ID_M33_NS)
            scpfw_resource_counter.channel_count += 1;
#endif
        for (j = 0; j < agent_cfg->channel_count; j++) {
            struct scpfw_channel_config *channel_cfg = agent_cfg->channel_config + j;

            /* Clocks for scmi_clock */
            scpfw_resource_counter.clock_count += channel_cfg->clock_count;
            /* Reset for smci_reset only */
            scpfw_resource_counter.reset_count += channel_cfg->reset_count;
            /* Regulators for smci_voltage_domain only */
            scpfw_resource_counter.regu_count += channel_cfg->voltd_count;
            /* Power domains */
            scpfw_resource_counter.pd_count += channel_cfg->pd_count;
        }
    }

#ifndef CFG_SCPFW_MOD_CLOCK
    fwk_assert(!scpfw_resource_counter.clock_count);
#endif
#ifndef CFG_SCPFW_MOD_RESET_DOMAIN
    fwk_assert(!scpfw_resource_counter.reset_count);
#endif
#ifndef CFG_SCPFW_MOD_VOLTAGE_DOMAIN
    fwk_assert(!scpfw_resource_counter.regu_count);
#endif
#ifndef CFG_SCPFW_MOD_POWER_DOMAIN
    fwk_assert(!scpfw_resource_counter.pd_count);
#endif
}

/*
 * Allocate all tables that may be needed. An optimized implementation would
 * allocate a single piece of memory and set the pointers accordingly.
 * */
static void allocate_global_resources(struct scpfw_config *cfg)
{
    struct mod_scmi_reset_domain_config *scmi_reset_config __maybe_unused;
    struct mod_scmi_voltd_config *scmi_voltd_config __maybe_unused;
    /* @cfg does not consider agent #0: ID reserved for the platform/server */
    size_t __maybe_unused scmi_agent_count = cfg->agent_count + 1;

#ifdef CFG_SCPFW_MOD_SCMI_CLOCK
    /* SCMI clock domains resources */
    scmi_clock_elt =
        fwk_mm_calloc(scmi_agent_count + 1, sizeof(*scmi_clock_elt));
    scmi_clk_agent_cfg =
        fwk_mm_calloc(scmi_agent_count, sizeof(*scmi_clk_agent_cfg));

    scmi_clock_elt[0] = (struct fwk_element){
        .name = "",
        .data = scmi_clk_agent_cfg + 0,
    };

    /*
     * Pre-initialize all agent slots with a non-NULL name so they don't act as
     * a NULL sentinel for the framework element counter. Agents without clocks
     * keep device_table=NULL (empty config from calloc) which is handled
     * gracefully by scmi_clock_element_init (returns FWK_SUCCESS immediately).
     */
    for (size_t k = 1; k < (size_t)scmi_agent_count; k++) {
        if (!scmi_clock_elt[k].name)
            scmi_clock_elt[k].name = "";
        scmi_clock_elt[k].data = scmi_clk_agent_cfg + k;
    }
#endif

#ifdef CFG_SCPFW_MOD_CLOCK
    /* Clock domains resources */
    tfm_clock_cfg = fwk_mm_calloc(scpfw_resource_counter.clock_count,
                                    sizeof(*tfm_clock_cfg));
    tfm_clock_elt = fwk_mm_calloc(scpfw_resource_counter.clock_count + 1,
                                    sizeof(*tfm_clock_elt));

    clock_data = fwk_mm_calloc(scpfw_resource_counter.clock_count,
                               sizeof(*clock_data));
    clock_elt = fwk_mm_calloc(scpfw_resource_counter.clock_count + 1,
                              sizeof(*clock_elt));
#endif

#ifdef CFG_SCPFW_MOD_RESET_DOMAIN
    /* SCMI reset domains resources */
    scmi_reset_agent_tbl = fwk_mm_calloc(scmi_agent_count,
                                         sizeof(*scmi_reset_agent_tbl));
    scmi_reset_config = (void *)config_scmi_reset_domain.data;
    scmi_reset_config->agent_table = scmi_reset_agent_tbl;
    scmi_reset_config->agent_count = scmi_agent_count;

    tfm_reset_data = fwk_mm_calloc(scpfw_resource_counter.reset_count,
                                     sizeof(*tfm_reset_data));
    tfm_reset_elt = fwk_mm_calloc(scpfw_resource_counter.reset_count + 1,
                                    sizeof(*tfm_reset_elt));

    reset_data = fwk_mm_calloc(scpfw_resource_counter.reset_count,
                               sizeof(*reset_data));
    reset_elt = fwk_mm_calloc(scpfw_resource_counter.reset_count + 1,
                              sizeof(*reset_elt));
#endif

#ifdef CFG_SCPFW_MOD_VOLTAGE_DOMAIN
    /* SCMI voltage domains resources */
    scmi_voltd_agent_tbl = fwk_mm_calloc(scmi_agent_count,
                                         sizeof(*scmi_voltd_agent_tbl));
    scmi_voltd_config = (void *)config_scmi_voltage_domain.data;
    scmi_voltd_config->agent_table = scmi_voltd_agent_tbl;
    scmi_voltd_config->agent_count = scmi_agent_count;

    tfm_regu_cfg = fwk_mm_calloc(scpfw_resource_counter.regu_count,
                                    sizeof(*tfm_regu_cfg));
    tfm_regu_elt = fwk_mm_calloc(scpfw_resource_counter.regu_count + 1,
                                    sizeof(*tfm_regu_elt));

    voltd_data = fwk_mm_calloc(scpfw_resource_counter.regu_count,
                               sizeof(*voltd_data));
    voltd_elt = fwk_mm_calloc(scpfw_resource_counter.regu_count + 1,
                              sizeof(*voltd_elt));
#endif

#ifdef CFG_SCPFW_MOD_POWER_DOMAIN
    /* <pd_count> power domains + 1 system domain + 1 empty cell */
    pd_elt = fwk_mm_calloc(scpfw_resource_counter.pd_count + 2,
                              sizeof(*pd_elt));
    stm32_pd_elt = fwk_mm_calloc(scpfw_resource_counter.pd_count + 2,
                              sizeof(*stm32_pd_elt));
#endif
}

const static int agent_type[] = {
    [STM32MP25_AGENT_ID_M33_NS] = SCMI_AGENT_TYPE_MANAGEMENT,
    [STM32MP25_AGENT_ID_CA35] = SCMI_AGENT_TYPE_OSPM,
    [STM32MP25_AGENT_ID_CA35_BL31] = SCMI_AGENT_TYPE_PSCI
};

static void set_scmi_comm_resources(struct scpfw_config *cfg)
{
    unsigned int channel_index;
    unsigned int __maybe_unused msg_smt_index = 0, tfm_smt_index = 0;
    size_t i, j;
    unsigned int  __maybe_unused p2a_index =
        scpfw_resource_counter.channel_count - 1;

    /* @cfg does not consider agent #0 this the reserved platform/server agent */
    size_t scmi_agent_count = cfg->agent_count + 1;

    scmi_agent_table = fwk_mm_calloc(scmi_agent_count,
                                     sizeof(*scmi_agent_table));

    scmi_service_elt = fwk_mm_calloc(scpfw_resource_counter.channel_count + 1,
                                     sizeof(*scmi_service_elt));

    scmi_service_data = fwk_mm_calloc(scpfw_resource_counter.channel_count + 1,
                                      sizeof(*scmi_service_data));

#ifdef CFG_SCPFW_MOD_MSG_SMT
    msg_smt_elt = fwk_mm_calloc(scpfw_resource_counter.channel_count + 1,
                                sizeof(*msg_smt_elt));
    msg_smt_data = fwk_mm_calloc(scpfw_resource_counter.channel_count,
                                 sizeof(*msg_smt_data));
#endif

#ifdef CFG_SCPFW_MOD_TFM_SMT
    tfm_smt_elt = fwk_mm_calloc(scpfw_resource_counter.channel_count + 1,
                                sizeof(*tfm_smt_elt));
    tfm_smt_data = fwk_mm_calloc(scpfw_resource_counter.channel_count,
                                 sizeof(*tfm_smt_data));
#endif

    tfm_mbx_elt = fwk_mm_calloc(scpfw_resource_counter.channel_count + 1,
                                sizeof(*tfm_mbx_elt));
    tfm_mbx_data = fwk_mm_calloc(scpfw_resource_counter.channel_count,
                                 sizeof(*tfm_mbx_data));

    /* Set now the uniqnue scmi module instance configuration data */
    scmi_data = (struct mod_scmi_config){
        .agent_table = scmi_agent_table,
        .agent_count = scmi_agent_count-1,
        .protocol_count_max = 9,
        .vendor_identifier = "ST",
        .sub_vendor_identifier = "ST",
    };
    channel_index = 0;

    for (i = 0; i < cfg->agent_count; i++) {
        struct scpfw_agent_config *agent_cfg = cfg->agent_config + i;
        size_t agent_index = i + 1;

        scmi_agent_table[agent_index].type = agent_type[agent_cfg->agent_id];
        scmi_agent_table[agent_index].name = agent_cfg->name;

        for (j = 0; j < agent_cfg->channel_count; j++) {
            struct scpfw_channel_config *channel_cfg =
                agent_cfg->channel_config + j;
            struct mod_scmi_service_config *service_data;

            service_data = scmi_service_data + channel_index;
            scmi_service_elt[channel_index].name = channel_cfg->name;
            scmi_service_elt[channel_index].data = service_data;

            tfm_mbx_elt[channel_index].name = channel_cfg->name;
            tfm_mbx_elt[channel_index].data = (void *)(tfm_mbx_data +
                                                       channel_index);
            switch  (agent_cfg->agent_id) {
#ifdef CFG_SCPFW_MOD_MSG_SMT
            case STM32MP25_AGENT_ID_M33_NS:
                *service_data = (struct mod_scmi_service_config){
                    .transport_id = (fwk_id_t)
                        FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_MSG_SMT,
                                            msg_smt_index),
                    .transport_api_id = (fwk_id_t)
                        FWK_ID_API_INIT(FWK_MODULE_IDX_MSG_SMT,
                                        MOD_MSG_SMT_API_IDX_SCMI_TRANSPORT),
                    .transport_notification_init_id = FWK_ID_NONE_INIT,
                    .scmi_agent_id = agent_cfg->agent_id,
#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
                    /*
                     * Select the last channel as p2a channel
                     * Only NS agent support async_notif
                     */
                    .scmi_p2a_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_SCMI,
                                                       p2a_index),
#else
                    .scmi_p2a_id = FWK_ID_NONE_INIT,
#endif
                };

                msg_smt_elt[msg_smt_index].name = channel_cfg->name;
                msg_smt_elt[msg_smt_index].data = (void *)(msg_smt_data +
                                                           msg_smt_index);

                msg_smt_data[msg_smt_index] =
                    (struct mod_msg_smt_channel_config){ .type =
                        MOD_MSG_SMT_CHANNEL_TYPE_REQUESTER,.mailbox_size = 128,
                        .driver_id = (fwk_id_t)
                            FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_TFM_MBX,
                                                channel_index),
                        .driver_api_id = (fwk_id_t)
                            FWK_ID_API_INIT(FWK_MODULE_IDX_TFM_MBX, 0),
                    };
                tfm_mbx_data[channel_index] =
                    (struct mod_tfm_mbx_channel_config){
                    .driver_id = (fwk_id_t)
                        FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_MSG_SMT,
                                            msg_smt_index),
                    .driver_api_id = (fwk_id_t)
                        FWK_ID_API_INIT(FWK_MODULE_IDX_MSG_SMT,
                                        MOD_MSG_SMT_API_IDX_DRIVER_INPUT),
                };
                msg_smt_index++;
                break;
#endif
#ifdef CFG_SCPFW_MOD_TFM_SMT
            case STM32MP25_AGENT_ID_CA35 :
            case STM32MP25_AGENT_ID_CA35_BL31:
                *service_data = (struct mod_scmi_service_config){
                    .transport_id = (fwk_id_t)
                        FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_TFM_SMT,
                                            tfm_smt_index),
                    .transport_api_id = (fwk_id_t)
                        FWK_ID_API_INIT(FWK_MODULE_IDX_TFM_SMT,
                                        MOD_TFM_SMT_API_IDX_SCMI_TRANSPORT),
                    .transport_notification_init_id = FWK_ID_NONE_INIT,
                    .scmi_agent_id = agent_cfg->agent_id,
                    .scmi_p2a_id = FWK_ID_NONE_INIT,
                };

                tfm_smt_elt[tfm_smt_index].name = channel_cfg->name;
                tfm_smt_elt[tfm_smt_index ].data = (void *)(tfm_smt_data +
                                                            tfm_smt_index);
                tfm_smt_data[tfm_smt_index] = (struct mod_tfm_smt_channel_config){
                    .type = MOD_TFM_SMT_CHANNEL_TYPE_REQUESTER,
                    .policies = MOD_SMT_POLICY_NONE,
                    .mailbox_address = (unsigned int)channel_cfg->shm.area,
                    .mailbox_size = 128,
                    .driver_id = (fwk_id_t)
                        FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_TFM_MBX, channel_index),
                    .driver_api_id = (fwk_id_t)
                        FWK_ID_API_INIT(FWK_MODULE_IDX_TFM_MBX, 0),
                };

                tfm_mbx_data[channel_index] =
                    (struct mod_tfm_mbx_channel_config){
                        .driver_id = (fwk_id_t)
                            FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_TFM_SMT,
                                                tfm_smt_index),
                        .driver_api_id = (fwk_id_t)
                            FWK_ID_API_INIT(FWK_MODULE_IDX_TFM_SMT,
                                            MOD_TFM_SMT_API_IDX_DRIVER_INPUT),
                        .chan_mbx = channel_cfg->chan_mbx,
                    };
                tfm_smt_index++;
                break;
#endif
            default:
                FWK_LOG_ERR("agent transport un supported");
                panic();
            }

            channel_index++;
        }

#if defined(CFG_SCPFW_MOD_MSG_SMT) && defined(BUILD_HAS_SCMI_NOTIFICATIONS)
        if (agent_cfg->agent_id == STM32MP25_AGENT_ID_M33_NS) {
            tfm_mbx_elt[p2a_index].name = "M33_NS_NOTIF";
            tfm_mbx_elt[p2a_index].data = (void *)(tfm_mbx_data + p2a_index);
            struct mod_scmi_service_config *service_data = scmi_service_data +
                p2a_index;
            scmi_service_elt[p2a_index].name = "M33_NS_NOTIF";
            scmi_service_elt[p2a_index].data = service_data;

            *service_data = (struct mod_scmi_service_config)
            {
                .transport_id = (fwk_id_t)
                    FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_MSG_SMT, msg_smt_index),
                .transport_api_id = (fwk_id_t)
                    FWK_ID_API_INIT(FWK_MODULE_IDX_MSG_SMT,
                                    MOD_MSG_SMT_API_IDX_SCMI_TRANSPORT),
                .transport_notification_init_id = FWK_ID_NONE_INIT,
                .scmi_agent_id = agent_cfg->agent_id,
                .scmi_p2a_id = FWK_ID_NONE_INIT,
            };

            msg_smt_elt[msg_smt_index].name = "M33_NS_NOTIF";
            msg_smt_elt[msg_smt_index].data = (void *)(msg_smt_data +
                                                       msg_smt_index);
            msg_smt_data[msg_smt_index] = (struct mod_msg_smt_channel_config){
                .type = MOD_MSG_SMT_CHANNEL_TYPE_COMPLETER,
                .mailbox_size = 128,
                .driver_id = (fwk_id_t)
                    FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_TFM_MBX,p2a_index),
                .driver_api_id = (fwk_id_t)
                    FWK_ID_API_INIT(FWK_MODULE_IDX_TFM_MBX, 0),
            };

            tfm_mbx_data[p2a_index] = (struct mod_tfm_mbx_channel_config){
                .driver_id = (fwk_id_t)
                    FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_MSG_SMT, msg_smt_index),
                .driver_api_id = (fwk_id_t)
                    FWK_ID_API_INIT(FWK_MODULE_IDX_MSG_SMT,
                                    MOD_MSG_SMT_API_IDX_DRIVER_INPUT),
            };
            msg_smt_index++;

            /* Array element at p2a_index is used, decrement it */
            p2a_index--;
        }
#endif
    }
#if defined(BUILD_HAS_SCMI_NOTIFICATIONS)
    fwk_assert(channel_index == p2a_index);
#endif
}

static void set_resources(struct scpfw_config *cfg)
{
    size_t i, j, k;
#ifdef CFG_SCPFW_MOD_POWER_DOMAIN
    size_t system_index = scpfw_resource_counter.pd_count; /* Last element */
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
        (fwk_id_t)FWK_ID_API_INIT(FWK_MODULE_IDX_STM32_PD,
                                  MOD_STM32_PD_API_IDX_BASIC);
    pd_elt_data_sys->attributes.pd_type = MOD_PD_TYPE_DEVICE;
    pd_elt_data_sys->allowed_state_mask_table =
        pd_allowed_state_mask_table;
    pd_elt_data_sys->allowed_state_mask_table_size =
        FWK_ARRAY_SIZE(pd_allowed_state_mask_table);
    pd_elt_sys->data = pd_elt_data_sys;
#endif
    for (i = 0; i < cfg->agent_count; i++) {
        struct scpfw_agent_config *agent_cfg = cfg->agent_config + i;
        size_t agent_index = i + 1;

        if (agent_index != agent_cfg->agent_id) {

            FWK_LOG_ERR("scpfw config expects agent ID is agent index");
            panic();
        }

        for (j = 0; j < agent_cfg->channel_count; j++) {
            struct scpfw_channel_config *channel_cfg = agent_cfg->channel_config + j;

#ifdef CFG_SCPFW_MOD_SCMI_CLOCK
            /* Add first SCMI clock. We will add later the clocks used for DVFS */
            if (channel_cfg->clock_count) {
                size_t clock_index = scpfw_resource_counter.clock_index;
                struct mod_scmi_clock_device *dev = NULL;

                /* Set SCMI clocks array for the SCMI agent */
                dev = fwk_mm_calloc(channel_cfg->clock_count,
                                    sizeof(struct mod_scmi_clock_device));

                fwk_assert(!scmi_clk_agent_cfg[agent_index].device_table);
                scmi_clk_agent_cfg[agent_index] =
                    (struct mod_scmi_clock_agent_config){
                        .agent_device_count = channel_cfg->clock_count,
                        .device_table = dev,
                    };

                /* Update element name to agent name (pre-init used "") */
                scmi_clock_elt[agent_index].name = agent_cfg->name;

                /* Set clock and tfm/clock elements and config data */
                for (k = 0; k < channel_cfg->clock_count; k++) {
                    struct scmi_clock *clock_cfg = channel_cfg->clock + k;

                    dev[k].element_id =
                        (fwk_id_t)FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_CLOCK, clock_index);

                    tfm_clock_cfg[clock_index].clk = clock_cfg->clk;
                    tfm_clock_cfg[clock_index].default_enabled = clock_cfg->enabled;

                    tfm_clock_elt[clock_index].name = clock_cfg->name;
                    tfm_clock_elt[clock_index].data = (void *)(tfm_clock_cfg + clock_index);

                    clock_data[clock_index] = (struct mod_clock_dev_config){
                        .driver_id = (fwk_id_t)FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_TFM_CLOCK,
                                                                   clock_index),
                        .api_id = (fwk_id_t)FWK_ID_API_INIT(FWK_MODULE_IDX_TFM_CLOCK,
                                                            0),
#ifdef CFG_SCPFW_MOD_POWER_DOMAIN
                        .pd_source_id = (fwk_id_t)
                            FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_POWER_DOMAIN, 0),
#endif
                    };

                    clock_elt[clock_index].name = clock_cfg->name;
                    clock_elt[clock_index].data = (void *)(clock_data + clock_index);

                    clock_index++;
                }

                scpfw_resource_counter.clock_index = clock_index;
            }
#endif

#ifdef CFG_SCPFW_MOD_RESET_DOMAIN
            if (channel_cfg->reset_count) {
                struct mod_scmi_reset_domain_device *dev = NULL;
                size_t reset_index = scpfw_resource_counter.reset_index;

                /* Set SCMI reset domains array for the SCMI agent */
                dev = fwk_mm_calloc(channel_cfg->reset_count, sizeof(*dev));

                fwk_assert(!scmi_reset_agent_tbl[agent_index].device_table);
                scmi_reset_agent_tbl[agent_index].agent_domain_count = channel_cfg->reset_count;
                scmi_reset_agent_tbl[agent_index].device_table = dev;

                /* Set reset_domain and tfm/reset elements and config data */
                for (k = 0; k < channel_cfg->reset_count; k++) {
                    struct scmi_reset *reset_cfg = channel_cfg->reset + k;

                    dev[k].element_id =
                        (fwk_id_t)FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_RESET_DOMAIN,
                                                      reset_index);

                    tfm_reset_data[reset_index].rstctrl = reset_cfg->rstctrl;

                    tfm_reset_elt[reset_index].name = reset_cfg->name;
                    tfm_reset_elt[reset_index].data =
                        (void *)(tfm_reset_data + reset_index);

                    reset_data[reset_index] = (struct mod_reset_domain_dev_config){
                        .driver_id =
                            (fwk_id_t)FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_TFM_RESET,
                                                          reset_index),
                        .driver_api_id =
                            (fwk_id_t)FWK_ID_API_INIT(FWK_MODULE_IDX_TFM_RESET, 0),
                        .modes = MOD_RESET_DOMAIN_AUTO_RESET |
                                 MOD_RESET_DOMAIN_MODE_EXPLICIT_ASSERT |
                                 MOD_RESET_DOMAIN_MODE_EXPLICIT_DEASSERT,
                    };

                    reset_elt[reset_index].name = reset_cfg->name;
                    reset_elt[reset_index].data = (void *)(reset_data + reset_index);

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

                /* Set voltage_domain and tfm/regu elements and config data */
                for (k = 0; k < channel_cfg->voltd_count; k++) {
                    struct scmi_voltd *voltd_cfg = channel_cfg->voltd + k;
                    static const char reserved[] = "reserved";
                    const char *name = NULL;

                    if (voltd_cfg->dev) {
                        name = voltd_cfg->dev->name;
                    } else {
                        name = reserved;
                    }
                    dev[k].element_id =
                       (fwk_id_t)FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_VOLTAGE_DOMAIN, regu_index);

                    tfm_regu_elt[regu_index].name = name;
                    tfm_regu_elt[regu_index].data = (void *)(tfm_regu_cfg + regu_index);
                    tfm_regu_cfg[regu_index].dev = voltd_cfg->dev;
                    tfm_regu_cfg[regu_index].default_enabled = voltd_cfg->enabled;

                    voltd_data[regu_index].driver_id =
                        (fwk_id_t)FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_TFM_REGU_CONSUMER,
                                                      regu_index);
                    voltd_data[regu_index].api_id =
                        (fwk_id_t)FWK_ID_API_INIT(FWK_MODULE_IDX_TFM_REGU_CONSUMER, 0);

                    voltd_elt[regu_index].name = name;
                    voltd_elt[regu_index].data = (void *)(voltd_data + regu_index);
                    regu_index++;
                }

                scpfw_resource_counter.regu_index = regu_index;
            }
#endif
#ifdef CFG_SCPFW_MOD_POWER_DOMAIN
            if (channel_cfg->pd_count) {
                size_t pd_index = scpfw_resource_counter.pd_index;
                /* Power domains */
                for (k = 0; k < channel_cfg->pd_count; k++) {
                    struct fwk_element *pd_elt_k = pd_elt + pd_index;
                    struct mod_power_domain_element_config *pd_elt_data_k =
                        fwk_mm_calloc(1, sizeof(*pd_elt_data_k));
                    struct fwk_element *stm32_pd_elt_k =
                        stm32_pd_elt + pd_index;
                    struct scmi_pd *scmi_pd = channel_cfg->pd + k;
                    struct mod_stm32_pd_config *mod_stm32_pd_config =
                        fwk_mm_calloc(1, sizeof(*mod_stm32_pd_config));

                    mod_stm32_pd_config->name = scmi_pd->name;
                    mod_stm32_pd_config->clk = scmi_pd->clk;
                    mod_stm32_pd_config->regu = scmi_pd->regu;
                    mod_stm32_pd_config->firewall = scmi_pd->firewall;
                    mod_stm32_pd_config->n_firewall = scmi_pd->n_firewall;

                    stm32_pd_elt_k->name = scmi_pd->name;
                    stm32_pd_elt_k->data = (void *)mod_stm32_pd_config;

                    pd_elt_k->name = scmi_pd->name;

                    pd_elt_data_k->parent_idx = system_index;
                    pd_elt_data_k->driver_id =
                        (fwk_id_t)FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_STM32_PD, pd_index);
                    pd_elt_data_k->api_id =
                        (fwk_id_t)FWK_ID_API_INIT(FWK_MODULE_IDX_STM32_PD,
                                                  MOD_STM32_PD_API_IDX_GPU);
                    pd_elt_data_k->attributes.pd_type = MOD_PD_TYPE_DEVICE;
                    pd_elt_data_k->allowed_state_mask_table =
                        pd_allowed_state_mask_table;
                    pd_elt_data_k->allowed_state_mask_table_size =
                        FWK_ARRAY_SIZE(pd_allowed_state_mask_table);
                    pd_elt_k->data = pd_elt_data_k;
                    pd_index++;

                }
                scpfw_resource_counter.pd_index = pd_index;

            }
#endif
        }
    }
}

void scpfw_configure(struct scpfw_config *cfg)
{
    count_resources(cfg);
    allocate_global_resources(cfg);
    set_scmi_comm_resources(cfg);
    set_resources(cfg);
}
