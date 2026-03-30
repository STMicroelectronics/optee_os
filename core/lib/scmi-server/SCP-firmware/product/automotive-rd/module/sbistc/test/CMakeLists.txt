#
# Arm SCP/MCP Software
# Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

set(TEST_SRC mod_sbistc)
set(TEST_FILE mod_sbistc)

set(UNIT_TEST_TARGET mod_sbistc_unit_test)

set(MODULE_SRC ${PRODUCT_ROOT}/${TEST_MODULE}/src)
set(MODULE_INC ${PRODUCT_ROOT}/${TEST_MODULE}/include)
list(APPEND OTHER_MODULE_INC ${PRODUCT_ROOT}/automotive-rd/module/fmu/include)

set(MODULE_UT_SRC ${CMAKE_CURRENT_LIST_DIR})
set(MODULE_UT_INC ${CMAKE_CURRENT_LIST_DIR})
set(MODULE_UT_MOCK_SRC ${CMAKE_CURRENT_LIST_DIR}/mocks)

list(APPEND MOCK_REPLACEMENTS fwk_id)
list(APPEND MOCK_REPLACEMENTS fwk_module)
list(APPEND MOCK_REPLACEMENTS fwk_notification)

include(${SCP_ROOT}/unit_test/module_common.cmake)

target_sources(${UNIT_TEST_TARGET}
    PRIVATE
        ${MODULE_UT_MOCK_SRC}/Mocktest_fmu_extras.c)


target_compile_definitions(${UNIT_TEST_TARGET} PUBLIC "BUILD_HAS_NOTIFICATION")
