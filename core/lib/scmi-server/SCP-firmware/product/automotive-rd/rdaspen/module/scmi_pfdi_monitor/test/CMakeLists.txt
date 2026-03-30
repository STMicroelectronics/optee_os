#
# Arm SCP/MCP Software
# Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

set(TEST_SRC mod_scmi_pfdi_monitor)
set(TEST_FILE mod_scmi_pfdi_monitor)

set(UNIT_TEST_TARGET mod_scmi_pfdi_monitor_unit_test)

set(MODULE_SRC ${PRODUCT_ROOT}/${TEST_MODULE}/src)
set(MODULE_INC ${PRODUCT_ROOT}/${TEST_MODULE}/include)

list(APPEND OTHER_MODULE_INC ${PRODUCT_ROOT}/automotive-rd/rdaspen/module/pfdi_monitor/include)
list(APPEND OTHER_MODULE_INC ${MODULE_ROOT}/scmi/include)

set(MODULE_UT_SRC ${CMAKE_CURRENT_LIST_DIR})
set(MODULE_UT_INC ${CMAKE_CURRENT_LIST_DIR})

list(APPEND MOCK_REPLACEMENTS fwk_id)
list(APPEND MOCK_REPLACEMENTS fwk_module)

include(${SCP_ROOT}/unit_test/module_common.cmake)
