#
# Arm SCP/MCP Software
# Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

# Test mod_si0_platform.c

set(TEST_SRC mod_si0_platform)
set(TEST_FILE mod_si0_platform)

set(UNIT_TEST_TARGET mod_si0_platform_unit_test)

set(MODULE_SRC ${PRODUCT_ROOT}/${TEST_MODULE}/src)
set(MODULE_INC ${PRODUCT_ROOT}/${TEST_MODULE}/include)

set(MODULE_UT_SRC ${CMAKE_CURRENT_LIST_DIR})
set(MODULE_UT_INC ${CMAKE_CURRENT_LIST_DIR})
set(MODULE_UT_MOCK_SRC ${CMAKE_CURRENT_LIST_DIR}/mocks)

list(APPEND MOCK_REPLACEMENTS fwk_id)
list(APPEND MOCK_REPLACEMENTS fwk_module)
list(APPEND MOCK_REPLACEMENTS fwk_notification)

include(${SCP_ROOT}/unit_test/module_common.cmake)

target_compile_definitions(${UNIT_TEST_TARGET} PUBLIC
                           "BUILD_HAS_NOTIFICATION")

target_sources(${UNIT_TEST_TARGET}
    PRIVATE
        ${MODULE_UT_MOCK_SRC}/Mocksi0_platform.c)


# Test platform_power_mgmt.c

set(TEST_SRC platform_power_mgmt)
set(TEST_FILE platform_power_mgmt)

set(UNIT_TEST_TARGET mod_si0_platform_platform_power_mgmt_unit_test)

set(MODULE_SRC ${PRODUCT_ROOT}/${TEST_MODULE}/src)
set(MODULE_INC ${PRODUCT_ROOT}/${TEST_MODULE}/include)
list(APPEND OTHER_MODULE_INC ${MODULE_ROOT}/power_domain/include)
list(APPEND OTHER_MODULE_INC ${MODULE_ROOT}/system_power/include)
list(APPEND OTHER_MODULE_INC ${MODULE_ROOT}/scmi/include)

set(MODULE_UT_SRC ${CMAKE_CURRENT_LIST_DIR})
set(MODULE_UT_INC ${CMAKE_CURRENT_LIST_DIR})

list(APPEND MOCK_REPLACEMENTS fwk_id)
list(APPEND MOCK_REPLACEMENTS fwk_module)

include(${SCP_ROOT}/unit_test/module_common.cmake)

target_compile_definitions(${UNIT_TEST_TARGET} PUBLIC
                           "BUILD_HAS_MOD_POWER_DOMAIN")
