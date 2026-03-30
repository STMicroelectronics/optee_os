#
# Arm SCP/MCP Software
# Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

# Common paths
set(TEST_FILE               mod_ni_710ae)
set(MODULE_SRC              ${MODULE_ROOT}/${TEST_MODULE}/src)
set(MODULE_INC              ${MODULE_ROOT}/${TEST_MODULE}/include)
set(MODULE_UT_SRC           ${CMAKE_CURRENT_LIST_DIR})
set(MODULE_UT_INC           ${CMAKE_CURRENT_LIST_DIR})
set(MODULE_UT_MOCK_SRC      ${CMAKE_CURRENT_LIST_DIR}/mocks)

# Target A: Module tests (mocks discovery_drv)
set(UNIT_TEST_TARGET        mod_${TEST_MODULE}_unit_test)
set(TEST_SRC                mod_ni_710ae)

list(APPEND MOCK_REPLACEMENTS fwk_module fwk_id fwk_mm)

include(${SCP_ROOT}/unit_test/module_common.cmake)

target_compile_definitions(${UNIT_TEST_TARGET}
  PRIVATE TESTING_MOD_NI_710AE
          UNIT_TEST_SRC=\"${MODULE_SRC}/mod_ni_710ae.c\")

target_sources(${UNIT_TEST_TARGET}
  PRIVATE ${MODULE_UT_MOCK_SRC}/Mockni_710ae_discovery_drv.c
  PRIVATE ${MODULE_UT_MOCK_SRC}/Mockni_710ae_lib.c)

# Target B: Discovery driver tests
# Build a second test target from the same test file, but compile the real discovery_drv
set(UNIT_TEST_TARGET        ni_710ae_discovery_drv_unit_test)
set(TEST_SRC                ni_710ae_discovery_drv)

# Fresh mock list: keep framework mocks but not include discovery_drv mock
set(MOCK_REPLACEMENTS       "")
list(APPEND MOCK_REPLACEMENTS fwk_module fwk_id fwk_mm)

include(${SCP_ROOT}/unit_test/module_common.cmake)

target_compile_definitions(${UNIT_TEST_TARGET}
  PRIVATE TESTING_NI_710AE_DISCOVERY_DRV
          UNIT_TEST_SRC=\"${MODULE_SRC}/ni_710ae_discovery_drv.c\")

target_sources(${UNIT_TEST_TARGET}
  PRIVATE ${MODULE_UT_MOCK_SRC}/Mockni_710ae_lib.c)
