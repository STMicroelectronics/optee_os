#
# Arm SCP/MCP Software
# Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

set(TEST_SRC mod_ros_clock)
set(TEST_FILE mod_ros_clock)

set(UNIT_TEST_TARGET mod_ros_clock_unit_test)

set(MODULE_SRC ${PRODUCT_ROOT}/${TEST_MODULE}/src)
set(MODULE_INC ${PRODUCT_ROOT}/${TEST_MODULE}/include)

list(APPEND OTHER_MODULE_INC ${MODULE_ROOT}/power_domain/include)
list(APPEND OTHER_MODULE_INC ${MODULE_ROOT}/clock/include)

set(MODULE_UT_SRC ${CMAKE_CURRENT_LIST_DIR})
set(MODULE_UT_INC ${CMAKE_CURRENT_LIST_DIR})

list(APPEND MOCK_REPLACEMENTS fwk_id)
list(APPEND MOCK_REPLACEMENTS fwk_module)

include(${SCP_ROOT}/unit_test/module_common.cmake)

target_compile_definitions(${UNIT_TEST_TARGET} PUBLIC
                           "BUILD_HAS_MOD_POWER_DOMAIN")
target_compile_definitions(${UNIT_TEST_TARGET} PUBLIC
                           "BUILD_HAS_MOD_CLOCK")
