#
# Arm SCP/MCP Software
# Copyright (c) 2023-2024, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

#
# Create the firmware target.
#

add_executable(scp-boot)

set(CMAKE_TC_DIR "../../")

target_include_directories(
    scp-boot PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_TC_DIR}/tc3/include"
    "${CMAKE_TC_DIR}/common/include")

# cmake-lint: disable=E1122

target_sources(scp-boot PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/config_atu.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/config_pl011.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/config_ppu_v1.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/config_sds.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/config_system_pll.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/config_pik_clock.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/config_css_clock.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/config_clock.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/config_gtimer.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/config_timer.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/config_tc3_bl1.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/config_transport.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/config_bootloader.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/config_mhu3.c")

#
# Some of our firmware includes require CMSIS.
#

target_link_libraries(scp-boot PUBLIC cmsis::core-m)

#
# We explicitly add the CMSIS include directories to our interface include
# directories. Each module target adds these include directories to their own,
# allowing them to include any firmware includes we expose.
#

target_include_directories(scp-boot
    PUBLIC $<TARGET_PROPERTY:cmsis::core-m,INTERFACE_INCLUDE_DIRECTORIES>)

cmake_dependent_option(
    SCP_PLATFORM_VARIANT "Choose platform software variant?"
    "${SCP_PLATFORM_VARIANT_INIT}" "DEFINED SCP_PLATFORM_VARIANT_INIT"
    "${SCP_PLATFORM_VARIANT}")
