/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "mod_power_capping.h"

#include <Mockfwk_module.h>

#include <fwk_element.h>
#include <fwk_id.h>
#include <fwk_module_idx.h>

enum test_power_capping_domain_idx {
    TEST_CORE_0,
    TEST_SOC,
    TEST_DOMAIN_COUNT,
};

enum test_power_limit_set_notifier_idx {
    TEST_POWER_LIMIT_SET_NOTIFIER_CORE_0,
    TEST_POWER_LIMIT_SET_NOTIFIER_SOC,
    TEST_POWER_LIMIT_SET_NOTIFIER_COUNT,
};

enum test_power_limit_set_notification_idx {
    TEST_POWER_LIMIT_SET_NOTIFICATION_CORE_0,
    TEST_POWER_LIMIT_SET_NOTIFICATION_SOC,
    TEST_POWER_LIMIT_SET_NOTIFICATION_COUNT,
};

struct fwk_element test_domain_config[TEST_DOMAIN_COUNT] = {
    [TEST_CORE_0] = {
        .data = &(struct mod_power_capping_domain_config) {
            .power_limit_set_notifier_id = FWK_ID_ELEMENT_INIT(
                FWK_MODULE_IDX_FAKE_LIMIT_NOTIFIER,
                TEST_POWER_LIMIT_SET_NOTIFIER_CORE_0),
            .power_limit_set_notification_id = FWK_ID_NOTIFICATION_INIT(
                FWK_MODULE_IDX_FAKE_LIMIT_NOTIFIER,
                TEST_POWER_LIMIT_SET_NOTIFICATION_CORE_0),
        },
    },
    [TEST_SOC] = {
        .data = &(struct mod_power_capping_domain_config) {
            .power_limit_set_notifier_id = FWK_ID_ELEMENT_INIT(
                FWK_MODULE_IDX_FAKE_LIMIT_NOTIFIER,
                TEST_POWER_LIMIT_SET_NOTIFIER_SOC),
            .power_limit_set_notification_id = FWK_ID_NOTIFICATION_INIT(
                FWK_MODULE_IDX_FAKE_LIMIT_NOTIFIER,
                TEST_POWER_LIMIT_SET_NOTIFICATION_SOC),
        },
    },
};
