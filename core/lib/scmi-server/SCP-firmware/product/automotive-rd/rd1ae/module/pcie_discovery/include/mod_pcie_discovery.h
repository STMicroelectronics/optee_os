/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOD_PCIE_DISCOVERY_H
#define MOD_PCIE_DISCOVERY_H

#include <fwk_macros.h>

#include <stdbool.h>
#include <stdint.h>

/*!
 * \brief Module API indices
 */
enum mod_pcie_discovery_api_idx {
    /*!
     * Interface to calculate ECAM, MMIOL and MMIOH resource size by doing PCIe
     * Bus Walk.
     */
    MOD_PCIE_DISCOVERY_API_IDX_CALCULATE_RESOURCES,
    /*! Total API Count */
    MOD_PCIE_DISCOVERY_API_IDX_MAP
};

/*!
 * \brief APIs to perform memory read and write operations.
 */
struct pcie_discovery_rw_api {
    /*!
     * \brief Read a byte from the given memory address
     *
     * \param addr Memory address
     *
     * \return byte value read from the memory address
     */
    uint8_t (*read8)(uint64_t addr);

    /*!
     * \brief Write a byte to the given memory address
     *
     * \param addr Memory address.
     * \param value Byte value to be written.
     */
    void (*write8)(uint64_t addr, uint8_t value);

    /*!
     * \brief Read a word from the given memory address
     *
     * \param addr Memory address
     *
     * \return word value read from the memory address
     */
    uint32_t (*read32)(uint64_t addr);

    /*!
     * \brief Write a word to the given memory address
     *
     * \param addr Memory address.
     * \param value Word value to be written.
     */
    void (*write32)(uint64_t addr, uint32_t value);
};

/*!
 * Datastructure to capture the size of memory required by all of the devices
 * present under the given host bridge.
 */
struct pcie_mmap_size {
    /*! ECAM region size */
    uint64_t ecam;

    /*! 32-bit MMIO space start size */
    uint64_t mmiol;

    /*! 64-bit MMIO space size */
    uint64_t mmioh;

    /*! Range of bus numbers consumed by the Root port */
    uint8_t bus;
};

struct mod_pcie_discovery_api {
    /*!
     * \brief Captures the size of memory required by the devices present in the
     *        tree.
     *        This does a depth first search, identify the device and then
     *        capture the memory required by the device in ECAM, MMIOL and MMIOH
     *        regions.
     *
     * \param ecam_base_address Base address of RP's ECAM region
     * \param mmap_size Placeholder for filling IO memory carveout sizes
     *                  required by all the endpoints under the given RP.
     * \param primary_bus_number Starting bus number.
     * \param rw_api Customised APIs use to do MMIO transactions in AP memory
     *               region. NOTE: The default read/write
     *               APIs are used this is NULL.
     *
     * \return FWK_E_PARAM If the ecam address and mmap_size are not valid.
     * \return FWK_SUCCESS On successful completion.
     */
    int (*calculate_resource)(
        uint64_t ecam_base_address,
        struct pcie_mmap_size *mmap_size,
        uint8_t primary_bus_number,
        struct pcie_discovery_rw_api *rw_api);
};

#endif /* MOD_PCIE_DISCOVERY_H */
