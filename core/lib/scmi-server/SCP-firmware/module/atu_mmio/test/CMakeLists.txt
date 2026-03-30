#
# Arm SCP/MCP Software
# Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

set(TEST_SRC mod_atu_mmio)
set(TEST_FILE mod_atu_mmio)

set(UNIT_TEST_TARGET mod_${TEST_MODULE}_unit_test)

set(MODULE_SRC ${MODULE_ROOT}/${TEST_MODULE}/src)
set(MODULE_INC ${MODULE_ROOT}/${TEST_MODULE}/include)
set(MODULE_UT_SRC ${CMAKE_CURRENT_LIST_DIR})
set(MODULE_UT_INC ${CMAKE_CURRENT_LIST_DIR})
set(MODULE_UT_MOCK_SRC ${CMAKE_CURRENT_LIST_DIR}/mocks)
list(APPEND OTHER_MODULE_INC ${MODULE_ROOT}/atu/include)
list(APPEND OTHER_MODULE_INC ${SCP_ROOT}/interface/address_remapper)

list(APPEND MOCK_REPLACEMENTS fwk_id)
list(APPEND MOCK_REPLACEMENTS fwk_module)

include(${SCP_ROOT}/unit_test/module_common.cmake)
