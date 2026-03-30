#
# Arm SCP/MCP Software
# Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

set(SCP_FIRMWARE "rdaspen-si0-bl2")
set(SCP_FIRMWARE_TARGET "rdaspen-si0-bl2")

set(SCP_TOOLCHAIN_INIT "GNU")

set(SCP_GENERATE_FLAT_BINARY_INIT TRUE)

set(SCP_ARCHITECTURE "aarch64")

set(SCP_ENABLE_NOTIFICATIONS_INIT TRUE)

set(SCP_ENABLE_OUTBAND_MSG_SUPPORT TRUE)

set(SCP_ENABLE_AE_EXTENSION TRUE)

set(SCP_ENABLE_SCMI_NOTIFICATIONS TRUE)

set(SCP_ENABLE_SCMI_PFDI_MONITOR_INIT FALSE)

if (NOT DEFINED SCP_PC_CONFIGURED_CORES_COUNT)
    set(SCP_PC_CONFIGURED_CORES_COUNT 4)
endif()

if (NOT DEFINED SCP_PFDI_ONLINE_TIMEOUT_US)
    set(SCP_PFDI_ONLINE_TIMEOUT_US 100000UL)
endif()

list(PREPEND SCP_MODULE_PATHS
     "${CMAKE_CURRENT_LIST_DIR}/../module/si0_platform"
     "${CMAKE_CURRENT_LIST_DIR}/../module/ros_clock"
     "${CMAKE_CURRENT_LIST_DIR}/../module/cluster_control"
     "${CMAKE_CURRENT_LIST_DIR}/../../module/fmu"
     "${CMAKE_CURRENT_LIST_DIR}/../../module/test_fmu"
     "${CMAKE_CURRENT_LIST_DIR}/../../module/ssu"
     "${CMAKE_CURRENT_LIST_DIR}/../../module/test_ssu"
     "${CMAKE_CURRENT_LIST_DIR}/../../module/sbistc"
     "${CMAKE_CURRENT_LIST_DIR}/../../module/test_sbistc"
     "${CMAKE_CURRENT_LIST_DIR}/../module/pfdi_monitor"
     "${CMAKE_CURRENT_LIST_DIR}/../module/scmi_pfdi_monitor")

list(APPEND SCP_MODULES
    "armv8r-mpu"
    "pl011"
    "gicx00"
    "ni-710ae"
    "system-pll"
    "ros-clock"
    "clock"
    "gtimer"
    "timer"
    "sid"
    "system-info"
    "pcid"
    "mhu3"
    "transport"
    "ppu-v1"
    "system-power"
    "power-domain"
    "cmn-cyprus"
    "fmu"
    "sbistc"
    "apcontext"
    "scmi"
    "sds"
    "scmi-power-domain"
    "scmi-system-power"
    "ssu"
    "cluster-control"
    "si0-platform"
)

if(SCP_ENABLE_DEBUGGER)
    list(APPEND SCP_MODULES "debugger-cli"
        "integration-test"
        "test-fmu"
        "test-ssu"
        "test-sbistc")
endif()

if(SCP_ENABLE_SCMI_PFDI_MONITOR)
    list(APPEND SCP_MODULES
        "pfdi-monitor"
        "scmi-pfdi-monitor")
endif()
