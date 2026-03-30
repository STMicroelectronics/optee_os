#
# Arm SCP/MCP Software
# Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

# Default flags target

set(TEST_SRC mod_scmi_pin_control)
set(TEST_FILE mod_scmi_pin_control)

set(UNIT_TEST_TARGET mod_${TEST_MODULE}_unit_test)

set(MODULE_SRC ${MODULE_ROOT}/${TEST_MODULE}/src)
set(MODULE_INC ${MODULE_ROOT}/${TEST_MODULE}/include)
list(APPEND OTHER_MODULE_INC ${MODULE_ROOT}/scmi/include)
list(APPEND OTHER_MODULE_INC ${MODULE_ROOT}/pinctrl/include)
list(APPEND OTHER_MODULE_INC ${MODULE_ROOT}/pinctrl_drv/include)
set(MODULE_UT_SRC ${CMAKE_CURRENT_LIST_DIR})
set(MODULE_UT_INC ${CMAKE_CURRENT_LIST_DIR})
set(MODULE_UT_MOCK_SRC ${CMAKE_CURRENT_LIST_DIR}/mocks)

list(APPEND MOCK_REPLACEMENTS fwk_module)
list(APPEND MOCK_REPLACEMENTS fwk_mm)
list(APPEND MOCK_REPLACEMENTS fwk_id)
list(APPEND MOCK_REPLACEMENTS fwk_core)

include(${SCP_ROOT}/unit_test/module_common.cmake)

target_sources(${UNIT_TEST_TARGET}
    PRIVATE
        ${MODULE_UT_MOCK_SRC}/Mockmod_scmi_extra.c)
target_sources(${UNIT_TEST_TARGET}
        PRIVATE
            ${MODULE_UT_MOCK_SRC}/Mockmod_pinctrl_extra.c)
