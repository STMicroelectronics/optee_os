#
# Arm SCP/MCP Software
# Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

set(SCP_FIRMWARE "fvp-baser-aemv8r")
set(SCP_FIRMWARE_TARGET "fvp-baser-aemv8r")

set(SCP_TOOLCHAIN_INIT "GNU")

set(SCP_GENERATE_FLAT_BINARY_INIT TRUE)

set(SCP_ARCHITECTURE "aarch64")

set(SCP_ENABLE_DEBUGGER_INIT FALSE)

list(APPEND SCP_MODULES "armv8r-mpu")
list(APPEND SCP_MODULES "pl011")
list(APPEND SCP_MODULES "gicx00")
list(APPEND SCP_MODULES "sp805")
list(APPEND SCP_MODULES "gtimer")
list(APPEND SCP_MODULES "timer")

set(SCP_ENABLE_NOTIFICATIONS TRUE)

if(SCP_ENABLE_DEBUGGER)
    list(APPEND SCP_MODULES "debugger-cli")
    list(APPEND SCP_MODULES "integration-test")
    list(APPEND SCP_MODULES  "test-timer")
    list(PREPEND SCP_MODULE_PATHS
    "${CMAKE_CURRENT_LIST_DIR}/../module/test_timer")
endif()
