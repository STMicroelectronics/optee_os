/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Integration test module definition.
 */

#ifndef MOD_INTEGRATION_TEST_H
#define MOD_INTEGRATION_TEST_H

#include <fwk_event.h>
#include <fwk_id.h>

#include <stddef.h>
#include <stdint.h>

/*!
 * \ingroup GroupModules
 * \defgroup GroupIntegrationTest Integration Test
 * \{
 */

/*!
 * \brief Event indices.
 */
enum mod_integration_test_event_idx {
    MOD_INTEGRATION_TEST_EVENT_IDX_RUN_TEST_SUITE,
    MOD_INTEGRATION_TEST_EVENT_IDX_CONTINUE_STEP,
    MOD_INTEGRATION_TEST_EVENT_IDX_COUNT,
};

/*!
 * \brief Event ID used by test modules to signal continuation of a test step.
 */
#define MOD_INTEGRATION_TEST_EVENT_ID_STEP_CONTINUE \
    FWK_ID_EVENT_INIT( \
        FWK_MODULE_IDX_INTEGRATION_TEST, \
        MOD_INTEGRATION_TEST_EVENT_IDX_CONTINUE_STEP)

/*!
 * \brief APIs that the module makes available to entities requesting binding.
 */
enum mod_integration_test_api_idx {
    /*! Integration test */
    MOD_INTEGRATION_TEST_API_IDX_TEST,

    /*! Number of defined APIs */
    MOD_INTEGRATION_TEST_API_COUNT,
};

/*!
 * \brief API to be exposed by test modules.
 */
struct mod_integration_test_api {
    /*! Run test suite */
    int (*run)(
        unsigned int case_idx,
        unsigned int step_idx,
        const struct fwk_event *event);

    /*! Obtain test case name */
    const char *(*test_name)(unsigned int case_idx);

    /*! Setup before each test case (may be NULL) */
    void (*setup)(void);

    /*! Teardown after each test case (may be NULL) */
    void (*teardown)(void);
};

/*!
 * \brief Integration test device descriptor
 */
struct mod_integration_test_config {
    /*!
     * \brief Test module ID
     */
    fwk_id_t test_id;

    /*!
     * \brief Run the test suite automatically at boot
     *
     * This may be useful during test development.
     */
    bool run_at_start;
    /*!
     * \brief Number of Test cases
     */
    unsigned int num_test_cases;
};

/*!
 * \}
 */

#endif /* MOD_INTEGRATION_TEST_H */
