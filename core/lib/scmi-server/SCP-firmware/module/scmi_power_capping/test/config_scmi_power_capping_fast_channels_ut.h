/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fwk_module_idx.h"
#include "internal/scmi_power_capping_fast_channels.h"
#include "mod_scmi_power_capping_unit_test.h"
#include "mod_scmi_std.h"
#include "mod_transport.h"

enum test_fch_index {
    TEST_FCH_IDX_CAP_GET_1,
    TEST_FCH_IDX_CAP_GET_2,
    TEST_FCH_IDX_CAP_GET_3,
    TEST_FCH_IDX_CAP_SET_1,
    TEST_FCH_IDX_CAP_SET_2,
    TEST_FCH_IDX_CAP_SET_3,
    TEST_FCH_IDX_PAI_GET_1,
    TEST_FCH_IDX_PAI_GET_2,
    TEST_FCH_IDX_PAI_GET_3,
    TEST_FCH_IDX_PAI_SET_1,
    TEST_FCH_IDX_PAI_SET_2,
    TEST_FCH_IDX_PAI_SET_3,
    TEST_FCH_IDX_COUNT
};

static const struct scmi_pcapping_fch_config test_fch_config[TEST_FCH_IDX_COUNT] = {
    [TEST_FCH_IDX_CAP_GET_1] = {
        .transport_id = FWK_ID_ELEMENT_INIT(
            FWK_MODULE_IDX_TRANSPORT,
            TEST_FCH_IDX_CAP_GET_1),
        .transport_api_id = FWK_ID_API_INIT(
            FWK_MODULE_IDX_TRANSPORT,
            MOD_TRANSPORT_API_IDX_FAST_CHANNELS),
        .service_id = FWK_ID_ELEMENT_INIT(FAKE_SCMI_MODULE_ID, 0u),
        .scmi_power_capping_domain_idx = 0u,
        .message_id = MOD_SCMI_POWER_CAPPING_CAP_GET,
    },
    [TEST_FCH_IDX_CAP_GET_2] = {
        .transport_id = FWK_ID_ELEMENT_INIT(
            FWK_MODULE_IDX_TRANSPORT,
            TEST_FCH_IDX_CAP_GET_2),
        .transport_api_id = FWK_ID_API_INIT(
            FWK_MODULE_IDX_TRANSPORT,
            MOD_TRANSPORT_API_IDX_FAST_CHANNELS),
        .service_id = FWK_ID_ELEMENT_INIT(FAKE_SCMI_MODULE_ID, 1u),
        .scmi_power_capping_domain_idx = 1u,
        .message_id = MOD_SCMI_POWER_CAPPING_CAP_GET,
    },
    [TEST_FCH_IDX_CAP_GET_3] = {
        .transport_id = FWK_ID_ELEMENT_INIT(
            FWK_MODULE_IDX_TRANSPORT,
            TEST_FCH_IDX_CAP_GET_3),
        .transport_api_id = FWK_ID_API_INIT(
            FWK_MODULE_IDX_TRANSPORT,
            MOD_TRANSPORT_API_IDX_FAST_CHANNELS),
        .service_id = FWK_ID_ELEMENT_INIT(FAKE_SCMI_MODULE_ID, 1u),
        .scmi_power_capping_domain_idx = 2u,
        .message_id = MOD_SCMI_POWER_CAPPING_CAP_GET,
    },
    [TEST_FCH_IDX_CAP_SET_1] = {
        .transport_id = FWK_ID_ELEMENT_INIT(
            FWK_MODULE_IDX_TRANSPORT,
            TEST_FCH_IDX_CAP_SET_1),
        .transport_api_id = FWK_ID_API_INIT(
            FWK_MODULE_IDX_TRANSPORT,
            MOD_TRANSPORT_API_IDX_FAST_CHANNELS),
        .service_id = FWK_ID_ELEMENT_INIT(FAKE_SCMI_MODULE_ID, 2u),
        .scmi_power_capping_domain_idx = 1u,
        .message_id = MOD_SCMI_POWER_CAPPING_CAP_SET,
    },
    [TEST_FCH_IDX_CAP_SET_2] = {
        .transport_id = FWK_ID_ELEMENT_INIT(
            FWK_MODULE_IDX_TRANSPORT,
            TEST_FCH_IDX_CAP_SET_2),
        .transport_api_id = FWK_ID_API_INIT(
            FWK_MODULE_IDX_TRANSPORT,
            MOD_TRANSPORT_API_IDX_FAST_CHANNELS),
        .service_id = FWK_ID_ELEMENT_INIT(FAKE_SCMI_MODULE_ID, 3u),
        .scmi_power_capping_domain_idx = 2u,
        .message_id = MOD_SCMI_POWER_CAPPING_CAP_SET,
    },
    [TEST_FCH_IDX_CAP_SET_3] = {
        .transport_id = FWK_ID_ELEMENT_INIT(
            FWK_MODULE_IDX_TRANSPORT,
            TEST_FCH_IDX_CAP_SET_3),
        .transport_api_id = FWK_ID_API_INIT(
            FWK_MODULE_IDX_TRANSPORT,
            MOD_TRANSPORT_API_IDX_FAST_CHANNELS),
        .service_id = FWK_ID_ELEMENT_INIT(FAKE_SCMI_MODULE_ID, 3u),
        .scmi_power_capping_domain_idx = 3u,
        .message_id = MOD_SCMI_POWER_CAPPING_CAP_SET,
    },
    [TEST_FCH_IDX_PAI_GET_1] = {
        .transport_id = FWK_ID_ELEMENT_INIT(
            FWK_MODULE_IDX_TRANSPORT,
            TEST_FCH_IDX_PAI_GET_1),
        .transport_api_id = FWK_ID_API_INIT(
            FWK_MODULE_IDX_TRANSPORT,
            MOD_TRANSPORT_API_IDX_FAST_CHANNELS),
        .service_id = FWK_ID_ELEMENT_INIT(FAKE_SCMI_MODULE_ID, 4u),
        .scmi_power_capping_domain_idx = 3u,
        .message_id = MOD_SCMI_POWER_CAPPING_PAI_GET,
    },
    [TEST_FCH_IDX_PAI_GET_2] = {
        .transport_id = FWK_ID_ELEMENT_INIT(
            FWK_MODULE_IDX_TRANSPORT,
            TEST_FCH_IDX_PAI_GET_2),
        .transport_api_id = FWK_ID_API_INIT(
            FWK_MODULE_IDX_TRANSPORT,
            MOD_TRANSPORT_API_IDX_FAST_CHANNELS),
        .service_id = FWK_ID_ELEMENT_INIT(FAKE_SCMI_MODULE_ID, 5u),
        .scmi_power_capping_domain_idx = 4u,
        .message_id = MOD_SCMI_POWER_CAPPING_PAI_GET,
    },
    [TEST_FCH_IDX_PAI_GET_3] = {
        .transport_id = FWK_ID_ELEMENT_INIT(
            FWK_MODULE_IDX_TRANSPORT,
            TEST_FCH_IDX_PAI_GET_3),
        .transport_api_id = FWK_ID_API_INIT(
            FWK_MODULE_IDX_TRANSPORT,
            MOD_TRANSPORT_API_IDX_FAST_CHANNELS),
        .service_id = FWK_ID_ELEMENT_INIT(FAKE_SCMI_MODULE_ID, 5u),
        .scmi_power_capping_domain_idx = 5u,
        .message_id = MOD_SCMI_POWER_CAPPING_PAI_GET,
    },
    [TEST_FCH_IDX_PAI_SET_1] = {
        .transport_id = FWK_ID_ELEMENT_INIT(
            FWK_MODULE_IDX_TRANSPORT,
            TEST_FCH_IDX_PAI_SET_1),
        .transport_api_id = FWK_ID_API_INIT(
            FWK_MODULE_IDX_TRANSPORT,
            MOD_TRANSPORT_API_IDX_FAST_CHANNELS),
        .service_id = FWK_ID_ELEMENT_INIT(FAKE_SCMI_MODULE_ID, 6u),
        .scmi_power_capping_domain_idx = 4u,
        .message_id = MOD_SCMI_POWER_CAPPING_PAI_SET,
    },
    [TEST_FCH_IDX_PAI_SET_2] = {
        .transport_id = FWK_ID_ELEMENT_INIT(
            FWK_MODULE_IDX_TRANSPORT,
            TEST_FCH_IDX_PAI_SET_2),
        .transport_api_id = FWK_ID_API_INIT(
            FWK_MODULE_IDX_TRANSPORT,
            MOD_TRANSPORT_API_IDX_FAST_CHANNELS),
        .service_id = FWK_ID_ELEMENT_INIT(FAKE_SCMI_MODULE_ID, 7u),
        .scmi_power_capping_domain_idx = 5u,
        .message_id = MOD_SCMI_POWER_CAPPING_PAI_SET,
    },
    [TEST_FCH_IDX_PAI_SET_3] = {
        .transport_id = FWK_ID_ELEMENT_INIT(
            FWK_MODULE_IDX_TRANSPORT,
            TEST_FCH_IDX_PAI_SET_3),
        .transport_api_id = FWK_ID_API_INIT(
            FWK_MODULE_IDX_TRANSPORT,
            MOD_TRANSPORT_API_IDX_FAST_CHANNELS),
        .service_id = FWK_ID_ELEMENT_INIT(FAKE_SCMI_MODULE_ID, 7u),
        .scmi_power_capping_domain_idx = 6u,
        .message_id = MOD_SCMI_POWER_CAPPING_PAI_SET,
    },
};
