/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOD_MOCK_TELEMETRY_SOURCE_H
#define MOD_MOCK_TELEMETRY_SOURCE_H

#include <mod_telemetry.h>

#include <stddef.h>
#include <stdint.h>

/*!
 * \brief Mock telemetry source configuration.
 *
 * The mock telemetry source exposes a configurable list of Data Events (DE).
 * Each DE provides its descriptor, optional fast channel attributes, name,
 * and an optional initial mocked value.
 */

/*!
 * \brief Signature of a mock telemetry value mutator.
 *
 * \param[in,out] value Pointer to the value to be mutated in place.
 */
typedef void (*mod_mock_telemetry_source_mutator)(uint64_t *value);

/*!
 * \brief Module configuration for a mock telemetry source instance.
 *
 * List of Data Events (DEs) exposed by the mock driver as well
 * as optional metadata such as FastChannel attributes, initial values, and
 * value mutators that evolve the mocked samples over time.
 */
struct mod_mock_telemetry_source_config {
    /*! Number of Data Events served by the mock telemetry source. */
    size_t de_count;

    /*! Pointer to the DE descriptor table (must contain \c de_count entries).
     */
    const struct mod_telemetry_de_desc *de_desc_table;

    /*!
     * Pointer to the DE fast channel attribute table.
     * Can be \c NULL if no fast channels are required.
     */
    const struct mod_telemetry_de_fch_attr *de_fch_attr_table;

    /*!
     * Pointer to the DE name table (must contain \c de_count entries).
     * Each entry must point to a NUL-terminated string.
     */
    const char *const *de_names;

    /*!
     * Pointer to the optional initial mocked values table.
     * When provided, it must contain \c de_count entries.
     * When \c NULL, mocked values default to zero.
     */
    const uint64_t *initial_values;

    /*!
     * Pointer to the optional mutator table (must contain \c de_count entries).
     * Each mutator is invoked on every generated value. When \c NULL, no
     * automatic mutation is applied to the DE values.
     */
    const mod_mock_telemetry_source_mutator *value_mutators;
};

#endif /* MOD_MOCK_TELEMETRY_SOURCE_H */
