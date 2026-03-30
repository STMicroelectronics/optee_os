/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef TEST_FWK_MODULE_MODULE_IDX_H
#define TEST_FWK_MODULE_MODULE_IDX_H

#include <fwk_id.h>

enum fwk_module_idx {
    FWK_MODULE_IDX_CLUSTER_CONTROL,
    FWK_MODULE_IDX_TEST_MODULE,
    FWK_MODULE_IDX_COUNT,
};

static const fwk_id_t fwk_module_id_cluster_control =
    FWK_ID_MODULE_INIT(FWK_MODULE_IDX_CLUSTER_CONTROL);

static const fwk_id_t fwk_module_id_test_module =
    FWK_ID_MODULE_INIT(FWK_MODULE_IDX_TEST_MODULE);

enum test_module_notification_idx {
    TEST_MODULE_NOTIFICATION_TEST,
    TEST_MODULE_NOTIFICATION_COUNT,
};

static const fwk_id_t test_module_notification_test = FWK_ID_NOTIFICATION_INIT(
    FWK_MODULE_IDX_TEST_MODULE,
    TEST_MODULE_NOTIFICATION_TEST);

#endif /* TEST_FWK_MODULE_MODULE_IDX_H */
