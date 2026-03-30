\ingroup GroupModules Modules
\defgroup GroupIntegrationTest Integration Test

# Integration Test

This is a module to help orchestrate integration tests written using the Unity
test framework at runtime. Tests may run automatically at boot, or on demand
using the debugger CLI.

## Design goals

 * Use the same test framework for integration tests as for unit tests.
 * Test suites may use the event loop (i.e. tests triggered using the CLI are
   scheduled for execution once the CLI is exited). Tests may return to the
   event loop during test execution.
 * Test suites may be triggered on demand (i.e. do not interfere with the
   normal boot flow).
 * Test suites can be configured to run automatically during system boot

## Writing an integration test

Integration tests should be written as standard SCP-firmware modules which
expose the `mod_integration_test_api` API. At least the `run` and `test_name`
methods should be implemented:
 * `run`: Points to a method which switches control flow to the relevant
   test function
 * `test_name`: Points to a method which returns the name of the currently active
   test case.

The `setup` and `teardown` methods may also be implemented, which will execute
before and after (respectively) each test case in the test suite.

## Configuration

The integration test module should be configured at the product level using
element-based configuration. Each test suite is defined as a module element
with associated metadata, e.g.:

```C
#include <mod_integration_test.h>

#include <fwk_module.h>

static const struct fwk_element config_integration_test_elements[] = {
    [TEST_ABC] = {
        .name = "abc",
        .data = &(struct mod_integration_test_config){
            .test_id = FWK_ID_MODULE_INIT(FWK_MODULE_IDX_TEST_ABC),
            .run_at_start = false,
            .num_test_cases = 3,
        },
    },
    [TEST_COUNT] = { 0 },
};

/*
 * Configuration for the integration test module
 */
 struct fwk_module_config config_integration_test = {
    .elements = FWK_MODULE_STATIC_ELEMENTS_PTR(config_integration_test_elements),
};

```

## Usage

- Test suites may be triggered:
  - Automatically at boot (if configured with `run_at_start = true`)
  - On demand using the debugger CLI:
    1. Enter the debugger CLI using Ctrl+E.
    2. Run the command `test abc`, where abc is the test name defined above.
    3. Exit the debugger CLI using Ctrl+D to run the test.
