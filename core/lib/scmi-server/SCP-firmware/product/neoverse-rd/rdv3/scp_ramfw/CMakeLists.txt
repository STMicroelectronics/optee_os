#
# Arm SCP/MCP Software
# Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

# cmake-lint: disable=E1122

#
# Create the firmware target.
#
add_executable(rdv3-bl2)

# SCP_PLATFORM_VARIANT value for supported RD-V3 platform variants:
# - '0' for RD-V3 platform
# - '1' for RD-V3-Cfg1 platform
# - '2' for RD-V3-Cfg2 platform

if (SCP_PLATFORM_VARIANT STREQUAL "0")
      target_compile_definitions(rdv3-bl2
                                 PUBLIC -DPLATFORM_VARIANT=0)
elseif (SCP_PLATFORM_VARIANT STREQUAL "1")
      target_compile_definitions(rdv3-bl2
                                 PUBLIC -DPLATFORM_VARIANT=1)
elseif (SCP_PLATFORM_VARIANT STREQUAL "2")
      target_compile_definitions(rdv3-bl2
                                 PUBLIC -DPLATFORM_VARIANT=2)
else()
      message(FATAL_ERROR "Please set SCP_PLATFORM_VARIANT to \n"
                                "'0' for RD-V3\n"
                                "'1' for RD-V3-Cfg1\n"
                                "'2' for RD-V3-Cfg2\n")
endif()

target_include_directories(
    rdv3-bl2
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
           "${CMAKE_CURRENT_SOURCE_DIR}/../include"
           "${CMAKE_CURRENT_SOURCE_DIR}/../../common/include")

target_sources(
    rdv3-bl2
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/config_system_power.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/config_armv7m_mpu.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/config_atu.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/config_atu_mmio.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/config_power_domain.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/config_ppu_v1.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/config_mhu3.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/config_transport.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/config_scmi.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/config_sds.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/config_timer.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/config_gtimer.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/config_scmi_system_power.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/config_cmn_cyprus.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/config_system_pll.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/config_pik_clock.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/config_clock.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/config_apcontext.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/config_scmi_power_domain.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/config_system_info.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/config_pl011.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/config_scp_platform.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/config_sid.c")

#
# Some of our firmware includes require CMSIS.
#

target_link_libraries(rdv3-bl2 PUBLIC cmsis::core-m)

#
# We explicitly add the CMSIS include directories to our interface include
# directories. Each module target adds these include directories to their own,
# allowing them to include any firmware includes we expose.
#

target_include_directories(
    rdv3-bl2
    PUBLIC $<TARGET_PROPERTY:cmsis::core-m,INTERFACE_INCLUDE_DIRECTORIES>)
