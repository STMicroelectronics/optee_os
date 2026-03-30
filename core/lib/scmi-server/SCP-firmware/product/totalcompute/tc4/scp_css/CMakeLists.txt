#
# Arm SCP/MCP Software
# Copyright (c) 2023-2024, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

#
# Create the firmware target.
#

add_executable(scp-css)

set(CMAKE_TC_DIR "../../")

# SCP_PLATFORM_VARIANT options:
# - 'TC4_VARIANT_STD' for TC4 standard build
# - 'TC4_VAR_EXPERIMENT_POWER' for TC4 with power/performance/thermal additions
#   used for evaluation purpose:
#   - TRAFFIC_COP on LITTLE cores
#   - MPMM on all cores
#   - THERMAL_MANAGEMENT for the entire system, with a simplified/dummy power
#     model
# - 'TC4_VAR_MPMM' for TC4 with only the MPMM scmi-perf plugin addition

target_compile_definitions(scp-css PUBLIC -DTC4_VARIANT_STD=0)
target_compile_definitions(scp-css PUBLIC -DTC4_VAR_EXPERIMENT_POWER=1)
target_compile_definitions(scp-css PUBLIC -DTC4_VAR_MPMM=2)

if(SCP_ENABLE_PLAT_FVP)
    target_compile_definitions(scp-css PUBLIC -DPLAT_FVP=1)
endif()

if (SCP_PLATFORM_VARIANT STREQUAL "1")
    message(NOTICE "SCP_PLATFORM_VARIANT set to: EXPERIMENT_POWER (scp-css)\n")

    target_compile_definitions(scp-css
        PUBLIC -DPLATFORM_VARIANT=TC4_VAR_EXPERIMENT_POWER)

    set(SCP_ENABLE_SCMI_NOTIFICATIONS TRUE PARENT_SCOPE)

# The following list of modules will be appended in Firmware.cmake in the same
# order

    list(APPEND SCP_MODULES "traffic-cop")
    target_sources(scp-css PRIVATE "config_traffic_cop.c")

    list(APPEND SCP_MODULES "amu-mmap")
    target_sources(scp-css PRIVATE "${CMAKE_TC_DIR}/common/config_amu_mmap.c")
    target_include_directories(scp-css PUBLIC
        "${CMAKE_SOURCE_DIR}/interface/amu")

    list(APPEND SCP_MODULES "mpmm")
    target_sources(scp-css PRIVATE "config_mpmm.c")

    list(APPEND SCP_MODULES "sensor")
    target_sources(scp-css PRIVATE "config_sensor.c")

    list(APPEND SCP_MODULES "reg-sensor")
    target_sources(scp-css PRIVATE "config_reg_sensor.c")

    list(APPEND SCP_MODULES "thermal-mgmt")
    target_sources(scp-css PRIVATE "config_thermal_mgmt.c")

    list(APPEND SCP_MODULES "tc4-power-model")
    list(PREPEND SCP_MODULE_PATHS
        "${CMAKE_CURRENT_LIST_DIR}/../module/tc4_power_model")
    target_sources(scp-css PRIVATE "config_tc4_power_model.c")

elseif (SCP_PLATFORM_VARIANT STREQUAL "2")
    message(NOTICE "SCP_PLATFORM_VARIANT set to: MPMM (scp-css)\n")

    target_compile_definitions(scp-css
        PUBLIC -DPLATFORM_VARIANT=TC4_VAR_MPMM)

    set(SCP_ENABLE_SCMI_NOTIFICATIONS TRUE PARENT_SCOPE)

    list(APPEND SCP_MODULES "amu-mmap")
    target_sources(scp-css PRIVATE "${CMAKE_TC_DIR}/common/config_amu_mmap.c")
    target_include_directories(scp-css PUBLIC
        "${CMAKE_SOURCE_DIR}/interface/amu")

    list(APPEND SCP_MODULES "mpmm")
    target_sources(scp-css PRIVATE "config_mpmm.c")

else()
    message(NOTICE "SCP_PLATFORM_VARIANT set to: STD (scp-css)\n")

    target_compile_definitions(scp-css
        PUBLIC -DPLATFORM_VARIANT=TC4_VARIANT_STD)

endif()

if(SCP_ENABLE_RESOURCE_PERMISSIONS)
    list(APPEND SCP_MODULES,"resource-perms")
endif()

target_include_directories(
    scp-css PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}"
                   "${CMAKE_TC_DIR}/tc4/include"
                   "${CMAKE_TC_DIR}/common"
                   "${CMAKE_TC_DIR}/common/include")

# cmake-lint: disable=E1122

target_sources(
    scp-css
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/config_atu.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/config_tc4_bl1.c"
            "${CMAKE_TC_DIR}/common/config_pl011.c"
            "${CMAKE_TC_DIR}/common/config_scmi_power_domain.c"
            "${CMAKE_TC_DIR}/common/config_scmi_system_power.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/config_mhu3.c"
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
            "${CMAKE_CURRENT_SOURCE_DIR}/config_clock.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/config_ccsm.c")

if(SCP_ENABLE_RESOURCE_PERMISSIONS)
    target_sources(scp-css PRIVATE "config_resource_perms.c")
endif()

if(SCP_ENABLE_SCMI_PERF_FAST_CHANNELS)
    list(APPEND SCP_MODULES "fch-polled")
    target_sources(scp-css PRIVATE "config_fch_polled.c")
endif()

#
# Some of our firmware includes require CMSIS.
#

target_link_libraries(scp-css PUBLIC cmsis::core-m)

#
# We explicitly add the CMSIS include directories to our interface include
# directories. Each module target adds these include directories to their own,
# allowing them to include any firmware includes we expose.
#

target_include_directories(scp-css
    PUBLIC $<TARGET_PROPERTY:cmsis::core-m,INTERFACE_INCLUDE_DIRECTORIES>)

if(SCP_ENABLE_PLUGIN_HANDLER)
    message(NOTICE "SCP_ENABLE_PLUGIN_HANDLER enabled")
endif()

set(SCP_MODULES ${SCP_MODULES} PARENT_SCOPE)
set(SCP_MODULE_PATHS ${SCP_MODULE_PATHS} PARENT_SCOPE)
