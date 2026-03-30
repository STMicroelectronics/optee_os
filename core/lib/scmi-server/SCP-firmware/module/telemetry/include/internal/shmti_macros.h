/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Telemetry SHMTI macros.
 */

#ifndef SHMTI_MACROS_H
#define SHMTI_MACROS_H

#include "tdcf_defs.h"

/* Helper macros for splitting 64-bit values */
#define HI32(x) ((uint32_t)((((uint64_t)x) >> 32) & 0xFFFFFFFF))
#define LO32(x) ((uint32_t)(((uint64_t)x) & 0xFFFFFFFF))

/*! Bit position of Block timestamp line flag in 32 bit value */
#define MOD_TELEMETRY_METADATA_BLOCK_TIMESTAMP_LINE_POS (4U)
/*! Bit position of Block timestamp flag in 32 bit value */
#define MOD_TELEMETRY_METADATA_BLOCK_TIMESTAMP_POS (3U)
/*! Bit position of Line timestamp flag in 32 bit value */
#define MOD_TELEMETRY_METADATA_LINE_TIMESTAMP_POS (2U)
/*! Bit position of timestamp valid flag in 32 bit value */
#define MOD_TELEMETRY_METADATA_TIMESTAMP_VALID_POS (1U)
/*! Bit position of Data invalid flag in 32 bit value */
#define MOD_TELEMETRY_METADATA_DATA_INVALID_POS (0U)

#define METADATA_BLOCK_TIMESTAMP_LINE \
    (1U << MOD_TELEMETRY_METADATA_BLOCK_TIMESTAMP_LINE_POS)
#define METADATA_DATA_LINE_USES_BLOCK_TIMESTAMP \
    ((1U << MOD_TELEMETRY_METADATA_BLOCK_TIMESTAMP_POS) | \
     (1U << MOD_TELEMETRY_METADATA_TIMESTAMP_VALID_POS))
#define METADATA_DATA_LINE_USES_LINE_TIMESTAMP \
    ((1U << MOD_TELEMETRY_METADATA_LINE_TIMESTAMP_POS) | \
     (1U << MOD_TELEMETRY_METADATA_TIMESTAMP_VALID_POS))
#define METADATA_DATA_LINE_INVALID \
    (1U << MOD_TELEMETRY_METADATA_DATA_INVALID_POS)
#define METADATA_BUILD( \
    block_ts_line, uses_block_ts, uses_line_ts, data_invalid) \
    (((block_ts_line) ? (METADATA_BLOCK_TIMESTAMP_LINE) : 0) | \
     ((uses_block_ts) ? (METADATA_DATA_LINE_USES_BLOCK_TIMESTAMP) : 0) | \
     ((uses_line_ts) ? (METADATA_DATA_LINE_USES_LINE_TIMESTAMP) : 0) | \
     ((data_invalid) ? (METADATA_DATA_LINE_INVALID) : 0))

#define WRITE_BLOCK_TIMESTAMP_LINE(ADDR, TIMESTAMP) \
    do { \
        struct block_timestamp_line *line = \
            (struct block_timestamp_line *)ADDR; \
        line->metadata = METADATA_BUILD(true, false, false, true); \
        line->id = 0; \
        line->block_timestamp_low = LO32(TS); \
        line->block_timestamp_high = HI32(TS); \
    } while (0)

#define WRITE_DATA_LINE_BLOCK_TIMESTAMP(ADDR, DATA, ID) \
    do { \
        struct de_non_ts_line *line = (struct de_non_ts_line *)ADDR; \
        line->metadata = METADATA_BUILD(false, true, false, true); \
        line->id = (ID); \
        line->data_low = LO32(DATA); \
        line->data_high = HI32(DATA); \
    } while (0)

#define WRITE_DATA_LINE_NON_TIMESTAMP(ADDR, DATA, ID) \
    do { \
        struct de_non_ts_line *line = (struct de_non_ts_line *)ADDR; \
        line->metadata = METADATA_BUILD(false, false, false, false); \
        line->id = (ID); \
        line->data_low = LO32(DATA); \
        line->data_high = HI32(DATA); \
    } while (0)

#define WRITE_DATA_LINE_LINE_TIMESTAMP(ADDR, DATA, ID, TIMESTAMP) \
    do { \
        struct de_ts_line *line = (struct de_ts_line *)ADDR; \
        line->metadata = METADATA_BUILD(false, false, true, false); \
        line->id = (ID); \
        line->data_low = LO32(DATA); \
        line->data_high = HI32(DATA); \
        line->timestamp_low = LO32(TIMESTAMP); \
        line->timestamp_high = HI32(TIMESTAMP); \
    } while (0)

#endif /* !SHMTI_MACROS_H */
