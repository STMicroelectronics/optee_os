/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     IO Block module.
 */

#include <mod_atu.h>
#include <mod_io_block.h>
#include <mod_noc_s3.h>

#include <interface_io_block.h>

#include <fwk_assert.h>
#include <fwk_event.h>
#include <fwk_id.h>
#include <fwk_log.h>
#include <fwk_mm.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>
#include <fwk_notification.h>
#include <fwk_status.h>

#include <stddef.h>
#include <stdint.h>

#define MOD_NAME "[IO_BLOCK] "

/* Module Context */
struct io_block_context {
    /*! IO block module config. */
    const struct mod_io_block_config *config;
    /*! IO block element config. */
    const struct mod_io_block_element_config **element_config;
    /*! NCI driver map/unmap APIs. */
    struct mod_noc_s3_memmap_api *nci_memmap_api;
    /*! ATU map/unmap APIs. */
    struct mod_atu_api *atu_api;
};

static struct io_block_context ctx;

static int map_region_in_nci(
    uint64_t base,
    uint32_t target_id,
    uint32_t source_id,
    uint64_t address,
    uint64_t size,
    uint8_t *region_idx)
{
    struct mod_noc_s3_psam_region psam_regions = {
        .target_id = target_id,
        .base_address = address,
        .size = size,
    };
    struct mod_noc_s3_comp_config comp_config = {
        .type = MOD_NOC_S3_NODE_TYPE_ASNI,
        .id = source_id,
        .psam_regions = &psam_regions,
        .psam_region_count = 1,
    };
    struct mod_noc_s3_dev dev = {
        .periphbase = base,
    };

    return ctx.nci_memmap_api->map_region_in_psam(
        &dev, &comp_config, region_idx);
}

static int unmap_region_in_nci(
    uint64_t base,
    uint32_t source_id,
    uint8_t region_idx)
{
    struct mod_noc_s3_dev dev = {
        .periphbase = ctx.config->logical_region_base,
    };
    struct mod_noc_s3_comp_config comp_config = {
        .type = MOD_NOC_S3_NODE_TYPE_ASNI,
        .id = source_id,
    };

    return ctx.nci_memmap_api->unmap_region_in_psam(
        &dev, &comp_config, region_idx);
}

static int map_region(struct interface_io_block_setup_mmap *mmap)
{
    struct atu_region_map atu_map = {
        .region_owner_id = FWK_ID_MODULE(FWK_MODULE_IDX_IO_BLOCK),
        .attributes = ATU_ENCODE_ATTRIBUTES_ROOT_PAS,
        .log_addr_base = ctx.config->logical_region_base,
        .region_size = ctx.config->logical_region_size,
    };
    uint8_t region_idx;
    size_t idx;
    int status;

    if (mmap == NULL) {
        return FWK_E_PARAM;
    }
    atu_map.phy_addr_base = mmap->io_block_address;

    status = ctx.atu_api->add_region(
        &atu_map, FWK_ID_ELEMENT(FWK_MODULE_IDX_ATU, 0), &region_idx);
    if (status != FWK_SUCCESS) {
        return status;
    }

    for (idx = 0; idx < mmap->region_count; idx++) {
        status = map_region_in_nci(
            atu_map.log_addr_base,
            mmap->carveout_info[idx].target_id,
            mmap->source_id,
            mmap->carveout_info[idx].base,
            mmap->carveout_info[idx].size,
            &mmap->carveout_info[idx].region_id);
        if (status != FWK_SUCCESS) {
            return status;
        }
    }

    status = ctx.atu_api->remove_region(
        region_idx,
        FWK_ID_ELEMENT(FWK_MODULE_IDX_ATU, 0),
        atu_map.region_owner_id);

    return status;
}

static int unmap_region(struct interface_io_block_setup_mmap *mmap)
{
    struct atu_region_map atu_map = {
        .region_owner_id = FWK_ID_MODULE(FWK_MODULE_IDX_IO_BLOCK),
        .attributes = ATU_ENCODE_ATTRIBUTES_ROOT_PAS,
        .log_addr_base = ctx.config->logical_region_base,
        .region_size = ctx.config->logical_region_size,
    };
    uint8_t region_idx;
    size_t idx;
    int status;

    if (mmap == NULL) {
        return FWK_E_PARAM;
    }
    atu_map.phy_addr_base = mmap->io_block_address;

    status = ctx.atu_api->add_region(
        &atu_map, FWK_ID_ELEMENT(FWK_MODULE_IDX_ATU, 0), &region_idx);
    if (status != FWK_SUCCESS) {
        return status;
    }

    for (idx = 0; idx < mmap->region_count; idx++) {
        status = unmap_region_in_nci(
            atu_map.log_addr_base,
            mmap->source_id,
            mmap->carveout_info[idx].region_id);
        if (status != FWK_SUCCESS) {
            return status;
        }
    }

    status = ctx.atu_api->remove_region(
        region_idx,
        FWK_ID_ELEMENT(FWK_MODULE_IDX_ATU, 0),
        atu_map.region_owner_id);

    return status;
}

static struct interface_io_block_memmap_api memmap_api = {
    .map_region = map_region,
    .unmap_region = unmap_region,
};

static int map_static_mappings(unsigned int element_idx)
{
    const struct mod_io_block_element_config *element_config;
    struct atu_region_map atu_map;
    uint32_t mapping_idx;

    atu_map.region_owner_id = FWK_ID_MODULE(FWK_MODULE_IDX_IO_BLOCK);
    atu_map.attributes = ATU_ENCODE_ATTRIBUTES_ROOT_PAS;
    atu_map.log_addr_base = ctx.config->logical_region_base;
    atu_map.region_size = ctx.config->logical_region_size;

    element_config = ctx.element_config[element_idx];

    for (mapping_idx = 0; mapping_idx < element_config->region_mem_maps_count;
         mapping_idx++) {
        struct interface_io_block_setup_mmap *mapping;
        uint32_t region_idx;
        uint8_t atu_region_idx;
        uint8_t nci_region_idx;
        int status;

        atu_map.phy_addr_base =
            element_config->region_mem_maps[mapping_idx].io_block_address;

        status = ctx.atu_api->add_region(
            &atu_map, FWK_ID_ELEMENT(FWK_MODULE_IDX_ATU, 0), &atu_region_idx);
        if (status != FWK_SUCCESS) {
            return status;
        }

        mapping = &element_config->region_mem_maps[mapping_idx];
        for (region_idx = 0; region_idx < mapping->region_count; region_idx++) {
            status = map_region_in_nci(
                atu_map.log_addr_base,
                mapping->carveout_info[region_idx].target_id,
                mapping->source_id,
                mapping->carveout_info[region_idx].base,
                mapping->carveout_info[region_idx].size,
                &nci_region_idx);
            if (status != FWK_SUCCESS) {
                return status;
            }
        }

        status = ctx.atu_api->remove_region(
            atu_region_idx,
            FWK_ID_ELEMENT(FWK_MODULE_IDX_ATU, 0),
            atu_map.region_owner_id);
        if (status != FWK_SUCCESS) {
            return status;
        }
    }

    return FWK_SUCCESS;
}

/*
 * Framework Handlers.
 */
static int mod_io_block_init(
    fwk_id_t module_id,
    unsigned int element_count,
    const void *config)
{
    ctx.config = config;
    if (element_count == 0) {
        return FWK_SUCCESS;
    }

    ctx.element_config = fwk_mm_calloc(
        element_count, sizeof(struct mod_io_block_element_config));

    return FWK_SUCCESS;
}

static int mod_io_block_element_init(
    fwk_id_t element_id,
    unsigned int unused,
    const void *data)
{
    const struct mod_io_block_element_config *element_config;

    element_config = (struct mod_io_block_element_config *)data;
    if ((element_config->region_mem_maps_count == 0) ||
        (element_config->region_mem_maps == NULL)) {
        fwk_unexpected();
        return FWK_E_DATA;
    }

    ctx.element_config[fwk_id_get_element_idx(element_id)] = element_config;

    return FWK_SUCCESS;
}

static int mod_io_block_bind(fwk_id_t id, unsigned int round)
{
    int status;

    status = fwk_module_bind(
        ctx.config->atu_id, ctx.config->atu_api_id, &ctx.atu_api);
    if (status != FWK_SUCCESS) {
        return status;
    }

    return fwk_module_bind(
        ctx.config->nci_id, ctx.config->nci_api_id, &ctx.nci_memmap_api);
}

static int mod_io_block_start(fwk_id_t id)
{
    /* Nothing to be done for module start call */
    if (fwk_id_get_type(id) == FWK_ID_TYPE_MODULE) {
        return FWK_SUCCESS;
    }

    if (fwk_id_type_is_valid(ctx.config->plat_notification.source_id)) {
        /*
         * Bind to the platform notification that is needed to start module
         * configuration.
         */
        return fwk_notification_subscribe(
            ctx.config->plat_notification.notification_id,
            ctx.config->plat_notification.source_id,
            id);
    }

    return map_static_mappings(fwk_id_get_element_idx(id));
}

static int mod_io_block_process_bind_request(
    fwk_id_t requester_id,
    fwk_id_t id,
    fwk_id_t api_id,
    const void **api)
{
    int status;
    enum mod_io_block_api_idx api_idx;

    status = FWK_SUCCESS;
    api_idx = (enum mod_io_block_api_idx)fwk_id_get_api_idx(api_id);

    switch (api_idx) {
    case MOD_IO_BLOCK_API_IDX_MAP_REGION:
        *api = &memmap_api;
        break;
    default:
        status = FWK_E_PARAM;
        break;
    };

    return status;
}

static int mod_io_block_process_notification(
    const struct fwk_event *event,
    struct fwk_event *resp_event)
{
    unsigned int element_idx;
    int status;

    if (fwk_id_is_type(event->target_id, FWK_ID_TYPE_MODULE)) {
        return FWK_E_PARAM;
    }

    if (fwk_id_is_equal(
            event->id, ctx.config->plat_notification.notification_id)) {
        status = fwk_notification_unsubscribe(
            event->id, event->source_id, event->target_id);
        if (status != FWK_SUCCESS) {
            return status;
        }

        element_idx = fwk_id_get_element_idx(event->target_id);
        status = map_static_mappings(element_idx);
        if (status != FWK_SUCCESS) {
            return status;
        }
    }

    return FWK_SUCCESS;
}

const struct fwk_module module_io_block = {
    .type = FWK_MODULE_TYPE_HAL,
    .init = mod_io_block_init,
    .api_count = MOD_IO_BLOCK_API_IDX_MAX,
    .element_init = mod_io_block_element_init,
    .bind = mod_io_block_bind,
    .start = mod_io_block_start,
    .process_bind_request = mod_io_block_process_bind_request,
    .process_notification = mod_io_block_process_notification,
};
