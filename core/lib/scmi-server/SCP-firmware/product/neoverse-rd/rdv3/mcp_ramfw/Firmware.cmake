#
# Arm SCP/MCP Software
# Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

#
# Configure the build system.
#

set(SCP_FIRMWARE "rdv3-mcp-bl2")

set(SCP_ENABLE_NEWLIB_NANO FALSE)

set(SCP_FIRMWARE_TARGET "rdv3-mcp-bl2")

set(SCP_TOOLCHAIN_INIT "GNU")

set(SCP_GENERATE_FLAT_BINARY_INIT TRUE)

set(SCP_ENABLE_NOTIFICATIONS_INIT TRUE)

# Disable Interprocedural optimization
set(SCP_ENABLE_IPO_INIT FALSE)

set(SCP_ARCHITECTURE "arm-m")

set(SCP_ENABLE_OUTBAND_MSG_SUPPORT TRUE)

set(SCP_ENABLE_ATU_DELEGATE FALSE)
set(SCP_ENABLE_ATU_MANAGE TRUE)

set(SCP_ENABLE_SCMI_NOTIFICATIONS TRUE)

list(PREPEND SCP_MODULE_PATHS
    "${CMAKE_SOURCE_DIR}/product/neoverse-rd/module/scmi_sys_power"
    "${CMAKE_CURRENT_LIST_DIR}/../module/mcp_platform")

# The order of the modules in the following list is the order in which the
# modules are initialized, bound, started during the pre-runtime phase.
# Any change in the order will cause firmware initialization errors.

list(APPEND SCP_MODULES "armv7m-mpu")
list(APPEND SCP_MODULES "pl011")
list(APPEND SCP_MODULES "atu")
list(APPEND SCP_MODULES "clock")
list(APPEND SCP_MODULES "timer")
list(APPEND SCP_MODULES "gtimer")
list(APPEND SCP_MODULES "mhu3")
list(APPEND SCP_MODULES "transport")
list(APPEND SCP_MODULES "scmi")
list(APPEND SCP_MODULES "scmi-sys-power")
list(APPEND SCP_MODULES "mcp-platform")
