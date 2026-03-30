/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <mod_fmu.h>
#include <mod_sbistc.h>

#include <fwk_assert.h>
#include <fwk_id.h>
#include <fwk_log.h>
#include <fwk_module.h>
#include <fwk_notification.h>

#include <stddef.h>
#include <stdint.h>

#define MOD_NAME "[SBISTC] "

static struct mod_fmu_api *fmu_api = NULL;
static struct mod_sbistc_config *sbistc_config = NULL;

#define NON_CRITICAL (false)
#define CRITICAL     (true)

/*
 * Exported APIs
 */

static int sbistc_get_count(uint8_t fault_id, uint8_t *count)
{
    const struct sbistc_fault_config *cfg;

    if (!sbistc_config || !fmu_api || !count)
        return FWK_E_PARAM;

    if (fault_id >= sbistc_config->count)
        return FWK_E_PARAM;

    cfg = &sbistc_config->flt_cfgs[fault_id];
    fwk_assert(cfg != NULL);

    fwk_id_t fmu_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_FMU, cfg->fmu_device_id);
    return fmu_api->get_count(fmu_id, cfg->fmu_node_id, count);
}

static int enable_fmu_parent_chain(uint8_t fmu_device_id, bool critical)
{
    fwk_id_t fmu_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_FMU, fmu_device_id);
    const struct mod_fmu_dev_config *current =
        (const struct mod_fmu_dev_config *)fwk_module_get_data(fmu_id);

    while (current && (current->parent != MOD_FMU_PARENT_NONE)) {
        struct mod_fmu_fault parent_fault = {
            .device_idx = current->parent,
            .node_idx = (critical) ? current->parent_cr_index :
                                     current->parent_ncr_index,
            .sm_idx = MOD_FMU_SM_ALL,
        };

        int status = fmu_api->set_enabled(&parent_fault, true);
        if (status != FWK_SUCCESS)
            return status;

        /* Move up the tree */
        fmu_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_FMU, current->parent);
        current =
            (const struct mod_fmu_dev_config *)fwk_module_get_data(fmu_id);
    }
    return FWK_SUCCESS;
}

static int sbistc_set_enabled(uint8_t fault_id, bool enable)
{
    const struct sbistc_fault_config *cfg;
    struct mod_fmu_fault fault;
    int status;

    if (!sbistc_config || !fmu_api)
        return FWK_E_PARAM;

    if (fault_id >= sbistc_config->count)
        return FWK_E_PARAM;

    cfg = &sbistc_config->flt_cfgs[fault_id];
    fwk_assert(cfg != NULL);

    /* Always enable/disable the leaf FMU node for this fault */
    fault.device_idx = cfg->fmu_device_id;
    fault.node_idx = cfg->fmu_node_id;
    fault.sm_idx = MOD_FMU_SM_ALL;

    status = fmu_api->set_enabled(&fault, enable);
    if (status != FWK_SUCCESS)
        return status;

    /**
     * If enabling, walk up the parent chain and enable each parent node
     * But if disabling, we do not need to disable parents as there are
     * other faults which might be active on the same parent.
     */
    if (enable) {
        status = enable_fmu_parent_chain(cfg->fmu_device_id, NON_CRITICAL);
        if (status != FWK_SUCCESS)
            return status;
    }

    return FWK_SUCCESS;
}

static int sbistc_set_handler(uint8_t fault_idx, void (*handler)(void))
{
    if (!sbistc_config)
        return FWK_E_PARAM;

    if (fault_idx >= sbistc_config->count)
        return FWK_E_PARAM;

    sbistc_config->flt_cfgs[fault_idx].handler = handler;
    return FWK_SUCCESS;
}

/* Exported API instance */
static const struct mod_sbistc_api sbistc_api = {
    .get_count = sbistc_get_count,
    .set_enabled = sbistc_set_enabled,
    .set_handler = sbistc_set_handler,
};

/*
 * Framework Handlers
 */

static int sbistc_init(
    fwk_id_t module_id,
    unsigned int element_count,
    const void *data)
{
    if (data == NULL)
        return FWK_E_PARAM;

    sbistc_config = (struct mod_sbistc_config *)data;
    FWK_LOG_INFO(MOD_NAME "SBISTC module initialized");
    return FWK_SUCCESS;
}

static int sbistc_bind(fwk_id_t id, unsigned int round)
{
    int status = FWK_SUCCESS;
    fwk_id_t fmu_api_id;

    /* Bind to FMU APIs */
    if (round == 0) {
        fmu_api_id = FWK_ID_API(FWK_MODULE_IDX_FMU, MOD_FMU_DEVICE_API_IDX);
        status = fwk_module_bind(
            FWK_ID_MODULE(FWK_MODULE_IDX_FMU),
            fmu_api_id,
            (const void **)&fmu_api);
    }
    return status;
}

static int sbistc_start(fwk_id_t id)
{
    int status = FWK_SUCCESS;

    if (fmu_api == NULL) {
        FWK_LOG_ERR(MOD_NAME "FMU API not bound");
        return FWK_E_STATE;
    }

    /* Enable all SBISTC faults by default */
    for (unsigned int idx = 0; idx < sbistc_config->count; ++idx) {
        status = sbistc_set_enabled(idx, true);
        if (status != FWK_SUCCESS) {
            FWK_LOG_ERR(
                MOD_NAME "Failed to enable fault %u (status=%d)", idx, status);
            return status;
        }
    }
    FWK_LOG_INFO(MOD_NAME "All SBISTC faults enabled by default");

#ifdef BUILD_HAS_NOTIFICATION
    if (!fwk_id_is_type(id, FWK_ID_TYPE_MODULE)) {
        return FWK_SUCCESS;
    }
    /* Subscribe to FMU fault notifications */
    status = fwk_notification_subscribe(
        mod_fmu_notification_id_fault, FWK_ID_MODULE(FWK_MODULE_IDX_FMU), id);
    if (status != FWK_SUCCESS) {
        FWK_LOG_ERR(MOD_NAME "Failed to subscribe to FMU fault notification");
    } else {
        FWK_LOG_INFO(MOD_NAME "Subscribed to FMU fault notifications");
    }
#endif /* BUILD_HAS_NOTIFICATION */

    return status;
}

#ifdef BUILD_HAS_NOTIFICATION
static int sbistc_process_notification(
    const struct fwk_event *event,
    struct fwk_event *resp_event)
{
    struct mod_fmu_fault_notification_params *params;
    struct sbistc_fault_config *cfg;

    if (event == NULL || !sbistc_config)
        return FWK_E_PARAM;

    fwk_assert(fwk_id_is_type(event->target_id, FWK_ID_TYPE_MODULE));

    if (fwk_id_is_equal(event->id, mod_fmu_notification_id_fault)) {
        params = (struct mod_fmu_fault_notification_params *)event->params;

        /* Find the fault config for this fault and call handler if set */
        for (unsigned int idx = 0; idx < sbistc_config->count; ++idx) {
            cfg = &sbistc_config->flt_cfgs[idx];
            fwk_assert(cfg != NULL);
            if (cfg->fmu_device_id == params->fault.device_idx &&
                cfg->fmu_node_id == params->fault.node_idx) {
                FWK_LOG_INFO(
                    MOD_NAME "%s detected (FMU dev %u, node %u)",
                    cfg->flt_name,
                    cfg->fmu_device_id,
                    cfg->fmu_node_id);

                if (cfg->handler) {
                    cfg->handler();
                }
            }
        }
    }
    return FWK_SUCCESS;
}
#endif /* BUILD_HAS_NOTIFICATION */

static int sbistc_process_bind_request(
    fwk_id_t source_id,
    fwk_id_t target_id,
    fwk_id_t api_id,
    const void **api)
{
    *api = &sbistc_api;
    return FWK_SUCCESS;
}

const struct fwk_module module_sbistc = {
    .type = FWK_MODULE_TYPE_DRIVER,
    .init = sbistc_init,
    .start = sbistc_start,
    .api_count = MOD_SBISTC_API_IDX_COUNT,
    .bind = sbistc_bind,
    .process_bind_request = sbistc_process_bind_request,
#ifdef BUILD_HAS_NOTIFICATION
    .process_notification = sbistc_process_notification,
#endif /* BUILD_HAS_NOTIFICATION */
};
