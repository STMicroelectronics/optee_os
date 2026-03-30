/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef FWK_MMIO_H
#define FWK_MMIO_H

#include <stdint.h>

/*!
 * \addtogroup GroupLibFramework Framework
 * \{
 */

/*!
 * \defgroup GroupMMIO Memory-Mapped I/O
 * \{
 */

/*!
 * \brief Writes an 8-bit MMIO register.
 *
 * \param addr The MMIO register to write.
 * \param value The value to write to the MMIO register.
 *
 */
static inline void fwk_mmio_write_8(uintptr_t addr, uint8_t value)
{
    *(volatile uint8_t *)addr = value;
}

/*!
 * \brief Reads an 8-bit MMIO register.
 *
 * \param addr The MMIO register to read.
 *
 * \return The value read.
 *
 */
static inline uint8_t fwk_mmio_read_8(uintptr_t addr)
{
    return *(volatile uint8_t *)addr;
}

/*!
 * \brief Reads a 8-bit MMIO register, performs a bitwise AND, and writes the
 *      result back to the 8-bit MMIO register.
 *
 * \param addr The MMIO register to write.
 * \param clear Value of the bitmask to clear.
 *
 */
static inline void fwk_mmio_clrbits_8(uintptr_t addr, uint8_t clear)
{
    fwk_mmio_write_8(addr, fwk_mmio_read_8(addr) & ~clear);
}

/*!
 * \brief Reads a 8-bit MMIO register, performs a bitwise OR, and writes the
 *      result back to the 8-bit MMIO register.
 *
 * \param addr The MMIO register to write.
 * \param set Value of the bitmask to set.
 *
 */
static inline void fwk_mmio_setbits_8(uintptr_t addr, uint8_t set)
{
    fwk_mmio_write_8(addr, fwk_mmio_read_8(addr) | set);
}

/*!
 * \brief Reads a 8-bit MMIO register, performs a bitwise AND followed by a
 *      bitwise OR, and writes the result back to the 8-bit MMIO register.
 *
 * \param addr The MMIO register to write.
 * \param clear Value of the bitmask to clear.
 * \param set Value of the bitmask to set.
 *
 */
static inline void fwk_mmio_clrsetbits_8(
    uintptr_t addr,
    uint8_t clear,
    uint8_t set)
{
    fwk_mmio_write_8(addr, (fwk_mmio_read_8(addr) & ~clear) | set);
}

/*!
 * \brief Writes an 16-bit MMIO register.
 *
 * \param addr The MMIO register to write.
 * \param value The value to write to the MMIO register.
 *
 */
static inline void fwk_mmio_write_16(uintptr_t addr, uint16_t value)
{
    *(volatile uint16_t *)addr = value;
}

/*!
 * \brief Reads an 16-bit MMIO register.
 *
 * \param addr The MMIO register to read.
 *
 * \return The value read.
 *
 */
static inline uint16_t fwk_mmio_read_16(uintptr_t addr)
{
    return *(volatile uint16_t *)addr;
}

/*!
 * \brief Reads a 16-bit MMIO register, performs a bitwise AND, and writes the
 *      result back to the 16-bit MMIO register.
 *
 * \param addr The MMIO register to write.
 * \param clear Value of the bitmask to clear.
 *
 */
static inline void fwk_mmio_clrbits_16(uintptr_t addr, uint16_t clear)
{
    fwk_mmio_write_16(addr, fwk_mmio_read_16(addr) & ~clear);
}

/*!
 * \brief Reads a 16-bit MMIO register, performs a bitwise OR, and writes the
 *      result back to the 16-bit MMIO register.
 *
 * \param addr The MMIO register to write.
 * \param set Value of the bitmask to set.
 *
 */
static inline void fwk_mmio_setbits_16(uintptr_t addr, uint16_t set)
{
    fwk_mmio_write_16(addr, fwk_mmio_read_16(addr) | set);
}

/*!
 * \brief Reads a 16-bit MMIO register, performs a bitwise AND followed by a
 *      bitwise OR, and writes the result back to the 16-bit MMIO register.
 *
 * \param addr The MMIO register to write.
 * \param clear Value of the bitmask to clear.
 * \param set Value of the bitmask to set.
 *
 */
static inline void fwk_mmio_clrsetbits_16(
    uintptr_t addr,
    uint16_t clear,
    uint16_t set)
{
    fwk_mmio_write_16(addr, (fwk_mmio_read_16(addr) & ~clear) | set);
}

/*!
 * \brief Writes an 32-bit MMIO register.
 *
 * \param addr The MMIO register to write.
 * \param value The value to write to the MMIO register.
 *
 */
static inline void fwk_mmio_write_32(uintptr_t addr, uint32_t value)
{
    *(volatile uint32_t *)addr = value;
}

/*!
 * \brief Reads an 32-bit MMIO register.
 *
 * \param addr The MMIO register to read.
 *
 * \return The value read.
 *
 */
static inline uint32_t fwk_mmio_read_32(uintptr_t addr)
{
    return *(volatile uint32_t *)addr;
}

/*!
 * \brief Reads a 32-bit MMIO register, performs a bitwise AND, and writes the
 *      result back to the 32-bit MMIO register.
 *
 * \param addr The MMIO register to write.
 * \param clear Value of the bitmask to clear.
 *
 */
static inline void fwk_mmio_clrbits_32(uintptr_t addr, uint32_t clear)
{
    fwk_mmio_write_32(addr, fwk_mmio_read_32(addr) & ~clear);
}

/*!
 * \brief Reads a 32-bit MMIO register, performs a bitwise OR, and writes the
 *      result back to the 32-bit MMIO register.
 *
 * \param addr The MMIO register to write.
 * \param set Value of the bitmask to set.
 *
 */
static inline void fwk_mmio_setbits_32(uintptr_t addr, uint32_t set)
{
    fwk_mmio_write_32(addr, fwk_mmio_read_32(addr) | set);
}

/*!
 * \brief Reads a 32-bit MMIO register, performs a bitwise AND followed by a
 *      bitwise OR, and writes the result back to the 32-bit MMIO register.
 *
 * \param addr The MMIO register to write.
 * \param clear Value of the bitmask to clear.
 * \param set Value of the bitmask to set.
 *
 */
static inline void fwk_mmio_clrsetbits_32(
    uintptr_t addr,
    uint32_t clear,
    uint32_t set)
{
    fwk_mmio_write_32(addr, (fwk_mmio_read_32(addr) & ~clear) | set);
}

/*!
 * \brief Writes an 64-bit MMIO register.
 *
 * \param addr The MMIO register to write.
 * \param value The value to write to the MMIO register.
 *
 */
static inline void fwk_mmio_write_64(uintptr_t addr, uint64_t value)
{
    *(volatile uint64_t *)addr = value;
}

/*!
 * \brief Reads an 64-bit MMIO register.
 *
 * \param addr The MMIO register to read.
 *
 * \return The value read.
 *
 */
static inline uint64_t fwk_mmio_read_64(uintptr_t addr)
{
    return *(volatile uint64_t *)addr;
}

/*!
 * \brief Reads a 64-bit MMIO register, performs a bitwise AND, and writes the
 *      result back to the 64-bit MMIO register.
 *
 * \param addr The MMIO register to write.
 * \param clear Value of the bitmask to clear.
 *
 */
static inline void fwk_mmio_clrbits_64(uintptr_t addr, uint64_t clear)
{
    fwk_mmio_write_64(addr, fwk_mmio_read_64(addr) & ~clear);
}

/*!
 * \brief Reads a 64-bit MMIO register, performs a bitwise OR, and writes the
 *      result back to the 64-bit MMIO register.
 *
 * \param addr The MMIO register to write.
 * \param set Value of the bitmask to set.
 *
 */
static inline void fwk_mmio_setbits_64(uintptr_t addr, uint64_t set)
{
    fwk_mmio_write_64(addr, fwk_mmio_read_64(addr) | set);
}

/*!
 * \brief Reads a 64-bit MMIO register, performs a bitwise AND followed by a
 *      bitwise OR, and writes the result back to the 64-bit MMIO register.
 *
 * \param addr The MMIO register to write.
 * \param clear Value of the bitmask to clear.
 * \param set Value of the bitmask to set.
 *
 */
static inline void fwk_mmio_clrsetbits_64(
    uintptr_t addr,
    uint64_t clear,
    uint64_t set)
{
    fwk_mmio_write_64(addr, (fwk_mmio_read_64(addr) & ~clear) | set);
}

/*!
 * \}
 */

/*!
 * \}
 */

#endif /* FWK_MMIO_H */
