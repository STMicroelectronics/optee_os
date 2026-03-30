/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef TEST_FWK_MODULE_IDX_H
#define TEST_FWK_MODULE_IDX_H

#include <fwk_id.h>

enum fwk_module_idx {
    FWK_MODULE_IDX_PINCTRL,
    FWK_MODULE_IDX_PINCTRL_DRV,
    FWK_MODULE_IDX_COUNT,
};

enum fwk_module_element_idx {
    FWK_MODULE_PINCTRL_DRV_API,
    FWK_MODULE_DRV_API_COUNT
};

static const fwk_id_t fwk_module_id_pinctrl =
    FWK_ID_MODULE_INIT(FWK_MODULE_IDX_PINCTRL);

static const fwk_id_t pinctrl_drv_id =
    FWK_ID_MODULE(FWK_MODULE_IDX_PINCTRL_DRV);

static const fwk_id_t pinctrl_drv_api_id =
    FWK_ID_API(FWK_MODULE_IDX_PINCTRL_DRV, FWK_MODULE_PINCTRL_DRV_API);

#endif /* TEST_FWK_MODULE_IDX_H */
