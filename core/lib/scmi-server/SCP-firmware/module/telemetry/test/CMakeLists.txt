#
# Arm SCP/MCP Software
# Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

set(TEST_SRC telemetry_shmti_manager)
set(TEST_FILE telemetry_shmti_manager)

set(UNIT_TEST_TARGET mod_${TEST_MODULE}_shmti_manager_unit_test)

set(MODULE_SRC ${MODULE_ROOT}/${TEST_MODULE}/src)
set(MODULE_INC ${MODULE_ROOT}/${TEST_MODULE}/include)
list(APPEND OTHER_MODULE_INC ${MODULE_ROOT}/timer/include)

list(APPEND OTHER_MODULE_INC ${MODULE_ROOT}/telemetry/include/internal)

set(MODULE_UT_SRC ${CMAKE_CURRENT_LIST_DIR})
set(MODULE_UT_INC ${CMAKE_CURRENT_LIST_DIR})

list(APPEND MOCK_REPLACEMENTS fwk_module)
list(APPEND MOCK_REPLACEMENTS fwk_mm)

include(${SCP_ROOT}/unit_test/module_common.cmake)
