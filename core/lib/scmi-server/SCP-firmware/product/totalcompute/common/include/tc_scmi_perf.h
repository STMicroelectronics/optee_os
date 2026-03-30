/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef TC_SCMI_PERF_H
#define TC_SCMI_PERF_H

#include "scp_mmap.h"

#include <mod_fch_polled.h>
#include <mod_scmi_perf.h>

#include <stdint.h>

#define FC_LEVEL_SET_ADDR(PERF_IDX) \
    (SCMI_FAST_CHANNEL_BASE + MOD_SCMI_PERF_FAST_CHANNEL_OFFSET_LEVEL_SET + \
     (MOD_SCMI_PERF_FAST_CHANNEL_OFFSET_TOTAL * PERF_IDX))

#define FC_LIMIT_SET_ADDR(PERF_IDX) \
    (SCMI_FAST_CHANNEL_BASE + MOD_SCMI_PERF_FAST_CHANNEL_OFFSET_LIMIT_SET + \
     (MOD_SCMI_PERF_FAST_CHANNEL_OFFSET_TOTAL * PERF_IDX))

#define FC_LEVEL_GET_ADDR(PERF_IDX) \
    (SCMI_FAST_CHANNEL_BASE + MOD_SCMI_PERF_FAST_CHANNEL_OFFSET_LEVEL_GET + \
     (MOD_SCMI_PERF_FAST_CHANNEL_OFFSET_TOTAL * PERF_IDX))

#define FC_LIMIT_GET_ADDR(PERF_IDX) \
    (SCMI_FAST_CHANNEL_BASE + MOD_SCMI_PERF_FAST_CHANNEL_OFFSET_LIMIT_GET + \
     (MOD_SCMI_PERF_FAST_CHANNEL_OFFSET_TOTAL * PERF_IDX))

#define FCH_POLLED_LEVEL_SET_LENGTH sizeof(uint32_t)
#define FCH_POLLED_LIMIT_SET_LENGTH \
    sizeof(struct mod_scmi_perf_fast_channel_limit)
#define FCH_POLLED_LEVEL_GET_LENGTH sizeof(uint32_t)
#define FCH_POLLED_LIMIT_GET_LENGTH \
    sizeof(struct mod_scmi_perf_fast_channel_limit)

#define FCH_ADDR_INIT(scp_addr, ap_addr, len) \
    &((struct mod_fch_polled_channel_config){ \
        .fch_addr = { \
            .local_view_address = scp_addr, \
            .target_view_address = ap_addr, \
            .length = len, \
        } })

#endif /* TC_SCMI_PERF_H */
