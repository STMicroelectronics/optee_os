/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef TEST_FWK_MODULE_MODULE_IDX_H
#define TEST_FWK_MODULE_MODULE_IDX_H

enum fwk_module_idx {
    FWK_MODULE_IDX_SI0_PLATFORM,
    FWK_MODULE_IDX_TRANSPORT,
    FWK_MODULE_IDX_TIMER,
    FWK_MODULE_IDX_POWER_DOMAIN,
    FWK_MODULE_IDX_SCMI,
    FWK_MODULE_IDX_COUNT,
};

static const fwk_id_t fwk_module_id_si0_platform =
    FWK_ID_MODULE_INIT(FWK_MODULE_IDX_SI0_PLATFORM);

static const fwk_id_t fwk_module_id_transport =
    FWK_ID_MODULE_INIT(FWK_MODULE_IDX_TRANSPORT);

static const fwk_id_t fwk_module_id_timer =
    FWK_ID_MODULE_INIT(FWK_MODULE_IDX_TIMER);

static const fwk_id_t fwk_module_id_power_domain =
    FWK_ID_MODULE_INIT(FWK_MODULE_IDX_POWER_DOMAIN);

#endif /* TEST_FWK_MODULE_MODULE_IDX_H */
