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

#ifdef CFG_SCPFW_MOD_OPTEE_CLOCK
#    include <mod_optee_clock.h>
#endif
#ifdef CFG_SCPFW_MOD_OPTEE_RESET
#    include <mod_optee_reset.h>
#endif
#ifdef CFG_SCPFW_MOD_MSG_SMT
#    include <mod_msg_smt.h>
#endif
#ifdef CFG_SCPFW_MOD_RESET_DOMAIN
#    include <mod_reset_domain.h>
#endif
#ifdef CFG_SCPFW_MOD_SCMI_CLOCK
#    include <mod_scmi_clock.h>
#endif
#ifdef CFG_SCPFW_MOD_SCMI_RESET_DOMAIN
#    include <mod_scmi_reset_domain.h>
#endif

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

#ifdef CFG_SCPFW_MOD_RESET_DOMAIN
/* SCMI reset domains and optee reset controller */
static struct mod_scmi_reset_domain_agent *scmi_reset_agent_tbl;
static struct fwk_element *optee_reset_elt;
static struct mod_optee_reset_dev_config *optee_reset_data;
static struct fwk_element *reset_elt;
static struct mod_reset_domain_dev_config *reset_data;
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

/*
 * Indices state when applying agents configuration
 * @channel_count: Number of channels (mailbox/shmem links) used
 * @clock_index: Current index for clock and optee/clock (same indices)
 * @clock_count: Number of clocks (also number of optee/clocks)
 * @reset_index: Current index for reset controller and optee/reset
 * @reset_count: Number of reset controller (optee/reset) instances
 */
struct scpfw_resource_counter {
    size_t channel_count;
    size_t clock_index;
    size_t clock_count;
    size_t reset_index;
    size_t reset_count;
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

            /* Clocks for scmi_clock */
            scpfw_resource_counter.clock_count += channel_cfg->clock_count;
            /* Reset for scmi_reset only */
            scpfw_resource_counter.reset_count += channel_cfg->reset_count;
        }
    }

#ifndef CFG_SCPFW_MOD_CLOCK
    fwk_assert(!scpfw_resource_counter.clock_count);
#endif
#ifndef CFG_SCPFW_MOD_RESET_DOMAIN
    fwk_assert(!scpfw_resource_counter.reset_count);
#endif
}

/*
 * Allocate all tables that may be needed. An optimized implementation would
 * allocate a single piece of memory and set the pointers accordingly.
 */
static void allocate_global_resources(struct scpfw_config *cfg)
{
    struct mod_scmi_reset_domain_config *scmi_reset_config __maybe_unused;
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
        scpfw_resource_counter.clock_count, sizeof(*optee_clock_cfg));
    optee_clock_elt = fwk_mm_calloc(
        scpfw_resource_counter.clock_count + 1, sizeof(*optee_clock_elt));

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
}

enum mailbox_type {
    MAILBOX_TYPE_MSG_SMT,
    MAILBOX_TYPE_COUNT,
};

static enum mailbox_type module_idx_smt_table[SCMI_AGENT_ID_COUNT] = {
    /*
     * Agent NSEC0 is Cortex-A that communicates over OP-TEE's SCMI PTA
     * and OP-TEE shared memory hence using MSG_SMT module.
     */
    [SCMI_AGENT_ID_NSEC0] = MAILBOX_TYPE_MSG_SMT,
};

static void set_scmi_comm_resources(struct scpfw_config *cfg)
{
    unsigned int channel_index = 0;
    unsigned int __maybe_unused msg_smt_index = 0;
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
                            FWK_MODULE_IDX_OPTEE_CLOCK, clock_index),
                        .api_id = (fwk_id_t)FWK_ID_API_INIT(
                            FWK_MODULE_IDX_OPTEE_CLOCK, 0),
                        .pd_source_id = FWK_ID_NONE,
                    };

                    dev[k].element_id = (fwk_id_t)FWK_ID_ELEMENT_INIT(
                        FWK_MODULE_IDX_CLOCK, clock_index);

                    optee_clock_cfg[clock_index].clk = clock_cfg->clk;
                    optee_clock_cfg[clock_index].default_enabled =
                        clock_cfg->enabled;

                    optee_clock_elt[clock_index].name = clock_cfg->name;
                    optee_clock_elt[clock_index].data =
                        (void *)(optee_clock_cfg + clock_index);

                    memcpy(clock_data + clock_index, &cdata, sizeof(cdata));

                    clock_elt[clock_index].name = clock_cfg->name;
                    clock_elt[clock_index].data =
                        (void *)(clock_data + clock_index);

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
