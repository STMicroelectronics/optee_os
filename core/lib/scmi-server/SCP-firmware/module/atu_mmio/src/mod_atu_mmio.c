/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     ATU MMIO module.
 */

#include <mod_atu.h>
#include <mod_atu_mmio.h>

#include <interface_address_remapper.h>

#include <fwk_assert.h>
#include <fwk_id.h>
#include <fwk_log.h>
#include <fwk_mmio.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>
#include <fwk_status.h>

#define MOD_NAME "[ATU MMIO] "

/*!
 * \brief Width of the Read/Write operation.
 */
enum atu_mmio_read_write_width {
    /* 8 bit Read/Write operation. */
    ATU_MMIO_RW_WIDTH_BYTE = 1,
    /* 16 bit Read/Write operation. */
    ATU_MMIO_RW_WIDTH_HALFWORD,
    /* 32 bit Read/Write operation. */
    ATU_MMIO_RW_WIDTH_WORD,
    /* 64 bit Read/Write operation. */
    ATU_MMIO_RW_WIDTH_DOUBLEWORD,
};

/*! Module Context. */
struct atu_mmio_context {
    /*! Module config. */
    const struct mod_atu_mmio_config *config;
    /*! Address translation unit APIs. */
    struct mod_atu_api *atu_api;
    /*! Current mapped physical address in the ATU. */
    uint64_t current_mapped_phys_address;
    /*! Region index of the mapping in ATU. */
    uint8_t mapped_index;
    /*! Flag indicating the presence of an active ATU mapping */
    bool is_region_mapped;
};

static struct atu_mmio_context ctx;

/*! Return the width of the data type. */
static size_t get_width(enum atu_mmio_read_write_width width)
{
    size_t size;

    switch (width) {
    case ATU_MMIO_RW_WIDTH_BYTE:
        size = sizeof(uint8_t);
        break;

    case ATU_MMIO_RW_WIDTH_HALFWORD:
        size = sizeof(uint16_t);
        break;

    case ATU_MMIO_RW_WIDTH_WORD:
        size = sizeof(uint32_t);
        break;

    case ATU_MMIO_RW_WIDTH_DOUBLEWORD:
        size = sizeof(uint64_t);
        break;

    default:
        size = 0;
        fwk_unexpected();
        break;
    };

    return size;
}

/* Check if the address fall within previously mapped window. */
static bool is_addr_mapped(uint64_t address, size_t size)
{
    return ctx.is_region_mapped &&
        (address >= ctx.current_mapped_phys_address) &&
        ((address + size) <=
         (ctx.current_mapped_phys_address + ctx.config->map_size));
}

/* Maps the address if it does not fall within previously mapped region. */
static uintptr_t map_region(
    uint64_t request_phy_address,
    enum atu_mmio_read_write_width width)
{
    struct atu_region_map atu_map;
    uint64_t aligned_request_phy_address;
    int status;

    if (!is_addr_mapped(request_phy_address, get_width(width))) {
        /*
         * A single ATU translation region is used to provide MMIO operations.
         * In order to prevent stale value reads when the ATU region is used to
         * map a different target address, flush and invalidate the cache before
         * unmapping the region.
         */
#if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
        SCB_CleanInvalidateDCache_by_Addr(
            (uintptr_t)(ctx.config->window_address), ctx.config->map_size);
#endif

        if (ctx.is_region_mapped) {
            status = ctx.atu_api->remove_region(
                ctx.mapped_index,
                ctx.config->atu_id,
                FWK_ID_MODULE(FWK_MODULE_IDX_ATU_MMIO));
            if (status != FWK_SUCCESS) {
                fwk_unexpected();
            }
            ctx.is_region_mapped = false;
        }

        /*
         * Align the physical address if it is not aligned with the translation
         * window.
         */
        aligned_request_phy_address = request_phy_address -
            (request_phy_address & (ctx.config->map_size - 1));
        atu_map.region_owner_id = FWK_ID_MODULE(FWK_MODULE_IDX_ATU_MMIO);
        /* TODO: Support for allowing non root PAS. */
        atu_map.attributes = ATU_ENCODE_ATTRIBUTES_ROOT_PAS;
        atu_map.log_addr_base = ctx.config->window_address;
        atu_map.region_size = ctx.config->map_size;
        atu_map.phy_addr_base = aligned_request_phy_address;

        status = ctx.atu_api->add_region(
            &atu_map, ctx.config->atu_id, &ctx.mapped_index);
        if (status != FWK_SUCCESS) {
            fwk_unexpected();
        }
        ctx.is_region_mapped = true;
        ctx.current_mapped_phys_address = aligned_request_phy_address;
    }

    /* Return the adjusted mapped address. */
    return ctx.config->window_address +
        (request_phy_address - ctx.current_mapped_phys_address);
}

/* Generic function for read operation. */
static uint64_t atu_mmio_read_value(
    uint64_t address,
    enum atu_mmio_read_write_width width)
{
    uintptr_t mapped_logical_address;
    uint64_t value;

    mapped_logical_address = map_region(address, width);

    switch (width) {
    case ATU_MMIO_RW_WIDTH_BYTE:
        value = fwk_mmio_read_8(mapped_logical_address);
        break;

    case ATU_MMIO_RW_WIDTH_HALFWORD:
        value = fwk_mmio_read_16(mapped_logical_address);
        break;

    case ATU_MMIO_RW_WIDTH_WORD:
        value = fwk_mmio_read_32(mapped_logical_address);
        break;

    case ATU_MMIO_RW_WIDTH_DOUBLEWORD:
        value = fwk_mmio_read_64(mapped_logical_address);
        break;

    default:
        fwk_unexpected();
        value = 0;
        break;
    };

    return value;
}

/* Generic fucntion for write operation. */
static void atu_mmio_write_value(
    uint64_t address,
    uint64_t value,
    enum atu_mmio_read_write_width width)
{
    uintptr_t mapped_log_address;

    mapped_log_address = map_region(address, width);

    switch (width) {
    case ATU_MMIO_RW_WIDTH_BYTE:
        fwk_mmio_write_8(mapped_log_address, (uint8_t)value);
        break;

    case ATU_MMIO_RW_WIDTH_HALFWORD:
        fwk_mmio_write_16(mapped_log_address, (uint16_t)value);
        break;

    case ATU_MMIO_RW_WIDTH_WORD:
        fwk_mmio_write_32(mapped_log_address, (uint32_t)value);
        break;

    case ATU_MMIO_RW_WIDTH_DOUBLEWORD:
        fwk_mmio_write_64(mapped_log_address, value);
        break;

    default:
        fwk_unexpected();
        break;
    };
}

static uint8_t atu_mmio_read8(uint64_t address)
{
    return (uint8_t)atu_mmio_read_value(address, ATU_MMIO_RW_WIDTH_BYTE);
}

static uint16_t atu_mmio_read16(uint64_t address)
{
    return (uint16_t)atu_mmio_read_value(address, ATU_MMIO_RW_WIDTH_HALFWORD);
}

static uint32_t atu_mmio_read32(uint64_t address)
{
    return (uint32_t)atu_mmio_read_value(address, ATU_MMIO_RW_WIDTH_WORD);
}

static uint64_t atu_mmio_read64(uint64_t address)
{
    return atu_mmio_read_value(address, ATU_MMIO_RW_WIDTH_DOUBLEWORD);
}

static void atu_mmio_write8(uint64_t address, uint8_t value)
{
    atu_mmio_write_value(address, value, ATU_MMIO_RW_WIDTH_BYTE);
}

static void atu_mmio_write16(uint64_t address, uint16_t value)
{
    atu_mmio_write_value(address, value, ATU_MMIO_RW_WIDTH_HALFWORD);
}

static void atu_mmio_write32(uint64_t address, uint32_t value)
{
    atu_mmio_write_value(address, value, ATU_MMIO_RW_WIDTH_WORD);
}

static void atu_mmio_write64(uint64_t address, uint64_t value)
{
    atu_mmio_write_value(address, value, ATU_MMIO_RW_WIDTH_DOUBLEWORD);
}

struct interface_address_remapper_rw_api rw_api = {
    .read8 = atu_mmio_read8,
    .read16 = atu_mmio_read16,
    .read32 = atu_mmio_read32,
    .read64 = atu_mmio_read64,
    .write8 = atu_mmio_write8,
    .write16 = atu_mmio_write16,
    .write32 = atu_mmio_write32,
    .write64 = atu_mmio_write64,
};

/*
 * Framework handlers.
 */
static int atu_mmio_init(
    fwk_id_t module_id,
    unsigned int element_count,
    const void *config)
{
    ctx.config = config;
    ctx.is_region_mapped = false;

    return FWK_SUCCESS;
}

static int atu_mmio_bind(fwk_id_t id, unsigned int round)
{
    return fwk_module_bind(
        ctx.config->atu_id, ctx.config->atu_api_id, &ctx.atu_api);
}

static int atu_mmio_process_bind_request(
    fwk_id_t requester_id,
    fwk_id_t id,
    fwk_id_t api_id,
    const void **api)
{
    enum mod_atu_mmio_api_idx api_idx;
    int status;

    if (!fwk_module_is_valid_module_id(id)) {
        return FWK_E_PARAM;
    }

    api_idx = (enum mod_atu_mmio_api_idx)fwk_id_get_api_idx(api_id);
    switch (api_idx) {
    case MOD_ATU_MMIO_API_IDX_MEM_RW:
        *api = &rw_api;
        status = FWK_SUCCESS;
        break;
    default:
        status = FWK_E_PARAM;
        break;
    };

    return status;
}

const struct fwk_module module_atu_mmio = {
    .type = FWK_MODULE_TYPE_DRIVER,
    .init = atu_mmio_init,
    .api_count = MOD_ATU_MMIO_API_IDX_MAX,
    .bind = atu_mmio_bind,
    .process_bind_request = atu_mmio_process_bind_request,
};
