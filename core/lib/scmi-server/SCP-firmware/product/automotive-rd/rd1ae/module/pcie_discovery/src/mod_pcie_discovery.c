/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <mod_pcie_discovery.h>

#include <fwk_assert.h>
#include <fwk_attributes.h>
#include <fwk_log.h>
#include <fwk_macros.h>
#include <fwk_module.h>

#include <fmw_cmsis.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* PCIe configuration space offset definitions */
#define PCIE_BAR_OFFSET 0x10

#define PCIE_BAR_SIZE 0x4

/*
 * ECAM space per bus
 * 32 devices * 8 functions * 4kB config space = 1MB
 */
#define MAX_ECAM_SPACE_PER_BUS (FWK_MIB)

/*
 * MAX ECAM space
 * MAX ECAM space per bus * Max numbers of bus(256)
 */
#define MAX_ECAM_SPACE (256 * MAX_ECAM_SPACE_PER_BUS)

/* PCIe configuration space offset definitions */
#define PCIE_HEADER_TYPE_OFFSET         0xE
#define PCIE_PRIMARY_BUS_NUM_OFFSET     0x18
#define PCIE_SECONDARY_BUS_NUM_OFFSET   0x19
#define PCIE_SUBORDINATE_BUS_NUM_OFFSET 0x1A

#define PCIE_HEADER_TYPE_MASK     0x7F
#define PCIE_HEADER_TYPE_ENDPOINT 0
#define PCIE_HEADER_TYPE_BRIDGE   1

/* BDF table offsets for PCIe & CCIX controllers */
#define PCIE_BDF_TABLE_OFFSET 0

/* PCIe standard definitions */
#define PCIE_BUS_NUM_MAX         0xFF
#define DEVICES_PER_BUS_MAX      32
#define FUNCTIONS_PER_DEVICE_MAX 8
#define BRIDGE_BAR_COUNT         2
#define DEVICE_BAR_COUNT         6

#define BDF_ADDR_SHIFT_BUS      20
#define BDF_ADDR_SHIFT_DEVICE   15
#define BDF_ADDR_SHIFT_FUNCTION 12

#define PCIE_MEM_TYPE_MASK 0x06
#define PCIE_MEM_TYPE_32   0x00 /* 32 bit address */
#define PCIE_MEM_TYPE_64   0x04 /* 64 bit address */
#define PCIE_MEM_MASK      ~(0xFULL)

#define PCIE_GET_MMIO_SIZE64(BAR) (~(BAR & PCIE_MEM_MASK) + 1)
#define PCIE_GET_MMIO_SIZE32(BAR) (~((uint32_t)(BAR & PCIE_MEM_MASK)) + 1)

#define PCIE_BRIDGE_MEM_WINDOW_ALIGNMENT (1UL << 20)

/* Default API to do 1 byte read */
uint8_t read8(uint64_t addr)
{
    return *((volatile uint8_t *)(uint32_t)addr);
}

/* Default API to do 1 byte write */
void write8(uint64_t addr, uint8_t value)
{
    *((volatile uint8_t *)(uint32_t)addr) = value;
}

/* Default API to do 4 byte read */
uint32_t read32(uint64_t addr)
{
    return *((volatile uint32_t *)(uint32_t)addr);
}

/* Default API to do 4 byte write */
void write32(uint64_t addr, uint32_t value)
{
    *((volatile uint32_t *)(uint32_t)addr) = value;
}

struct pcie_discovery_rw_api default_rw_api = {
    .read8 = read8,
    .write8 = write8,
    .read32 = read32,
    .write32 = write32,
};

/* The bridge expects the mmio window address to be 1M align */
static void align_address_for_bridge_window(struct pcie_mmap_size *mmap_size)
{
    mmap_size->mmiol =
        FWK_ALIGN_NEXT(mmap_size->mmiol, PCIE_BRIDGE_MEM_WINDOW_ALIGNMENT);
    mmap_size->mmioh =
        FWK_ALIGN_NEXT(mmap_size->mmioh, PCIE_BRIDGE_MEM_WINDOW_ALIGNMENT);
}

/* Program sequence to get the configuration size of a PCIe device */
static void get_mmio_memory_size(
    uint64_t ecam_addr,
    struct pcie_mmap_size *mmap_size,
    struct pcie_discovery_rw_api *rw_api,
    uint8_t num_bar)
{
    uint8_t idx;
    uint64_t pcie_bar_addr;
    uint64_t mmio_size;

    /*
     * Write 0xFFFFFFFF to all the BARs sequentially to get the configuration
     * size.
     */
    pcie_bar_addr = ecam_addr + PCIE_BAR_OFFSET;
    for (idx = 0; idx < num_bar; idx++) {
        rw_api->write32(pcie_bar_addr, 0xFFFFFFFF);
        pcie_bar_addr += PCIE_BAR_SIZE;
    }

    /*
     * This section treats IO and Memory address space as Memory address space.
     */
    pcie_bar_addr = ecam_addr + PCIE_BAR_OFFSET;
    for (idx = 0; idx < num_bar; idx++) {
        mmio_size = rw_api->read32(pcie_bar_addr);
        if ((mmio_size != 0) && (mmio_size != 0xFFFFFFFF)) {
            if ((mmio_size & PCIE_MEM_TYPE_MASK) == PCIE_MEM_TYPE_64) {
                /*
                 * Device supports 64 bit addressing. Increase the counter and
                 * address and read the next 32 bits of 64 bit mmap size.
                 */
                idx++;
                pcie_bar_addr += PCIE_BAR_SIZE;
                mmio_size |= ((uint64_t)(rw_api->read32(pcie_bar_addr))) << 32;
                mmio_size = PCIE_GET_MMIO_SIZE64(mmio_size);
                mmap_size->mmioh = FWK_ALIGN_NEXT(mmap_size->mmioh, mmio_size);
                mmap_size->mmioh += mmio_size;
            } else {
                /* Device supports 32 bit addressing. */
                mmio_size = PCIE_GET_MMIO_SIZE32(mmio_size);
                mmap_size->mmiol = FWK_ALIGN_NEXT(mmap_size->mmiol, mmio_size);
                mmap_size->mmiol += mmio_size;
            }
        }
        pcie_bar_addr += PCIE_BAR_SIZE;
    }
}

/*
 * Procedure to do PCIe bus walk. This will also calculate the total mmio region
 * size required for each root port.
 */
static uint8_t pcie_bus_scan(
    uint64_t ecam_addr,
    uint8_t pri_bnum,
    uint8_t sec_bnum,
    struct pcie_mmap_size *mmap_size,
    struct pcie_discovery_rw_api *rw_api)
{
    int dev_num;
    int fn_num;
    uint8_t bar_count;
    uint8_t header_type;
    uint8_t sub_bnum;
    uint32_t df_addr;
    uint32_t vid;
    uint64_t config_addr;

    sub_bnum = pri_bnum;
    /* Loop over all devices on pri_bnum bus */
    for (dev_num = 0; dev_num < DEVICES_PER_BUS_MAX; dev_num++) {
        /* Loop over all functions on dev_num device */
        for (fn_num = 0; fn_num < FUNCTIONS_PER_DEVICE_MAX; fn_num++) {
            df_addr = (dev_num << BDF_ADDR_SHIFT_DEVICE) |
                (fn_num << BDF_ADDR_SHIFT_FUNCTION);
            config_addr = ecam_addr + df_addr;

            vid = rw_api->read32(config_addr);
            /* If function 0 of any device has invalid VID break the loop */
            if ((vid & 0xFFFF) == 0xFFFF) {
                if (fn_num == 0) {
                    break;
                } else {
                    continue;
                }
            }

            FWK_LOG_LOCAL(
                "[PCIE]: Found device: %d:%d:%d\n", pri_bnum, dev_num, fn_num);

            /*
             * Read the header type to identify if the device
             * is an endpoint or a PCI-PCI bridge.
             */
            header_type = rw_api->read8(config_addr + PCIE_HEADER_TYPE_OFFSET);
            if ((header_type & PCIE_HEADER_TYPE_MASK) ==
                PCIE_HEADER_TYPE_BRIDGE) {
                /*
                 * PCI-PCI bridge is identified. Set primary and secondary bus
                 * numbers. Let subordinate bus number be max possible bus
                 * number as we need to further identify devices downstream.
                 */
                rw_api->write8(
                    config_addr + PCIE_PRIMARY_BUS_NUM_OFFSET, pri_bnum);
                rw_api->write8(
                    config_addr + PCIE_SECONDARY_BUS_NUM_OFFSET, sec_bnum);
                rw_api->write8(
                    config_addr + PCIE_SUBORDINATE_BUS_NUM_OFFSET,
                    PCIE_BUS_NUM_MAX);
                /*
                 * Recursively call the scan function with incremented
                 * primary and secondary bus numbers.
                 */
                sub_bnum = pcie_bus_scan(
                    ecam_addr + ((sec_bnum - pri_bnum) << BDF_ADDR_SHIFT_BUS),
                    sec_bnum,
                    sec_bnum + 1,
                    mmap_size,
                    rw_api);
                /*
                 * The recursive call has returned from an endpoint
                 * identification so use the returned bus number as the
                 * bridge's subordinate bus number.
                 */
                rw_api->write8(
                    config_addr + PCIE_SUBORDINATE_BUS_NUM_OFFSET, sub_bnum);
                sec_bnum = sub_bnum + 1;
                bar_count = BRIDGE_BAR_COUNT;
                /* Memory carveout under bridge should be 1 MB aligned. */
                align_address_for_bridge_window(mmap_size);
            } else {
                /*
                 * Endpoint is identified. Proceed to other functions &
                 * devices in this bus and return to previous recursive call.
                 */
                sub_bnum = sec_bnum - 1;
                bar_count = DEVICE_BAR_COUNT;
            }
            get_mmio_memory_size(config_addr, mmap_size, rw_api, bar_count);
        }
    }
    /* Return the subordinate bus number to previous recursive call */
    return sub_bnum;
}

static int calculate_resource(
    uint64_t ecam_base_address,
    struct pcie_mmap_size *mmap_size,
    uint8_t primary_bus_number,
    struct pcie_discovery_rw_api *rw_api)
{
    uint8_t pri_bnum;
    uint8_t sec_bnum;
    uint8_t sub_bnum;

    if ((ecam_base_address == 0) || (mmap_size == NULL) ||
        (primary_bus_number == UINT8_MAX)) {
        return FWK_E_PARAM;
    }

    if (rw_api == NULL) {
        if (ecam_base_address > UINT32_MAX) {
            return FWK_E_PARAM;
        }

        rw_api = &default_rw_api;
    }

    pri_bnum = primary_bus_number;
    sec_bnum = primary_bus_number + 1;

    sub_bnum =
        pcie_bus_scan(ecam_base_address, pri_bnum, sec_bnum, mmap_size, rw_api);

    mmap_size->bus = (sub_bnum - pri_bnum) + 1;
    mmap_size->ecam = mmap_size->bus * MAX_ECAM_SPACE_PER_BUS;

    /*
     * Linux/EDK2 expects the mmiol size to be minumum size of
     * (Number of bus * 1M)
     */
    mmap_size->mmiol = FWK_ALIGN_NEXT(
        mmap_size->mmiol, mmap_size->bus * MAX_ECAM_SPACE_PER_BUS);

    /* Align for host bridge */
    align_address_for_bridge_window(mmap_size);

    return FWK_SUCCESS;
}

static int pcie_discovery_init(
    fwk_id_t module_id,
    unsigned int unused,
    const void *unused2)
{
    return FWK_SUCCESS;
}

static struct mod_pcie_discovery_api pcie_discovery_api = {
    .calculate_resource = calculate_resource,
};

static int pcie_discovery_process_bind_request(
    fwk_id_t requester_id,
    fwk_id_t id,
    fwk_id_t api_id,
    const void **api)
{
    enum mod_pcie_discovery_api_idx api_idx;

    api_idx = (enum mod_pcie_discovery_api_idx)fwk_id_get_api_idx(api_id);
    if (api_idx != MOD_PCIE_DISCOVERY_API_IDX_CALCULATE_RESOURCES) {
        return FWK_E_PARAM;
    }

    *api = &pcie_discovery_api;
    return FWK_SUCCESS;
}

const struct fwk_module module_pcie_discovery = {
    .type = FWK_MODULE_TYPE_SERVICE,
    .api_count = MOD_PCIE_DISCOVERY_API_IDX_MAP,
    .init = pcie_discovery_init,
    .process_bind_request = pcie_discovery_process_bind_request,
};

const struct fwk_module_config config_pcie_discovery = { 0 };
