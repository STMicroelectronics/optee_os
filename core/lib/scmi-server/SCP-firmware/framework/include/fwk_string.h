/*
 * Arm SCP/MCP Software
 * Copyright (c) 2021-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef FWK_STRING_H
#define FWK_STRING_H

#include <fwk_attributes.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/*!
 * \addtogroup GroupLibFramework Framework
 * \{
 */

/*!
 * \addtogroup GroupString Strings Management
 *
 * \brief Manipulation of strings and arrays.
 *
 * \details This interface extends the standard C library for strings and arrays
 *      manipulations to improve portability and provide a consistent set of
 *      checks.
 *
 * \{
 */

/*!
 * \brief Copies the value `ch` into each of the first `count` characters of the
 *      object pointed by dest.
 *
 * \details This is a wrapper of the memset() provided by the standard
 *      library. It behaves as the standard-library function but also checks on
 *      whether the returned `dest` is equal to the original dest. If not, the
 *      function traps.
 *
 * \details This should be used in place of memset() when the return value can
 *      be discarded.
 *
 * \param[in] dest Pointer to the object to fill
 * \param[in] ch Value to be copied
 * \param[in] count number of bytes to copy
 */
FWK_LEAF FWK_NOTHROW void fwk_str_memset(void *dest, int ch, size_t count);

/*!
 * \brief Copies `count` characters from the object pointed to by `src` to the
 *      object pointed to by `dest`.
 *
 * \details This is a wrapper of the memcpy() provided by the standard
 *      library. It behaves as the standard-library function but also checks on
 *      whether the returned `dest` is equal to the original dest. If not, the
 *      function traps.
 *
 * \details This should be used in place of memcpy() when the return value can
 *      be discarded.
 *
 * \param[in] dest Pointer to the object to copy to
 * \param[in] src Pointer to the object to copy from
 * \param[in] count number of bytes to copy
 */
FWK_LEAF FWK_NOTHROW void fwk_str_memcpy(
    void *dest,
    const void *src,
    size_t count);

/*!
 * \brief Copies at most `count` characters from the object pointed to by `src`
 *      (including the terminating null character) to the object pointed to by
 *      `dest`.
 *
 * \details This is a wrapper of the strncpy() provided by the standard
 *      library. It behaves as the standard-library function but also checks on
 *      whether the returned `dest` is equal to the original dest. If not, the
 *      function traps.
 *
 * \details This should be used in place of strncpy() when the return value can
 *      be discarded.
 *
 * \param[in] dest Pointer to the object to copy to
 * \param[in] src Pointer to the object to copy from
 * \param[in] count number of bytes to copy
 */
FWK_LEAF FWK_NOTHROW void fwk_str_strncpy(
    char *dest,
    const char *src,
    size_t count);

/*!
 * \brief Checks that a copy will be within the bounds of the destination buffer
 *
 * \details This is a simple maths function, returns true if the copy will be in
 *      bounds otherwise returns false.
 *
 * \param[in] buffer_size Size of a buffer to copy to
 * \param[in] offset Offset of copy within destination buffer
 * \param[in] data_size number of bytes to copy
 * \return    True if the copy is within the bounds of the destination buffer,
 *            false otherwise
 */
FWK_LEAF FWK_NOTHROW bool fwk_str_is_in_boundry(
    size_t buffer_size,
    size_t offset,
    size_t data_size);

/*!
 * \}
 */

/*!
 * \}
 */

#endif /* FWK_STRING_H */
