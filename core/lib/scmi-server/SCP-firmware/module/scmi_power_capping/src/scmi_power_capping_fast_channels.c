/*
 * Arm SCP/MCP Software
 * Copyright (c) 2023-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *      SCMI power capping and monitoring protocol fast channels.
 */

#include "internal/scmi_power_capping.h"
#include "internal/scmi_power_capping_core.h"
#include "internal/scmi_power_capping_fast_channels.h"
#include "mod_scmi_std.h"

#ifdef BUILD_HAS_SCMI_POWER_CAPPING_STD_COMMANDS
#    include <mod_scmi.h>
#endif
#include <mod_transport.h>

#include <fwk_core.h>
#include <fwk_event.h>
#include <fwk_id.h>
#include <fwk_log.h>
#include <fwk_mm.h>

#ifndef BUILD_HAS_SCMI_POWER_CAPPING
#    include <fwk_module.h>
#endif

/*!
 * \brief Fast channels address index.
 */
enum mod_scmi_power_capping_fast_channels_cmd_handler_index {
    MOD_SCMI_PCAPPING_FAST_CHANNEL_HANDLER_IDX_CAP_GET,
    MOD_SCMI_PCAPPING_FAST_CHANNEL_HANDLER_IDX_CAP_SET,
    MOD_SCMI_PCAPPING_FAST_CHANNEL_HANDLER_IDX_PAI_GET,
    MOD_SCMI_PCAPPING_FAST_CHANNEL_HANDLER_IDX_PAI_SET,
    MOD_SCMI_PCAPPING_FAST_CHANNEL_HANDLER_IDX_COUNT
};

struct pcapping_fast_channel_ctx {
    const struct scmi_pcapping_fch_config *fch_config;
    /* The fast channel address */
    struct mod_transport_fast_channel_addr fch_address;
    uint32_t fch_attributes;
    uint32_t fch_rate_limit;
    /* Transport Fast Channels API */
    const struct mod_transport_fast_channels_api *transport_fch_api;
};

struct {
    struct pcapping_fast_channel_ctx *fch_ctx_table;
    uint32_t fch_count;
    bool callback_registered;
    enum mod_transport_fch_interrupt_type interrupt_type;
} pcapping_fast_channel_global_ctx = { .callback_registered = false };

static void pcapping_fast_channel_callback(uintptr_t param)
{
    int status;
    uint32_t *event_param;

    struct fwk_event event = (struct fwk_event){
        .id = FWK_ID_EVENT_INIT(
            FWK_MODULE_IDX_SCMI_POWER_CAPPING,
            SCMI_POWER_CAPPING_EVENT_IDX_FAST_CHANNELS_PROCESS),
        .source_id = FWK_ID_MODULE_INIT(FWK_MODULE_IDX_SCMI_POWER_CAPPING),
        .target_id = FWK_ID_MODULE(FWK_MODULE_IDX_SCMI_POWER_CAPPING),
    };
    event_param = (uint32_t *)event.params;
    *event_param = (uint32_t)param;

    status = fwk_put_event(&event);
    if (status != FWK_SUCCESS) {
        FWK_LOG_ERR(
            "[SCMI-Power-Capping-Fast-Channel] Error creating fast channel "
            "event.");
    }
}

static void pcapping_fast_channel_get_cap(
    struct pcapping_fast_channel_ctx *fch_ctx)
{
    int status;

    status = pcapping_core_get_cap(
        fch_ctx->fch_config->scmi_power_capping_domain_idx,
        (uint32_t *)fch_ctx->fch_address.local_view_address);
    if (status != FWK_SUCCESS) {
        FWK_LOG_ERR("[SCMI-Power-Capping-Fast-Channel] Error getting cap.");
    }
}

static void pcapping_fast_channel_set_cap(
    struct pcapping_fast_channel_ctx *fch_ctx)
{
    int status;

    status = pcapping_core_set_cap(
        fch_ctx->fch_config->service_id,
        fch_ctx->fch_config->scmi_power_capping_domain_idx,
        true,
        *(uint32_t *)fch_ctx->fch_address.local_view_address);
    if ((status != FWK_SUCCESS) && (status != FWK_PENDING)) {
        FWK_LOG_ERR("[SCMI-Power-Capping-Fast-Channel] Error setting cap.");
    }
}

static void pcapping_fast_channel_get_pai(
    struct pcapping_fast_channel_ctx *fch_ctx)
{
    int status;

    status = pcapping_core_get_pai(
        fch_ctx->fch_config->scmi_power_capping_domain_idx,
        (uint32_t *)fch_ctx->fch_address.local_view_address);
    if (status != FWK_SUCCESS) {
        FWK_LOG_ERR("[SCMI-Power-Capping-Fast-Channel] Error getting PAI.");
    }
}

static void pcapping_fast_channel_set_pai(
    struct pcapping_fast_channel_ctx *fch_ctx)
{
    int status;

    status = pcapping_core_set_pai(
        fch_ctx->fch_config->service_id,
        fch_ctx->fch_config->scmi_power_capping_domain_idx,
        *(uint32_t *)fch_ctx->fch_address.local_view_address);
    if (status != FWK_SUCCESS) {
        FWK_LOG_ERR("[SCMI-Power-Capping-Fast-Channel] Error setting PAI.");
    }
}

static void (*pcapping_fast_channel_handler
                 [MOD_SCMI_PCAPPING_FAST_CHANNEL_HANDLER_IDX_COUNT])(
    struct pcapping_fast_channel_ctx *fch_ctx) = {
    [MOD_SCMI_PCAPPING_FAST_CHANNEL_HANDLER_IDX_CAP_GET] =
        pcapping_fast_channel_get_cap,
    [MOD_SCMI_PCAPPING_FAST_CHANNEL_HANDLER_IDX_CAP_SET] =
        pcapping_fast_channel_set_cap,
    [MOD_SCMI_PCAPPING_FAST_CHANNEL_HANDLER_IDX_PAI_GET] =
        pcapping_fast_channel_get_pai,
    [MOD_SCMI_PCAPPING_FAST_CHANNEL_HANDLER_IDX_PAI_SET] =
        pcapping_fast_channel_set_pai,
};

#ifdef BUILD_HAS_SCMI_POWER_CAPPING_STD_COMMANDS
static bool is_fch_match(
    struct pcapping_fast_channel_ctx *fch_ctx,
    uint32_t domain_idx,
    uint32_t message_id)
{
    return fch_ctx->fch_config->scmi_power_capping_domain_idx == domain_idx &&
        fch_ctx->fch_config->message_id == message_id;
}

static struct pcapping_fast_channel_ctx *get_channel_ctx(
    uint32_t domain_idx,
    uint32_t message_id)
{
    for (size_t channel_index = 0u;
         channel_index < pcapping_fast_channel_global_ctx.fch_count;
         channel_index++) {
        struct pcapping_fast_channel_ctx *fch_ctx =
            &pcapping_fast_channel_global_ctx.fch_ctx_table[channel_index];

        if (is_fch_match(fch_ctx, domain_idx, message_id)) {
            return fch_ctx;
        }
    }

    return NULL;
}

int pcapping_fast_channel_get_info(
    uint32_t domain_idx,
    uint32_t message_id,
    struct pcapping_fast_channel_info *info)
{
    struct pcapping_fast_channel_ctx *fch_ctx;

    fch_ctx = get_channel_ctx(domain_idx, message_id);

    if (fch_ctx == NULL) {
        return FWK_E_RANGE;
    }

    info->fch_address = fch_ctx->fch_address.target_view_address;
    info->fch_channel_size = fch_ctx->fch_address.length;
    info->fch_attributes = fch_ctx->fch_attributes;
    info->fch_rate_limit = fch_ctx->fch_rate_limit;

    return FWK_SUCCESS;
}

bool pcapping_fast_channel_get_msg_support(uint32_t message_id)
{
    bool fast_channel_support = false;
    uint32_t channel_index = 0u;

    while ((channel_index < pcapping_fast_channel_global_ctx.fch_count) &&
           (!fast_channel_support)) {
        struct pcapping_fast_channel_ctx *fch_ctx =
            &pcapping_fast_channel_global_ctx.fch_ctx_table[channel_index];

        fast_channel_support = fch_ctx->fch_config->message_id == message_id;
        channel_index++;
    }

    return fast_channel_support;
}

bool pcapping_fast_channel_get_domain_support(uint32_t domain_idx)
{
    bool fast_channel_support = false;
    uint32_t channel_index = 0u;

    while ((channel_index < pcapping_fast_channel_global_ctx.fch_count) &&
           (!fast_channel_support)) {
        struct pcapping_fast_channel_ctx *fch_ctx =
            &pcapping_fast_channel_global_ctx.fch_ctx_table[channel_index];

        fast_channel_support =
            fch_ctx->fch_config->scmi_power_capping_domain_idx == domain_idx;
        channel_index++;
    }

    return fast_channel_support;
}
#endif

static void pcapping_fast_channel_process_command(uint32_t fch_idx)
{
    struct pcapping_fast_channel_ctx *fch_ctx;
    uint32_t handler_index;

    if (fch_idx < pcapping_fast_channel_global_ctx.fch_count) {
        fch_ctx = &(pcapping_fast_channel_global_ctx.fch_ctx_table[fch_idx]);

        handler_index =
            fch_ctx->fch_config->message_id - MOD_SCMI_POWER_CAPPING_CAP_GET;

        pcapping_fast_channel_handler[handler_index](fch_ctx);

    } else {
        FWK_LOG_ERR(
            "[SCMI-Power-Capping-Fast-Channel] Error processing out of index "
            "fast channel ID "
            "event.");
    }
}

static void pcapping_fast_channel_setup_interrupt(
    uint32_t fch_idx,
    const struct scmi_pcapping_fch_config *fch_config,
    struct pcapping_fast_channel_ctx *fch_ctx)
{
    if (pcapping_fast_channel_global_ctx.interrupt_type ==
        MOD_TRANSPORT_FCH_INTERRUPT_TYPE_TIMER) {
        /*
         * For polled fast channels, we need to register one single
         * call back for all channels so register this only once.
         */
        if (!pcapping_fast_channel_global_ctx.callback_registered) {
            fch_ctx->transport_fch_api->transport_fch_register_callback(
                fch_config->transport_id,
                (uintptr_t)NULL,
                pcapping_fast_channel_callback);
            pcapping_fast_channel_global_ctx.callback_registered = true;
        }
    } else {
        fch_ctx->transport_fch_api->transport_fch_register_callback(
            fch_config->transport_id,
            (uintptr_t)fch_idx,
            pcapping_fast_channel_callback);
    }
}

int pcapping_fast_channel_process_event(const struct fwk_event *event)
{
    uint32_t fch_idx;

    if (pcapping_fast_channel_global_ctx.interrupt_type ==
        MOD_TRANSPORT_FCH_INTERRUPT_TYPE_HW) {
        fch_idx = *(uint32_t *)event->params;
        pcapping_fast_channel_process_command(fch_idx);
    } else {
        for (fch_idx = 0; fch_idx < pcapping_fast_channel_global_ctx.fch_count;
             fch_idx++) {
            pcapping_fast_channel_process_command(fch_idx);
        }
    }

    return FWK_SUCCESS;
}

int pcapping_fast_channel_ctx_init(
    const struct scmi_pcapping_fch_config *fch_config_table,
    size_t fch_count)
{
    pcapping_fast_channel_global_ctx.fch_count = fch_count;

    pcapping_fast_channel_global_ctx.fch_ctx_table = fwk_mm_calloc(
        pcapping_fast_channel_global_ctx.fch_count,
        sizeof(struct pcapping_fast_channel_ctx));

    for (uint32_t fch_idx = 0u; fch_idx < fch_count; fch_idx++) {
        const struct scmi_pcapping_fch_config *fch_config =
            &fch_config_table[fch_idx];

        if ((fch_config->message_id > MOD_SCMI_POWER_CAPPING_PAI_SET) ||
            (fch_config->message_id < MOD_SCMI_POWER_CAPPING_CAP_GET)) {
            return FWK_E_RANGE;
        }
        pcapping_fast_channel_global_ctx.fch_ctx_table[fch_idx].fch_config =
            fch_config;
    }
    return FWK_SUCCESS;
}

int pcapping_fast_channel_bind(void)
{
    int status;
    uint32_t fch_idx;
    struct pcapping_fast_channel_ctx *fch_ctx;

    for (fch_idx = 0; fch_idx < pcapping_fast_channel_global_ctx.fch_count;
         fch_idx++) {
        fch_ctx = &(pcapping_fast_channel_global_ctx.fch_ctx_table[fch_idx]);
        status = fwk_module_bind(
            fch_ctx->fch_config->transport_id,
            fch_ctx->fch_config->transport_api_id,
            &(fch_ctx->transport_fch_api));
        if (status != FWK_SUCCESS) {
            return FWK_E_PANIC;
        }
    }

    return FWK_SUCCESS;
}

void pcapping_fast_channel_start(void)
{
    uint32_t fch_idx;
    const struct scmi_pcapping_fch_config *fch_config;
    struct pcapping_fast_channel_ctx *fch_ctx;

    for (fch_idx = 0; fch_idx < pcapping_fast_channel_global_ctx.fch_count;
         fch_idx++) {
        fch_ctx = &(pcapping_fast_channel_global_ctx.fch_ctx_table[fch_idx]);
        fch_config = fch_ctx->fch_config;
        fch_ctx->transport_fch_api->transport_get_fch_address(
            fch_config->transport_id, &(fch_ctx->fch_address));

        fch_ctx->fch_attributes = 0x0u;

        fch_ctx->transport_fch_api->transport_get_fch_rate_limit(
            fch_config->transport_id, &(fch_ctx->fch_rate_limit));

        fch_ctx->transport_fch_api->transport_get_fch_interrupt_type(
            fch_config->transport_id,
            &(pcapping_fast_channel_global_ctx.interrupt_type));

        pcapping_fast_channel_setup_interrupt(fch_idx, fch_config, fch_ctx);
    }
}
