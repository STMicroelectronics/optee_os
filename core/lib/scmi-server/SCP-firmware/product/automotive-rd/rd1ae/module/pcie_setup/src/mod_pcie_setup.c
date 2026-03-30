/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     RD-1 AE PCIe Setup.
 */

#include <mod_pcie_discovery.h>
#include <mod_pcie_setup.h>
#include <mod_sds.h>
#include <mod_system_info.h>

#include <interface_address_remapper.h>
#include <interface_cmn.h>
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

#include <fmw_cmsis.h>

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>

#define MOD_NAME "[PCIE_SETUP] "

struct mod_pcie_setup_context {
    /* Pointer to the element configuration pointers (table) */
    struct mod_pcie_setup_config **config;

    /* APIs required to do PCIe bus walk and resource discovery */
    struct mod_pcie_discovery_api *pcie_discovery_api;

    /* Pointer to the module config. */
    const void *module_config;

    /* Pointer to the chip specific module info. */
    struct mod_pcie_setup_resource_info resource_info;

    /* Pointer to system info APIs */
    struct mod_system_info_get_info_api *system_info_api;

    /* Pointer to system info configuration. */
    const struct mod_system_info *system_info;

    /* API to program PCIe memory region in GVP NCI. */
    struct interface_io_block_memmap_api *io_block_memmap_api;

    /* APIs to configure CMN moudle. */
    struct interface_cmn_memmap_rnsam_api *cmn_memmap_rnsam_api;

    /* AP remap Address read write APIs */
    struct interface_address_remapper_rw_api *remapper_rw_api;

    /* APIs to program PCIe memory map configuration info in SDS. */
    struct mod_sds_api *sds_api;

    /*
     * Size of block config. This depends on the number of rootport in
     * the block.
     */
    size_t block_config_size;

    /* Number of IO Blocks . */
    size_t block_count;
};

static struct mod_pcie_setup_context pcie_setup_context;

static int pcie_setup_send_initialised_notification(void)
{
    struct fwk_event notification_event = { 0 };
    unsigned int notification_count;

    notification_event.id = mod_pcie_setup_notification_initialised;
    notification_event.source_id = fwk_module_id_pcie_setup;

    FWK_LOG_INFO(MOD_NAME
                 "Sending PCIe initialisation completion notification.");
    return fwk_notification_notify(&notification_event, &notification_count);
}

static int pcie_setup_update_sds(
    struct mod_pcie_setup_block_mmap *block_info,
    struct mod_pcie_setup_config *config)
{
    static uint32_t offset = sizeof(struct mod_pcie_setup_block_mmap_list);
    static uint64_t count = 0;
    static uint64_t table_size = sizeof(struct mod_pcie_setup_block_mmap_list);
    size_t block_size;
    int status;

    count++;
    status = pcie_setup_context.sds_api->struct_write(
        config->sds_struct_id,
        offsetof(struct mod_pcie_setup_block_mmap_list, block_count),
        &count,
        sizeof(((struct mod_pcie_setup_block_mmap_list *)0)->block_count));
    if (status != FWK_SUCCESS) {
        FWK_LOG_ERR("Block count: SDS Update failed, error:%d", status);
        return status;
    }

    block_size = sizeof(struct mod_pcie_setup_block_mmap) +
        (block_info->ep_count * sizeof(struct mod_pcie_setup_mmap));

    table_size += block_size;
    status = pcie_setup_context.sds_api->struct_write(
        config->sds_struct_id,
        offsetof(struct mod_pcie_setup_block_mmap_list, table_size),
        &table_size,
        sizeof(((struct mod_pcie_setup_block_mmap_list *)0)->table_size));
    if (status != FWK_SUCCESS) {
        FWK_LOG_ERR("Table Size: SDS Update failed, error:%d", status);
        return status;
    }

    status = pcie_setup_context.sds_api->struct_write(
        config->sds_struct_id, offset, block_info, block_size);
    offset += block_size;
    if (status != FWK_SUCCESS) {
        FWK_LOG_ERR("PCIe Table: SDS Update failed, error:%d", status);
    }

    return status;
}

static void allocate_ecam_address(
    struct mod_pcie_setup_mmap *mmap,
    struct pcie_mmap_size *mmap_size)
{
    struct mod_pcie_setup_resource_info *res_info;

    res_info = &pcie_setup_context.resource_info;

    if (mmap_size->ecam != 0) {
        if (mmap_size->ecam > res_info->mmap.ecam.size) {
            FWK_LOG_ERR(MOD_NAME "No memory left to allocate for ECAM");
            fwk_unexpected();
        }
        mmap->ecam.start = res_info->mmap.ecam.start;
        mmap->ecam.size = mmap_size->ecam;
        res_info->mmap.ecam.start += mmap_size->ecam;
        res_info->mmap.ecam.size -= mmap_size->ecam;
    } else {
        /*
         * Reset the values to zero to prevent caller from using garbage values.
         */
        mmap->ecam.start = 0;
        mmap->ecam.size = 0;
    }
}

static void allocate_mmiol_address(
    struct mod_pcie_setup_mmap *mmap,
    struct pcie_mmap_size *mmap_size)
{
    struct mod_pcie_setup_resource_info *res_info;

    res_info = &pcie_setup_context.resource_info;

    if (mmap_size->mmiol != 0) {
        if (mmap_size->mmiol > res_info->mmap.mmiol.size) {
            FWK_LOG_ERR(MOD_NAME "No memory left to allocate for MMIOL");
            fwk_unexpected();
        }
        mmap->mmiol.start = res_info->mmap.mmiol.start;
        mmap->mmiol.size = mmap_size->mmiol;
        res_info->mmap.mmiol.start += mmap_size->mmiol;
        res_info->mmap.mmiol.size -= mmap_size->mmiol;
    } else {
        /*
         * Reset the values to zero to prevent caller from using garbage values.
         */
        mmap->mmiol.start = 0;
        mmap->mmiol.size = 0;
    }
}

static void allocate_mmioh_address(
    struct mod_pcie_setup_mmap *mmap,
    struct pcie_mmap_size *mmap_size)
{
    struct mod_pcie_setup_resource_info *res_info;

    res_info = &pcie_setup_context.resource_info;

    if (mmap_size->mmioh != 0) {
        if (mmap_size->mmioh > res_info->mmap.mmioh.size) {
            FWK_LOG_ERR(MOD_NAME "No memory left to allocate for MMIOH");
            fwk_unexpected();
        }
        mmap->mmioh.start = res_info->mmap.mmioh.start;
        mmap->mmioh.size = mmap_size->mmioh;
        res_info->mmap.mmioh.start += mmap_size->mmioh;
        res_info->mmap.mmioh.size -= mmap_size->mmioh;
    } else {
        /*
         * Reset the values to zero to prevent caller from using garbage values.
         */
        mmap->mmioh.start = 0;
        mmap->mmioh.size = 0;
    }
}

static void allocate_bus(
    struct mod_pcie_setup_mmap *mmap,
    struct pcie_mmap_size *mmap_size)
{
    struct mod_pcie_setup_resource_info *res_info;

    res_info = &pcie_setup_context.resource_info;

    if (mmap_size->bus != 0) {
        if (mmap_size->bus > res_info->mmap.bus.size) {
            FWK_LOG_ERR(MOD_NAME "No bus left to allocate");
            fwk_unexpected();
        }
        mmap->bus.start = res_info->mmap.bus.start;
        mmap->bus.size = mmap_size->bus;
        res_info->mmap.bus.start += mmap_size->bus;
        res_info->mmap.bus.size -= mmap_size->bus;
    } else {
        /*
         * Reset the values to zero to prevent caller from using garbage values.
         */
        mmap->bus.start = 0;
        mmap->bus.size = 0;
    }
}

/*
 * This takes in the size required for each of the region and then allocates
 * regions for them from platform provided configuration.
 */
static inline void get_address_range(
    struct mod_pcie_setup_mmap *mmap,
    struct pcie_mmap_size *mmap_size)
{
    allocate_ecam_address(mmap, mmap_size);
    allocate_mmiol_address(mmap, mmap_size);
    allocate_mmioh_address(mmap, mmap_size);
    allocate_bus(mmap, mmap_size);
}

static int map_region_in_io_block(
    enum interface_io_block_carveout_type carveout_type,
    struct mod_pcie_setup_config *config,
    uint64_t address,
    uint64_t size,
    uint8_t index,
    uint8_t *region)
{
    struct interface_io_block_carveout_info carveout_info = {
        .base = address,
        .size = size,
        .carveout_type = carveout_type,
        .target_id = config->ep_config[index].rp_node_id
    };
    struct interface_io_block_setup_mmap mmap_info = {
        .io_block_address = config->reg_base,
        .carveout_info = &carveout_info,
        .region_count = 1,
        .source_id = config->nci_source_node_id,
    };
    int status;

    status = pcie_setup_context.io_block_memmap_api->map_region(&mmap_info);
    if (status == FWK_SUCCESS) {
        *region = carveout_info.region_id;
    }

    return status;
}

static int unmap_region_in_io_block(
    struct mod_pcie_setup_config *config,
    uint8_t region)
{
    struct interface_io_block_carveout_info carveout_info = {
        .region_id = region,
    };
    struct interface_io_block_setup_mmap mmap_info = {
        .io_block_address = config->reg_base,
        .carveout_info = &carveout_info,
        .region_count = 1,
        .source_id = config->nci_source_node_id,
    };
    int status;

    status = pcie_setup_context.io_block_memmap_api->unmap_region(&mmap_info);

    return status;
}

static int discover_and_configure_pcie_device(
    struct mod_pcie_setup_config *config,
    uint8_t index,
    struct mod_pcie_setup_mmap *mmap)
{
    struct interface_address_remapper_rw_api *remapper_rw_api;
    struct mod_pcie_discovery_api *pcie_discovery_api;
    struct mod_pcie_setup_resource_info *res_info;
    struct pcie_discovery_rw_api pcie_rw_api;
    struct pcie_mmap_size mmap_size = { 0 };
    uint64_t ecam_adjusted_address;
    uint8_t region;
    int status;

    res_info = &pcie_setup_context.resource_info;

    status = map_region_in_io_block(
        INTERFACE_IO_BLOCK_CARVEOUT_TYPE_ECAM,
        config,
        res_info->mmap.ecam.start,
        res_info->mmap.ecam.size,
        index,
        &region);
    if (status != FWK_SUCCESS) {
        FWK_LOG_ERR(MOD_NAME "NCI mapping failed");
        return status;
    }

    remapper_rw_api = pcie_setup_context.remapper_rw_api;
    pcie_rw_api.read8 = remapper_rw_api->read8;
    pcie_rw_api.write8 = remapper_rw_api->write8;
    pcie_rw_api.read32 = remapper_rw_api->read32;
    pcie_rw_api.write32 = remapper_rw_api->write32;

    ecam_adjusted_address = res_info->mmap.ecam.start;

    if (res_info->mmap.ecam.start < res_info->translation) {
        ecam_adjusted_address += res_info->translation;
    }
    pcie_discovery_api = pcie_setup_context.pcie_discovery_api;
    pcie_discovery_api->calculate_resource(
        ecam_adjusted_address - (res_info->mmap.bus.start * FWK_MIB),
        &mmap_size,
        res_info->mmap.bus.start,
        &pcie_rw_api);
    get_address_range(mmap, &mmap_size);

    status = unmap_region_in_io_block(config, region);
    if (status != FWK_SUCCESS) {
        return status;
    }

    if (mmap->ecam.size != 0) {
        status = map_region_in_io_block(
            INTERFACE_IO_BLOCK_CARVEOUT_TYPE_ECAM,
            config,
            mmap->ecam.start,
            mmap->ecam.size,
            index,
            &region);
        if (status != FWK_SUCCESS) {
            FWK_LOG_ERR(MOD_NAME "ECAM Mapping in NCI failed");
            return status;
        }
    }

    if (mmap->mmiol.size != 0) {
        status = map_region_in_io_block(
            INTERFACE_IO_BLOCK_CARVEOUT_TYPE_MMIOL,
            config,
            mmap->mmiol.start,
            mmap->mmiol.size,
            index,
            &region);
        if (status != FWK_SUCCESS) {
            FWK_LOG_ERR(MOD_NAME "MMIOL Mapping in NCI failed");
            return status;
        }
    }

    if (mmap->mmioh.size != 0) {
        status = map_region_in_io_block(
            INTERFACE_IO_BLOCK_CARVEOUT_TYPE_MMIOH,
            config,
            mmap->mmioh.start,
            mmap->mmioh.size,
            index,
            &region);
        if (status != FWK_SUCCESS) {
            FWK_LOG_ERR(MOD_NAME "MMIOH Mapping in NCI failed");
            return status;
        }
    }

    return FWK_SUCCESS;
}

static int configure_pcie_endpoint(
    struct mod_pcie_setup_config *config,
    struct mod_pcie_setup_block_mmap *block_mmap)
{
    struct interface_cmn_memmap_rnsam_api *cmn_memmap_rnsam_api;
    struct mod_pcie_setup_resource_info *res_info;
    uint64_t ecam_start;
    uint64_t mmiol_start;
    uint64_t mmioh_start;
    size_t ecam_size;
    size_t mmiol_size;
    size_t mmioh_size;
    uint8_t index;
    int status;

    res_info = &pcie_setup_context.resource_info;

    ecam_start = res_info->mmap.ecam.start;
    mmiol_start = res_info->mmap.mmiol.start;
    mmioh_start = res_info->mmap.mmioh.start;
    ecam_size = 0;
    mmiol_size = 0;
    mmioh_size = 0;

    cmn_memmap_rnsam_api = pcie_setup_context.cmn_memmap_rnsam_api;
    status = cmn_memmap_rnsam_api->map_io_region(
        res_info->mmap.ecam.start,
        res_info->mmap.ecam.size,
        config->cmn_node_id);
    if (status != FWK_SUCCESS) {
        return status;
    }

    for (index = 0; index < res_info->ep_count; index++) {
        if (config->ep_config[index].valid) {
            struct mod_pcie_setup_mmap *ep_mmap =
                &block_mmap->ep[block_mmap->ep_count];
            block_mmap->ep_count++;
            status = discover_and_configure_pcie_device(config, index, ep_mmap);
            if (status != FWK_SUCCESS) {
                return status;
            }
            ecam_size += ep_mmap->ecam.size;
            mmiol_size += ep_mmap->mmiol.size;
            mmioh_size += ep_mmap->mmioh.size;
            ep_mmap->interrupt_id = res_info->ep_interrupt_ids[index];
            block_mmap->segment = pcie_setup_context.system_info->chip_id;
            block_mmap->translation = res_info->translation;
            block_mmap->smmu_base = config->smmu_base;
        }
    }

    status = pcie_setup_update_sds(block_mmap, config);
    if (status != FWK_SUCCESS) {
        return status;
    }

    if (ecam_size != 0) {
        status = cmn_memmap_rnsam_api->map_io_region(
            ecam_start, ecam_size, config->cmn_node_id);
        if (status != FWK_SUCCESS) {
            return status;
        }
    }

    if (mmiol_size != 0) {
        status = cmn_memmap_rnsam_api->map_io_region(
            mmiol_start, mmiol_size, config->cmn_node_id);
        if (status != FWK_SUCCESS) {
            return status;
        }
    }

    if (mmioh_size != 0) {
        status = cmn_memmap_rnsam_api->map_io_region(
            mmioh_start, mmioh_size, config->cmn_node_id);
        if (status != FWK_SUCCESS) {
            return status;
        }
    }

    return FWK_SUCCESS;
}

static int configure_pcie_ecam_mmio_space(
    struct mod_pcie_setup_config *config,
    unsigned int element)
{
    struct mod_pcie_setup_block_mmap *block_mmap;
    unsigned int chip_id;
    int status = FWK_SUCCESS;

    FWK_LOG_INFO(MOD_NAME "Configuring PCIe: %d", element);
    block_mmap = fwk_mm_calloc(pcie_setup_context.block_config_size, 1);
    if (block_mmap == NULL) {
        return FWK_E_NOMEM;
    }
    chip_id = pcie_setup_context.system_info->chip_id;
    block_mmap->id =
        config->block_id + (chip_id * pcie_setup_context.block_count);
    status = configure_pcie_endpoint(config, block_mmap);
    fwk_mm_free(block_mmap);

    if (status != FWK_SUCCESS) {
        FWK_LOG_ERR(MOD_NAME "PCIe(%d) Setup Failed.", element);
        return status;
    }

    if (element == (pcie_setup_context.block_count - 1)) {
        status = pcie_setup_send_initialised_notification();
    }

    return status;
}

/* Framework APIs. */
static int mod_pcie_setup_init(
    fwk_id_t module_id,
    unsigned int block_count,
    const void *data)
{
    const struct mod_pcie_setup_module_config *module_config;
    size_t block_config_size;

    module_config = data;
    if ((module_config == NULL) || (module_config->resource_info_count == 0)) {
        return FWK_E_PARAM;
    }

    /*
     * Copy first instance of the resource info for binding.
     */
    pcie_setup_context.module_config = data;
    memcpy(
        &pcie_setup_context.resource_info,
        module_config->resource_info,
        sizeof(pcie_setup_context.resource_info));

    if (block_count == 0) {
        return FWK_SUCCESS;
    }

    pcie_setup_context.block_count = block_count;
    pcie_setup_context.config =
        fwk_mm_calloc(block_count, sizeof(struct mod_pcie_setup_config *));
    if (pcie_setup_context.config == NULL) {
        return FWK_E_NOMEM;
    }

    block_config_size = sizeof(struct mod_pcie_setup_block_mmap) +
        (sizeof(struct mod_pcie_setup_mmap) *
         pcie_setup_context.resource_info.ep_count);
    pcie_setup_context.block_config_size = block_config_size;

    return FWK_SUCCESS;
}

static int mod_pcie_setup_element_init(
    fwk_id_t element_id,
    unsigned int unused,
    const void *data)
{
    struct mod_pcie_setup_config *config;

    config = (struct mod_pcie_setup_config *)data;
    if ((config == NULL) || (config->reg_base == 0)) {
        fwk_unexpected();
        return FWK_E_DATA;
    }

    pcie_setup_context.config[fwk_id_get_element_idx(element_id)] = config;

    return FWK_SUCCESS;
}

static int mod_pcie_setup_bind(fwk_id_t id, unsigned int round)
{
    struct mod_pcie_setup_resource_info *res_info;
    int status;

    res_info = &pcie_setup_context.resource_info;

    status = fwk_module_bind(
        res_info->atu_mmio_id,
        res_info->atu_mmio_api_id,
        &pcie_setup_context.remapper_rw_api);
    if (status != FWK_SUCCESS) {
        return status;
    }

    status = fwk_module_bind(
        res_info->cmn_id,
        res_info->cmn_api_id,
        &pcie_setup_context.cmn_memmap_rnsam_api);
    if (status != FWK_SUCCESS) {
        return status;
    }

    status = fwk_module_bind(
        FWK_ID_MODULE(FWK_MODULE_IDX_SYSTEM_INFO),
        FWK_ID_API(FWK_MODULE_IDX_SYSTEM_INFO, MOD_SYSTEM_INFO_GET_API_IDX),
        &pcie_setup_context.system_info_api);
    if (status != FWK_SUCCESS) {
        return status;
    }

    status = fwk_module_bind(
        res_info->io_block_id,
        res_info->io_block_api_id,
        &pcie_setup_context.io_block_memmap_api);
    if (status != FWK_SUCCESS) {
        return status;
    }

    status = fwk_module_bind(
        FWK_ID_MODULE(FWK_MODULE_IDX_PCIE_DISCOVERY),
        FWK_ID_API(
            FWK_MODULE_IDX_PCIE_DISCOVERY,
            MOD_PCIE_DISCOVERY_API_IDX_CALCULATE_RESOURCES),
        &pcie_setup_context.pcie_discovery_api);
    if (status != FWK_SUCCESS) {
        return status;
    }

    if (pcie_setup_context.sds_api == NULL) {
        status = fwk_module_bind(
            FWK_ID_MODULE(FWK_MODULE_IDX_SDS),
            FWK_ID_API(FWK_MODULE_IDX_SDS, 0),
            &pcie_setup_context.sds_api);
        if (status != FWK_SUCCESS) {
            return status;
        }
    }

    return status;
}

static int mod_pcie_setup_start(fwk_id_t id)
{
    struct mod_pcie_setup_resource_info *res_info;
    unsigned int element;
    int status;

    if (fwk_id_is_type(id, FWK_ID_TYPE_MODULE)) {
        const struct mod_pcie_setup_module_config *module_config;
        unsigned int chip_id;

        status = pcie_setup_context.system_info_api->get_system_info(
            &pcie_setup_context.system_info);
        if (status != FWK_SUCCESS) {
            return status;
        }

        chip_id = pcie_setup_context.system_info->chip_id;
        module_config = pcie_setup_context.module_config;
        if (chip_id >= module_config->resource_info_count) {
            FWK_LOG_ERR(MOD_NAME "Module configuration not valid.");
            return FWK_E_PARAM;
        }

        memcpy(
            &pcie_setup_context.resource_info,
            &module_config->resource_info[chip_id],
            sizeof(pcie_setup_context.resource_info));

        return FWK_SUCCESS;
    }

    fwk_assert(fwk_module_is_valid_element_id(id));

    element = fwk_id_get_element_idx(id);

    res_info = &pcie_setup_context.resource_info;

    if (fwk_id_type_is_valid(res_info->plat_notification.source_id)) {
        /* Bind to the platform notification that is needed to start module
         * configuration. */
        status = fwk_notification_subscribe(
            res_info->plat_notification.notification_id,
            res_info->plat_notification.source_id,
            id);
    } else {
        struct mod_pcie_setup_config *config;
        config = pcie_setup_context.config[element];
        status = configure_pcie_ecam_mmio_space(config, element);
    }

    return status;
}

static int mod_pcie_setup_process_notification(
    const struct fwk_event *event,
    struct fwk_event *resp_event)
{
    struct mod_pcie_setup_resource_info *res_info;
    struct mod_pcie_setup_config *config;

    fwk_assert(!fwk_id_is_type(event->target_id, FWK_ID_TYPE_MODULE));

    res_info = &pcie_setup_context.resource_info;
    if (fwk_id_is_equal(
            event->id, res_info->plat_notification.notification_id)) {
        unsigned int element;
        int status;

        status = fwk_notification_unsubscribe(
            event->id, event->source_id, event->target_id);
        if (status != FWK_SUCCESS) {
            return status;
        }

        element = fwk_id_get_element_idx(event->target_id);
        config = pcie_setup_context.config[element];
        status = configure_pcie_ecam_mmio_space(config, element);
        if (status != FWK_SUCCESS) {
            return status;
        }
    }

    return FWK_SUCCESS;
}

const struct fwk_module module_pcie_setup = {
    .type = FWK_MODULE_TYPE_DRIVER,
    .init = mod_pcie_setup_init,
    .element_init = mod_pcie_setup_element_init,
    .bind = mod_pcie_setup_bind,
    .start = mod_pcie_setup_start,
    .notification_count = (unsigned int)MOD_PCIE_SETUP_NOTIFICATION_IDX_COUNT,
    .process_notification = mod_pcie_setup_process_notification,
};
