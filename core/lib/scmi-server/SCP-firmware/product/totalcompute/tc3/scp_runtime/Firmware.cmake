#
# Arm SCP/MCP Software
# Copyright (c) 2023-2024, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

#
# Configure the build system.
#

set(SCP_FIRMWARE "scp-runtime")

set(SCP_FIRMWARE_TARGET "scp-runtime")

set(SCP_TOOLCHAIN_INIT "GNU")

set(SCP_GENERATE_FLAT_BINARY_INIT TRUE)

set(SCP_ENABLE_NOTIFICATIONS_INIT TRUE)

set(SCP_ENABLE_RESOURCE_PERMISSIONS_INIT TRUE)

set(SCP_ENABLE_IPO_INIT FALSE)

set(SCP_ENABLE_SCMI_PERF_FAST_CHANNELS_INIT FALSE)

set(SCP_ENABLE_PLUGIN_HANDLER_INIT FALSE)

set(SCP_ENABLE_OUTBAND_MSG_SUPPORT_INIT TRUE)

set(SCP_PLATFORM_VARIANT_INIT "0") #Standard variant by default

set(SCP_ARCHITECTURE "arm-m")

set(SCP_PLATFORM_VARIANT ${SCP_PLATFORM_VARIANT_INIT} CACHE STRING
    "Platform variant for the build")

list(PREPEND SCP_MODULE_PATHS "${CMAKE_CURRENT_LIST_DIR}/../../common/module/tc_system")

# The order of the modules in the following list is the order in which the
# modules are initialized, bound, started during the pre-runtime phase.
# any change in the order will cause firmware initialization errors.

list(APPEND SCP_MODULES "pl011")
list(APPEND SCP_MODULES "gtimer")
list(APPEND SCP_MODULES "timer")
list(APPEND SCP_MODULES "ppu-v1")
list(APPEND SCP_MODULES "mock-ppu")
list(APPEND SCP_MODULES "system-power")
list(APPEND SCP_MODULES "mhu3")
list(APPEND SCP_MODULES "transport")
list(APPEND SCP_MODULES "scmi")
list(APPEND SCP_MODULES "sds")
list(APPEND SCP_MODULES "system-pll")
list(APPEND SCP_MODULES "pik-clock")
list(APPEND SCP_MODULES "css-clock")
list(APPEND SCP_MODULES "clock")
list(APPEND SCP_MODULES "power-domain")
list(APPEND SCP_MODULES "scmi-power-domain")
list(APPEND SCP_MODULES "scmi-system-power")
list(APPEND SCP_MODULES "dvfs")
list(APPEND SCP_MODULES "scmi-clock")
list(APPEND SCP_MODULES "scmi-perf")
list(APPEND SCP_MODULES "mock-psu")
list(APPEND SCP_MODULES "psu")
list(APPEND SCP_MODULES "tc-system")

if (SCP_PLATFORM_VARIANT STREQUAL "1" OR SCP_PLATFORM_VARIANT STREQUAL "2")
    set(SCP_ENABLE_PLUGIN_HANDLER TRUE)
    set(SCP_ENABLE_SCMI_PERF_FAST_CHANNELS TRUE)
    set(SCP_ENABLE_SCMI_NOTIFICATIONS TRUE)

    list(APPEND SCP_MODULES "fch-polled")
    list(APPEND SCP_MODULES "traffic-cop")
    list(APPEND SCP_MODULES "amu-mmap")
    list(APPEND SCP_MODULES "mpmm")

    if (SCP_PLATFORM_VARIANT STREQUAL "1")
        list(APPEND SCP_MODULES "sensor")
        list(APPEND SCP_MODULES "reg-sensor")
        list(APPEND SCP_MODULES "thermal-mgmt")
        list(APPEND SCP_MODULES "tc3-power-model")
        list(PREPEND SCP_MODULE_PATHS
            "${CMAKE_CURRENT_LIST_DIR}/../module/tc3_power_model")
    endif()
endif()
