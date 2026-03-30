#
# Arm SCP/MCP Software
# Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

set(TEST_SRC mod_pfdi_monitor)
set(TEST_FILE mod_pfdi_monitor)

set(UNIT_TEST_TARGET mod_pfdi_monitor_unit_test)

list(APPEND OTHER_MODULE_INC ${MODULE_ROOT}/timer/include)
list(APPEND OTHER_MODULE_INC ${MODULE_ROOT}/power_domain/include)

set(MODULE_SRC ${PRODUCT_ROOT}/${TEST_MODULE}/src)
set(MODULE_INC ${PRODUCT_ROOT}/${TEST_MODULE}/include)

set(MODULE_UT_SRC ${CMAKE_CURRENT_LIST_DIR})
set(MODULE_UT_INC ${CMAKE_CURRENT_LIST_DIR})

list(APPEND MOCK_REPLACEMENTS fwk_id)
list(APPEND MOCK_REPLACEMENTS fwk_module)
list(APPEND MOCK_REPLACEMENTS fwk_notification)

include(${SCP_ROOT}/unit_test/module_common.cmake)

target_compile_definitions(${UNIT_TEST_TARGET} PUBLIC
                           "BUILD_HAS_NOTIFICATION")
