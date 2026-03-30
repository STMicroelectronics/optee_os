#
# Arm SCP/MCP Software
# Copyright (c) 2023-2024, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

#
# Create the firmware target.
#

add_executable(scp-runtime)

set(CMAKE_TC_DIR "../../")

# SCP_PLATFORM_VARIANT options:
# - 'TC3_VARIANT_STD' for TC3 standard build
# - 'TC3_VAR_EXPERIMENT_POWER' for TC3 with power/performance/thermal additions
#   used for evaluation purpose:
#   - TRAFFIC_COP on LITTLE cores
#   - MPMM on all cores
#   - THERMAL_MANAGEMENT for the entire system, with a simplified/dummy power
#     model
# - 'TC3_VAR_MPMM' for TC3 with only the MPMM scmi-perf plugin addition

target_compile_definitions(scp-runtime PUBLIC -DTC3_VARIANT_STD=0)
target_compile_definitions(scp-runtime PUBLIC -DTC3_VAR_EXPERIMENT_POWER=1)
target_compile_definitions(scp-runtime PUBLIC -DTC3_VAR_MPMM=2)

if (SCP_PLATFORM_VARIANT STREQUAL "1")
    message(NOTICE "SCP_PLATFORM_VARIANT set to: EXPERIMENT_POWER (scp-runtime)\n")

    target_compile_definitions(scp-runtime
        PUBLIC -DPLATFORM_VARIANT=TC3_VAR_EXPERIMENT_POWER)

# The following list of modules will be appended in Firmware.cmake in the same
# order
    target_sources(scp-runtime PRIVATE "config_traffic_cop.c")
    target_sources(scp-runtime PRIVATE "${CMAKE_TC_DIR}/common/config_amu_mmap.c")
    target_include_directories(scp-runtime PUBLIC
        "${CMAKE_SOURCE_DIR}/interface/amu")

    target_sources(scp-runtime PRIVATE "config_mpmm.c")
    target_sources(scp-runtime PRIVATE "config_sensor.c")
    target_sources(scp-runtime PRIVATE "config_reg_sensor.c")
    target_sources(scp-runtime PRIVATE "config_thermal_mgmt.c")
    target_sources(scp-runtime PRIVATE "config_tc3_power_model.c")
elseif (SCP_PLATFORM_VARIANT STREQUAL "2")
    message(NOTICE "SCP_PLATFORM_VARIANT set to: MPMM (scp-runtime)\n")

    target_compile_definitions(scp-runtime
        PUBLIC -DPLATFORM_VARIANT=TC3_VAR_MPMM)

# The following list of modules will be appended in Firmware.cmake in the same
# order
    target_sources(scp-runtime PRIVATE "config_traffic_cop.c")
    target_sources(scp-runtime PRIVATE "${CMAKE_TC_DIR}/common/config_amu_mmap.c")
    target_include_directories(scp-runtime PUBLIC
        "${CMAKE_SOURCE_DIR}/interface/amu")

    target_sources(scp-runtime PRIVATE "config_mpmm.c")
else()
    message(NOTICE "SCP_PLATFORM_VARIANT set to: STD (scp-runtime)\n")

    target_compile_definitions(scp-runtime
        PUBLIC -DPLATFORM_VARIANT=TC3_VARIANT_STD)

endif()

if(SCP_ENABLE_RESOURCE_PERMISSIONS)
    list(APPEND SCP_MODULES,"resource-perms")
endif()

target_include_directories(
    scp-runtime PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_TC_DIR}/tc3/include"
    "${CMAKE_TC_DIR}/common/include")

# cmake-lint: disable=E1122

target_sources(scp-runtime PRIVATE
            "${CMAKE_TC_DIR}/common/config_pl011.c"
            "${CMAKE_TC_DIR}/common/config_scmi_power_domain.c"
            "${CMAKE_TC_DIR}/common/config_scmi_system_power.c"
            "${CMAKE_TC_DIR}/common/config_mhu3.c"
            "${CMAKE_TC_DIR}/common/config_scmi_clock.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/config_system_power.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/config_power_domain.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/config_scmi.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/config_transport.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/config_ppu_v1.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/config_sds.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/config_scmi_perf.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/config_gtimer.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/config_timer.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/config_dvfs.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/config_psu.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/config_mock_ppu.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/config_mock_psu.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/config_system_pll.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/config_pik_clock.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/config_css_clock.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/config_clock.c")

if(SCP_ENABLE_RESOURCE_PERMISSIONS)
    target_sources(scp-runtime PRIVATE "config_resource_perms.c")
endif()

if(SCP_ENABLE_SCMI_PERF_FAST_CHANNELS)
    target_sources(scp-runtime PRIVATE "config_fch_polled.c")
endif()

#
# Some of our firmware includes require CMSIS.
#

target_link_libraries(scp-runtime PUBLIC cmsis::core-m)

#
# We explicitly add the CMSIS include directories to our interface include
# directories. Each module target adds these include directories to their own,
# allowing them to include any firmware includes we expose.
#

target_include_directories(scp-runtime
    PUBLIC $<TARGET_PROPERTY:cmsis::core-m,INTERFACE_INCLUDE_DIRECTORIES>)

if(SCP_ENABLE_PLUGIN_HANDLER)
    message(NOTICE "SCP_ENABLE_PLUGIN_HANDLER enabled")
endif()

set(SCP_MODULES ${SCP_MODULES} PARENT_SCOPE)
set(SCP_MODULE_PATHS ${SCP_MODULE_PATHS} PARENT_SCOPE)
