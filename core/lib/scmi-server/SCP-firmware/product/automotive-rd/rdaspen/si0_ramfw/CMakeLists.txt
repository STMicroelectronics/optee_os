#
# Arm SCP/MCP Software
# Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

add_executable(rdaspen-si0-bl2)

target_include_directories(rdaspen-si0-bl2 PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_sources(rdaspen-si0-bl2 PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/config_armv8r_mpu.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/config_cluster_control.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/config_cmn_cyprus.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/config_fmu.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/config_gicx00.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/config_mhu3.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/config_si0_platform.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/config_sid.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/config_system_info.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/config_pl011.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/config_power_domain.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/config_ppu_v1.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/config_transport.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/config_system_pll.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/config_clock.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/config_ros_clock.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/config_gtimer.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/config_timer.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/config_sds.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/config_apcontext.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/config_scmi.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/config_scmi_power_domain.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/config_scmi_system_power.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/config_system_power.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/config_ni_710ae.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/config_ssu.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/config_sbistc.c"
    )

if(SCP_ENABLE_DEBUGGER)
    target_compile_definitions(rdaspen-si0-bl2 PRIVATE BUILD_HAS_DEBUGGER)
    target_sources(rdaspen-si0-bl2 PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/config_debugger_cli.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/config_integration_test.c")
endif()

target_compile_definitions(rdaspen-si0-bl2 PUBLIC
        -DPC_CONFIGURED_CORES_COUNT=${SCP_PC_CONFIGURED_CORES_COUNT})

if(SCP_ENABLE_SCMI_PFDI_MONITOR)
    target_compile_definitions(rdaspen-si0-bl2 PUBLIC
        -DPFDI_ONLINE_TIMEOUT_US=${SCP_PFDI_ONLINE_TIMEOUT_US})
    target_sources(rdaspen-si0-bl2 PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/config_scmi_pfdi_monitor.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/config_pfdi_monitor.c"
        )
endif()
