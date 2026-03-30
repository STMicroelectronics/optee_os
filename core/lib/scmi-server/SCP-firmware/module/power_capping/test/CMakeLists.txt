#
# Arm SCP/MCP Software
# Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

set(TEST_SRC mod_power_capping)
set(TEST_FILE mod_power_capping)

if(TEST_ON_TARGET)
    set(TEST_MODULE power_capping)
    set(MODULE_ROOT ${CMAKE_SOURCE_DIR}/module)
else()
    set(UNIT_TEST_TARGET mod_${TEST_MODULE}_unit_test)
endif()

set(MODULE_SRC ${MODULE_ROOT}/${TEST_MODULE}/src)
set(MODULE_INC ${MODULE_ROOT}/${TEST_MODULE}/include)
set(MODULE_UT_SRC ${CMAKE_CURRENT_LIST_DIR})
set(MODULE_UT_INC ${CMAKE_CURRENT_LIST_DIR})
set(MODULE_UT_MOCK_SRC ${CMAKE_CURRENT_LIST_DIR}/mocks)

list(APPEND MOCK_REPLACEMENTS fwk_core)
list(APPEND MOCK_REPLACEMENTS fwk_mm)
list(APPEND MOCK_REPLACEMENTS fwk_module)
list(APPEND MOCK_REPLACEMENTS fwk_notification)

list(APPEND OTHER_MODULE_INC ${MODULE_ROOT}/pid_controller/include)
list(APPEND OTHER_MODULE_INC ${SCP_ROOT}/interface/power_management)

include(${SCP_ROOT}/unit_test/module_common.cmake)

target_sources(${UNIT_TEST_TARGET}
    PRIVATE ${MODULE_UT_MOCK_SRC}/fwk_variables_mocks.c
    PRIVATE ${MODULE_UT_MOCK_SRC}/Mockmod_power_capping_extra.c)
