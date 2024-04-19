#
# Arm SCP/MCP Software
# Copyright (c) 2022, Linaro Limited and Contributors. All rights reserved.
# Copyright (c) 2024, STMicroelectronics and the Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

add_library(arch-tfm)

# TF-M CMake_switches
set (CFG_SCPFW_MOD_DVFS n)
set (CFG_SCPFW_MOD_CLOCK n)
set (CFG_SCPFW_MOD_MOCK_CLOCK n)
set (CFG_SCPFW_MOD_MOCK_PPU n)
set (CFG_SCPFW_MOD_MOCK_PSU n)
set (CFG_SCPFW_MOD_MSG_SMT n)
set (CFG_SCPFW_MOD_POWER_DOMAIN n)
set (CFG_SCPFW_MOD_PSU n)
set (CFG_SCPFW_MOD_REG_SENSOR n)
set (CFG_SCPFW_MOD_RESET_DOMAIN n)
set (CFG_SCPFW_MOD_SCMI n)
set (CFG_SCPFW_MOD_SCMI_APCORE n)
set (CFG_SCPFW_MOD_SCMI_CLOCK n)
set (CFG_SCPFW_MOD_SCMI_PERF n)
set (CFG_SCPFW_MOD_SCMI_POWER_DOMAIN n)
set (CFG_SCPFW_MOD_SCMI_RESET_DOMAIN n)
set (CFG_SCPFW_MOD_SCMI_SENSOR n)
set (CFG_SCPFW_MOD_SCMI_VOLTAGE_DOMAIN n)
set (CFG_SCPFW_MOD_SENSOR n)
set (CFG_SCPFW_MOD_SYSTEM_PLL n)
set (CFG_SCPFW_MOD_VOLTAGE_DOMAIN n)
set (CFG_SCPFW_MOD_TFM_SMT n)

if(NOT SCP_TFM_PLAT_DIR)
        message(FATAL_ERROR
            "Insufficient firmware metadata provided.\n"
            "For tfm arch, you must set SCP_TFM_PLAT_DIR")
endif()

if(NOT SCP_TFM_DIR)
        message(FATAL_ERROR
            "Insufficient firmware metadata provided.\n"
            "For tfm arch, you must set SCP_TFM_DIR")
endif()


target_include_directories(arch-tfm
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(arch-tfm
    PRIVATE tfm_sprt
    PUBLIC module-tfm-mbx
)

target_sources(
    arch-tfm
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/arch_interrupt.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/arch_main.c")
