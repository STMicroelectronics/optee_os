/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Telemetry Data Capture Format(TDCF).
 */

#ifndef TDCF_DEFS_H
#define TDCF_DEFS_H

#include <fwk_attributes.h>

#include <stddef.h>
#include <stdint.h>

/*!
 * \brief DE data line with Timestamp.
 */
struct FWK_PACKED de_ts_line {
    /*! LineMetadata (Bit2=1, Bit1=1, rest 0) */
    uint32_t metadata;
    /*! Data Event ID */
    uint32_t id;
    /*! Lower 32 bits of DE data. */
    uint32_t data_low;
    /*! Higher 32 bits of DE data. */
    uint32_t data_high;
    /*! Lower 32 bits of Line timestamp. */
    uint32_t timestamp_low;
    /*! Higher 32 bits of Line timestamp. */
    uint32_t timestamp_high;
};

/*!
 * \brief DE data line without Timestamp.
 */
struct FWK_PACKED de_non_ts_line {
    /*! LineMetadata (Bit3=1, Bit1=1, rest 0) */
    uint32_t metadata;
    /*! Data Event ID */
    uint32_t id;
    /*! Lower 32 bits of DE data. */
    uint32_t data_low;
    /*! Higher 32 bits of DE data. */
    uint32_t data_high;
};

/*!
 * \brief Block Timestamp Line.
 */
struct FWK_PACKED block_timestamp_line {
    /*! LineMetadata (Bit3=1, Bit1=1, rest 0) */
    uint32_t metadata;
    /*! Data Event ID(=0)*/
    uint32_t id;
    /*! Lower 32 bits of Block timestamp. */
    uint32_t block_timestamp_low;
    /*! Higher 32 bits of Block timestamp. */
    uint32_t block_timestamp_high;
};

#endif /* !TDCF_DEFS_H */
