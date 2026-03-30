/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "internal/fmu_common.h"

#include <mod_fmu.h>

#include <fwk_assert.h>
#include <fwk_id.h>
#include <fwk_interrupt.h>
#include <fwk_log.h>
#include <fwk_mm.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>
#include <fwk_notification.h>
#include <fwk_status.h>

#define FMU_ROOT_IDX       0
#define FMU_MAX_TREE_DEPTH 4

static struct {
    const struct mod_fmu_dev_config **device_config;
    const struct mod_fmu_impl_api **impl_apis;
    unsigned int num_devices;
} ctx;

static unsigned int find_next_fmu(
    unsigned int device_idx,
    unsigned int node_idx)
{
    unsigned int idx;
    const struct mod_fmu_dev_config *device_config;

    if (device_idx == MOD_FMU_PARENT_NONE || node_idx == MOD_FMU_PARENT_NONE) {
        return MOD_FMU_PARENT_NONE;
    }

    for (idx = 0; idx < ctx.num_devices; idx++) {
        device_config = ctx.device_config[idx];
        if (device_config->parent == device_idx &&
            (device_config->parent_cr_index == node_idx ||
             device_config->parent_ncr_index == node_idx)) {
            return idx;
        }
    }

    return MOD_FMU_PARENT_NONE;
}

/*!
 * \brief Structure to store a discovered fault entry
 */
struct fault_entry {
    uint32_t dev;
    uint32_t node;
};

static bool next_fault(bool critical)
{
    const struct mod_fmu_dev_config *cfg;
    struct mod_fmu_fault fault = { 0 };
    bool (*peek_funct)(const struct mod_fmu_dev_config *, unsigned int *);
    void (*ack_funct)(
        const struct mod_fmu_dev_config *,
        struct mod_fmu_fault *,
        unsigned int,
        bool *);
    unsigned int depth = 0;
    unsigned int dev = FMU_ROOT_IDX;
    unsigned int node;
    unsigned int notifications_sent;
    struct fault_entry fault_entry_stack[FMU_MAX_TREE_DEPTH] = { 0 };
    bool fault_tracked = false;
#ifdef BUILD_HAS_NOTIFICATION
    struct mod_fmu_fault_notification_params *params;
#endif

    /* Traverse through the tree root-first and push fault (device + node) onto
     * the fault stack*/
    while (dev != MOD_FMU_PARENT_NONE) {
        /* Check if maximum tree depth reached */
        if (depth >= FMU_MAX_TREE_DEPTH) {
            FWK_LOG_ERR(MOD_NAME "Maximum tree depth reached");
            fwk_trap();
        }

        /* Find fault entry in this device */
        cfg = ctx.device_config[dev];
        peek_funct = ctx.impl_apis[dev]->fault_peek;
        fwk_assert(peek_funct != NULL);

        /* Check if errors in this FMU and stop if not */
        if (!peek_funct(cfg, &node)) {
            break;
        }

        /* Push next fault entry onto stack */
        fault_entry_stack[depth++] =
            (struct fault_entry){ .dev = dev, .node = node };

        /* Get next device */
        dev = find_next_fmu(dev, node);
    }

    /* Return false if no fault found at the root FMU */
    if (depth == 0) {
        return false;
    }

    /* Set device of fault to last discovered fault */
    fault.device_idx = fault_entry_stack[depth - 1].dev;

    /* Pop entries one by one from the fault stack and acknowledge them */
    do {
        depth--;

        /* Pop entry */
        cfg = ctx.device_config[fault_entry_stack[depth].dev];
        ack_funct = ctx.impl_apis[fault_entry_stack[depth].dev]->fault_ack;
        fwk_assert(ack_funct != NULL);

        /* Acknowledge fault */
        ack_funct(cfg, &fault, fault_entry_stack[depth].node, &fault_tracked);

    } while (depth != 0);

    /* Log details and raise event */
    FWK_LOG_INFO(
        MOD_NAME "%s fault received: Device: 0x%x, Node 0x%x, SM 0x%x",
        critical ? "Critical" : "Non-critical",
        fault.device_idx,
        fault.node_idx,
        fault.sm_idx);

#ifdef BUILD_HAS_NOTIFICATION
    struct fwk_event event = {
        .id = mod_fmu_notification_id_fault,
        .source_id = FWK_ID_MODULE_INIT(FWK_MODULE_IDX_FMU),
    };
    params = (struct mod_fmu_fault_notification_params *)event.params;
    params->critical = critical;
    params->fault.device_idx = fault.device_idx;
    params->fault.node_idx = fault.node_idx;
    params->fault.sm_idx = fault.sm_idx;

    if (fwk_notification_notify(&event, &notifications_sent) != FWK_SUCCESS) {
        FWK_LOG_ERR(MOD_NAME "Error raising notification");
        fwk_trap();
    }
#endif

    return true;
}

/*
 * Common interrupt handler
 */
static void fmu_isr(bool critical)
{
    unsigned int i;

    /* Discover all active fault records */
    for (i = 0; next_fault(critical); i++) {
        continue;
    }

    if (i == 0) {
        FWK_LOG_ERR(MOD_NAME "No error record found");
        fwk_trap();
    }
}

/*
 * Critical fault ISR
 */
static void fmu_isr_critical(void)
{
    fmu_isr(true);
}

/*
 * Non-critical fault ISR
 */
static void fmu_isr_non_critical(void)
{
    fmu_isr(false);
}

/*
 * API Handlers
 */
static int inject(const struct mod_fmu_fault *fault)
{
    const struct mod_fmu_dev_config *config;
    int (*func)(
        const struct mod_fmu_dev_config *config,
        const struct mod_fmu_fault *fault);

    if (fault == NULL || fault->device_idx >= ctx.num_devices) {
        return FWK_E_PARAM;
    }

    func = ctx.impl_apis[fault->device_idx]->inject;
    if (func == NULL) {
        return FWK_E_SUPPORT;
    }
    config = ctx.device_config[fault->device_idx];

    return func(config, fault);
}

static int get_enabled(const struct mod_fmu_fault *fault, bool *enabled)
{
    const struct mod_fmu_dev_config *config;
    int (*func)(
        const struct mod_fmu_dev_config *config,
        const struct mod_fmu_fault *fault,
        bool *enabled);

    if (fault == NULL || enabled == NULL ||
        fault->device_idx >= ctx.num_devices) {
        return FWK_E_PARAM;
    }

    func = ctx.impl_apis[fault->device_idx]->get_enabled;
    if (func == NULL) {
        return FWK_E_SUPPORT;
    }
    config = ctx.device_config[fault->device_idx];

    return func(config, fault, enabled);
}

static int set_enabled(const struct mod_fmu_fault *fault, bool enabled)
{
    const struct mod_fmu_dev_config *config;
    int (*func)(
        const struct mod_fmu_dev_config *config,
        const struct mod_fmu_fault *fault,
        bool enabled);

    if (fault == NULL || fault->device_idx >= ctx.num_devices) {
        return FWK_E_PARAM;
    }

    func = ctx.impl_apis[fault->device_idx]->set_enabled;
    if (func == NULL) {
        return FWK_E_SUPPORT;
    }
    config = ctx.device_config[fault->device_idx];

    return func(config, fault, enabled);
}

static int get_count(fwk_id_t id, uint16_t node_id, uint8_t *count)
{
    const struct mod_fmu_dev_config *config;
    unsigned int idx;
    int (*func)(
        const struct mod_fmu_dev_config *config,
        uint16_t node_id,
        uint8_t *count);

    idx = fwk_id_get_element_idx(id);
    if (count == NULL || idx >= ctx.num_devices) {
        return FWK_E_PARAM;
    }

    func = ctx.impl_apis[idx]->get_count;
    if (func == NULL) {
        return FWK_E_SUPPORT;
    }
    config = ctx.device_config[idx];

    return func(config, node_id, count);
}

static int set_count(fwk_id_t id, uint16_t node_id, uint8_t count)
{
    const struct mod_fmu_dev_config *config;
    unsigned int idx;
    int (*func)(
        const struct mod_fmu_dev_config *config,
        uint16_t node_id,
        uint8_t count);

    idx = fwk_id_get_element_idx(id);
    if (idx >= ctx.num_devices) {
        return FWK_E_PARAM;
    }

    func = ctx.impl_apis[idx]->set_count;
    if (func == NULL) {
        return FWK_E_SUPPORT;
    }
    config = ctx.device_config[idx];

    return func(config, node_id, count);
}

static int get_threshold(fwk_id_t id, uint16_t node_id, uint8_t *threshold)
{
    const struct mod_fmu_dev_config *config;
    unsigned int idx;
    int (*func)(
        const struct mod_fmu_dev_config *config,
        uint16_t node_id,
        uint8_t *threshold);

    idx = fwk_id_get_element_idx(id);
    if (threshold == NULL || idx >= ctx.num_devices) {
        return FWK_E_PARAM;
    }

    func = ctx.impl_apis[idx]->get_threshold;
    if (func == NULL) {
        return FWK_E_SUPPORT;
    }
    config = ctx.device_config[idx];

    return func(config, node_id, threshold);
}

static int set_threshold(fwk_id_t id, uint16_t node_id, uint8_t threshold)
{
    const struct mod_fmu_dev_config *config;
    unsigned int idx;
    int (*func)(
        const struct mod_fmu_dev_config *config,
        uint16_t node_id,
        uint8_t threshold);

    idx = fwk_id_get_element_idx(id);
    if (idx >= ctx.num_devices) {
        return FWK_E_PARAM;
    }

    func = ctx.impl_apis[idx]->set_threshold;
    if (func == NULL) {
        return FWK_E_SUPPORT;
    }
    config = ctx.device_config[idx];

    return func(config, node_id, threshold);
}

static int get_upgrade_enabled(fwk_id_t id, uint16_t node_id, bool *enabled)
{
    const struct mod_fmu_dev_config *config;
    unsigned int idx;
    int (*func)(
        const struct mod_fmu_dev_config *config,
        uint16_t node_id,
        bool *enabled);

    idx = fwk_id_get_element_idx(id);
    if (enabled == NULL || idx >= ctx.num_devices) {
        return FWK_E_PARAM;
    }

    func = ctx.impl_apis[idx]->get_upgrade_enabled;
    if (func == NULL) {
        return FWK_E_SUPPORT;
    }
    config = ctx.device_config[idx];

    return func(config, node_id, enabled);
}

static int set_upgrade_enabled(fwk_id_t id, uint16_t node_id, bool enabled)
{
    const struct mod_fmu_dev_config *config;
    unsigned int idx;
    int (*func)(
        const struct mod_fmu_dev_config *config,
        uint16_t node_id,
        bool enabled);

    idx = fwk_id_get_element_idx(id);
    if (idx >= ctx.num_devices) {
        return FWK_E_PARAM;
    }

    func = ctx.impl_apis[idx]->set_upgrade_enabled;
    if (func == NULL) {
        return FWK_E_SUPPORT;
    }
    config = ctx.device_config[idx];

    return func(config, node_id, enabled);
}

static int set_critical(const struct mod_fmu_fault *fault, bool critical)
{
    const struct mod_fmu_dev_config *config;
    int (*func)(
        const struct mod_fmu_dev_config *config,
        const struct mod_fmu_fault *fault,
        bool critical);

    if (fault == NULL || fault->device_idx >= ctx.num_devices) {
        return FWK_E_PARAM;
    }

    func = ctx.impl_apis[fault->device_idx]->set_critical;
    if (func == NULL) {
        return FWK_E_SUPPORT;
    }
    config = ctx.device_config[fault->device_idx];

    return func(config, fault, critical);
}

struct mod_fmu_api fmu_api = {
    .inject = inject,
    .get_enabled = get_enabled,
    .set_enabled = set_enabled,
    .get_count = get_count,
    .set_count = set_count,
    .get_threshold = get_threshold,
    .set_threshold = set_threshold,
    .get_upgrade_enabled = get_upgrade_enabled,
    .set_upgrade_enabled = set_upgrade_enabled,
    .set_critical = set_critical,
};

/*
 * Framework Handlers
 */
static int fmu_init(
    fwk_id_t module_id,
    unsigned int element_count,
    const void *data)
{
    fwk_assert(data != NULL);
    fwk_assert(element_count > 0);

    ctx.num_devices = element_count;
    ctx.device_config =
        fwk_mm_calloc(element_count, sizeof(struct mod_fmu_dev_config *));
    ctx.impl_apis =
        fwk_mm_calloc(element_count, sizeof(struct mod_fmu_impl_api *));

    return FWK_SUCCESS;
}

extern struct mod_fmu_impl_api mod_fmu_system_api;
extern struct mod_fmu_impl_api mod_fmu_gic_mhu_api;

struct mod_fmu_impl_api *implementation_apis[MOD_FMU_IMPL_COUNT] = {
    [MOD_FMU_SYSTEM_IMPL] = &mod_fmu_system_api,
    [MOD_FMU_GIC_MHU_IMPL] = &mod_fmu_gic_mhu_api,
};

static int fmu_device_init(
    fwk_id_t device_id,
    unsigned int unused,
    const void *data)
{
    unsigned int element_idx = fwk_id_get_element_idx(device_id);
    int (*configure)(const struct mod_fmu_config *config);
    fwk_id_t module_id;

    fwk_assert(data != NULL);
    fwk_assert(element_idx < ctx.num_devices);

    ctx.device_config[element_idx] = data;
    ctx.impl_apis[element_idx] =
        implementation_apis[ctx.device_config[element_idx]->implementation];
    fwk_assert(ctx.impl_apis[element_idx]->fault_peek != NULL);
    fwk_assert(ctx.impl_apis[element_idx]->fault_ack != NULL);

    configure = ctx.impl_apis[element_idx]->configure;

    if (configure == NULL) {
        return FWK_SUCCESS;
    }

    module_id = FWK_ID_MODULE(fwk_id_get_module_idx(device_id));
    return configure(fwk_module_get_data(module_id));
}

static int fmu_start(fwk_id_t id)
{
    int status;
    const struct mod_fmu_config *config = fwk_module_get_data(id);

    if (!fwk_id_is_type(id, FWK_ID_TYPE_MODULE)) {
        return FWK_SUCCESS;
    }

    status = fwk_interrupt_set_isr(config->irq_critical, &fmu_isr_critical);
    if (status != FWK_SUCCESS) {
        return status;
    }

    status =
        fwk_interrupt_set_isr(config->irq_non_critical, &fmu_isr_non_critical);
    if (status != FWK_SUCCESS) {
        return status;
    }

    status = fwk_interrupt_enable(config->irq_critical);
    if (status != FWK_SUCCESS) {
        return status;
    }

    status = fwk_interrupt_enable(config->irq_non_critical);
    if (status != FWK_SUCCESS) {
        return status;
    }

    return FWK_SUCCESS;
}

static int fmu_bind(fwk_id_t id, unsigned int round)
{
    unsigned int element_idx;
    int (*func)(fwk_id_t id);

    if ((round != 0) || (fwk_id_is_type(id, FWK_ID_TYPE_MODULE))) {
        return FWK_SUCCESS;
    }

    element_idx = fwk_id_get_element_idx(id);
    func = ctx.impl_apis[element_idx]->bind;

    if (func == NULL) {
        return FWK_SUCCESS;
    }

    return func(id);
}

static int fmu_process_bind_request(
    fwk_id_t requester_id,
    fwk_id_t target_id,
    fwk_id_t api_id,
    const void **api)
{
    switch (fwk_id_get_api_idx(api_id)) {
    case MOD_FMU_DEVICE_API_IDX:
        *api = &fmu_api;
        break;
    default:
        return FWK_E_PARAM;
    }
    return FWK_SUCCESS;
}

const struct fwk_module module_fmu = {
    .type = FWK_MODULE_TYPE_DRIVER,
    .init = fmu_init,
    .element_init = fmu_device_init,
    .start = fmu_start,
    .bind = fmu_bind,
    .api_count = MOD_FMU_API_COUNT,
    .process_bind_request = fmu_process_bind_request,
#ifdef BUILD_HAS_NOTIFICATION
    .notification_count = (unsigned int)MOD_FMU_NOTIFICATION_IDX_COUNT,
#endif
};
