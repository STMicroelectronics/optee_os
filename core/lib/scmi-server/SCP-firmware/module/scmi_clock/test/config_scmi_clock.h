/*
 * Arm SCP/MCP Software
 * Copyright (c) 2015-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <mod_scmi_clock.h>

#include <fwk_id.h>
#include <fwk_macros.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>

#define FAKE_MODULE_IDX 0x5
#define MAX_PENDING_TRANSACTION 100

enum fake_scmi_agent {
    PLATFORM = 0,
    FAKE_SCMI_AGENT_IDX_PSCI = 1,
    FAKE_SCMI_AGENT_IDX_OSPM0,
    FAKE_SCMI_AGENT_IDX_OSPM1,
    FAKE_SCMI_AGENT_IDX_COUNT,
};

/*!
 * \brief Clock device indexes.
 */
enum clock_dev_idx {
    CLOCK_DEV_IDX_FAKE0,
    CLOCK_DEV_IDX_FAKE1,
    CLOCK_DEV_IDX_FAKE2,
    CLOCK_DEV_IDX_FAKE3,
    CLOCK_DEV_IDX_COUNT
};

/*!
 * \brief OSPM0 SCMI Clock indexes.
 */
enum scmi_clock_ospm0_idx {
    SCMI_CLOCK_OSPM0_IDX0,
    SCMI_CLOCK_OSPM0_IDX1,
    SCMI_CLOCK_OSPM0_IDX2,
    SCMI_CLOCK_OSPM0_IDX3,
    SCMI_CLOCK_OSPM0_COUNT
};

/*!
 * \brief OSPM1 SCMI Clock indexes.
 */
enum scmi_clock_ospm1_idx {
    SCMI_CLOCK_OSPM1_IDX0,
    SCMI_CLOCK_OSPM1_COUNT
};

static uint8_t dev_clock_ref_count_table[CLOCK_DEV_IDX_COUNT];

static uint8_t ospm0_state_table[SCMI_CLOCK_OSPM0_COUNT];
static uint8_t ospm1_state_table[SCMI_CLOCK_OSPM1_COUNT];

static uint8_t ospm0_state_table_expected
    [SCMI_CLOCK_OSPM0_COUNT];

static uint8_t ospm1_state_table_expected
    [SCMI_CLOCK_OSPM1_COUNT];

static uint8_t dev_clock_ref_count_table_expected[CLOCK_DEV_IDX_COUNT];

static uint8_t ospm0_state_table_default
    [SCMI_CLOCK_OSPM0_COUNT] = {
        MOD_CLOCK_STATE_RUNNING,
        MOD_CLOCK_STATE_RUNNING,
        MOD_CLOCK_STATE_RUNNING,
        MOD_CLOCK_STATE_RUNNING,
    };

static uint8_t ospm1_state_table_default
    [SCMI_CLOCK_OSPM1_COUNT] = {
        MOD_CLOCK_STATE_RUNNING,
    };

static const uint8_t dev_clock_ref_count_table_default[CLOCK_DEV_IDX_COUNT] = {
    1,
    1,
    1,
    2,
};

static struct clock_operations clock_ops_table[CLOCK_DEV_IDX_COUNT];

#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
#define INIT_SUBSCRIBERS(SUBSCRIBER_COUNT, VALUE) \
    { [0 ... (FAKE_SCMI_AGENT_IDX_COUNT - 1)] = VALUE }

#define INIT_RESOURCES(RESOURCE_COUNT, SUBSCRIBER_COUNT, VALUE) \
    { [0 ... (RESOURCE_COUNT - 1)] = INIT_SUBSCRIBERS(SUBSCRIBER_COUNT, VALUE) }

#define INIT_TABLE(OPERATION_COUNT, RESOURCE_COUNT, SUBSCRIBER_COUNT, VALUE) \
    { [0 ... (OPERATION_COUNT -1)] = INIT_RESOURCES(RESOURCE_COUNT, SUBSCRIBER_COUNT, VALUE) }

// static fwk_id_t subscriber_table_default[SCMI_CLOCK_NOTIFICATION_COUNT][CLOCK_DEV_IDX_COUNT][FAKE_SCMI_AGENT_IDX_COUNT] =
//     INIT_TABLE(SCMI_CLOCK_NOTIFICATION_COUNT, CLOCK_DEV_IDX_COUNT, FAKE_SCMI_AGENT_IDX_COUNT, FWK_ID_NONE);

// static fwk_id_t subscriber_table[SCMI_CLOCK_NOTIFICATION_COUNT][CLOCK_DEV_IDX_COUNT][FAKE_SCMI_AGENT_IDX_COUNT] =
//     INIT_TABLE(SCMI_CLOCK_NOTIFICATION_COUNT, CLOCK_DEV_IDX_COUNT, FAKE_SCMI_AGENT_IDX_COUNT, FWK_ID_NONE);

static fwk_id_t subscriber_table_default[FAKE_SCMI_AGENT_IDX_COUNT] = {
    FWK_ID_NONE,
    FWK_ID_NONE,
    FWK_ID_NONE,
    FWK_ID_NONE
};

static fwk_id_t *resource_table_default[CLOCK_DEV_IDX_COUNT] = {
    [0] = subscriber_table_default,
    [1] = subscriber_table_default,
    [2] = subscriber_table_default,
    [3] = subscriber_table_default,
};

static fwk_id_t **operation_table_default[SCMI_CLOCK_NOTIFICATION_COUNT] =
{
    [0] = resource_table_default,
    [1] = resource_table_default,
};

static fwk_id_t subscriber_table[FAKE_SCMI_AGENT_IDX_COUNT] = {
    FWK_ID_NONE,
    FWK_ID_NONE,
    FWK_ID_NONE,
    FWK_ID_NONE
};

static fwk_id_t *resource_table[CLOCK_DEV_IDX_COUNT] = {
    [0] = subscriber_table,
    [1] = subscriber_table,
    [2] = subscriber_table,
    [3] = subscriber_table,
};

static fwk_id_t **operation_table[SCMI_CLOCK_NOTIFICATION_COUNT] =
{
    [0] = resource_table,
    [1] = resource_table,
};
#endif

static const struct mod_scmi_clock_device clock_dev_fake0 = {
    .element_id =
            FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_CLOCK, CLOCK_DEV_IDX_FAKE0),
    .starts_enabled = true,
};

static const struct mod_scmi_clock_device clock_dev_fake1 = {
    .element_id =
            FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_CLOCK, CLOCK_DEV_IDX_FAKE1),
    .starts_enabled = true,
    .supports_extended_name = true,
#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
    .notify_changed_rate = true,
    .notify_requested_rate = true,
#endif
};

static const struct mod_scmi_clock_device clock_dev_fake2 = {
    .element_id =
            FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_CLOCK, CLOCK_DEV_IDX_FAKE2),
    .starts_enabled = true,
};

static const struct mod_scmi_clock_device clock_dev_fake3 = {
    .element_id =
            FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_CLOCK, CLOCK_DEV_IDX_FAKE3),
    .starts_enabled = true,
    .supports_extended_name = true,
};

static const struct mod_scmi_clock_device ospm0_device_table[SCMI_CLOCK_OSPM0_COUNT] = {
    [SCMI_CLOCK_OSPM0_IDX0] = clock_dev_fake0,
    [SCMI_CLOCK_OSPM0_IDX1] = clock_dev_fake1,
    [SCMI_CLOCK_OSPM0_IDX2] = clock_dev_fake2,
    [SCMI_CLOCK_OSPM0_IDX3] = clock_dev_fake3,
};

static const struct mod_scmi_clock_device ospm1_device_table[SCMI_CLOCK_OSPM1_COUNT] = {
    [SCMI_CLOCK_OSPM1_IDX0] = clock_dev_fake3,
};

static const struct mod_scmi_clock_agent_config ospm0_config = {
    .device_table = ospm0_device_table,
    .agent_device_count = FWK_ARRAY_SIZE(ospm0_device_table),
};

static const struct mod_scmi_clock_agent_config ospm1_config = {
    .device_table = ospm1_device_table,
    .agent_device_count = FWK_ARRAY_SIZE(ospm1_device_table),
};

static const struct fwk_element element_table[FAKE_SCMI_AGENT_IDX_COUNT] = {
    [PLATFORM] = {
        .data = &(const struct mod_scmi_clock_agent_config){ 0 },
    },
    [FAKE_SCMI_AGENT_IDX_PSCI] = {
        .data = &(const struct mod_scmi_clock_agent_config){ 0 },
    },
    [FAKE_SCMI_AGENT_IDX_OSPM0] = {
        .data = &ospm0_config,
    },
    [FAKE_SCMI_AGENT_IDX_OSPM1] = {
        .data = &ospm1_config,
    },
};

struct fwk_module_config config_scmi_clock = {
    .elements = FWK_MODULE_STATIC_ELEMENTS_PTR(element_table),
    .data = &((struct mod_scmi_clock_config) {
        .max_pending_transactions = MAX_PENDING_TRANSACTION,
    }),
};

static struct mod_scmi_clock_agent ospm0_table = {
    .agent_config = &ospm0_config,
    .state_table = ospm0_state_table_default,
};

static struct mod_scmi_clock_agent ospm1_table = {
    .agent_config = &ospm1_config,
    .state_table = ospm1_state_table_default,
};

static struct mod_scmi_clock_agent *agent_table[FAKE_SCMI_AGENT_IDX_COUNT] = {
    [PLATFORM] = NULL,
    [FAKE_SCMI_AGENT_IDX_PSCI] = NULL,
    [FAKE_SCMI_AGENT_IDX_OSPM0] = &ospm0_table,
    [FAKE_SCMI_AGENT_IDX_OSPM1] = &ospm1_table,
};
