#
# Arm SCP/MCP Software
# Copyright (c) 2024, STMicroelectronics and the Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
# Configure the build system.
#

set(SCP_FIRMWARE "scmi-fw")

set(SCP_FIRMWARE_TARGET "scmi-fw")

set(SCP_TOOLCHAIN_INIT "GNU")

set(SCP_ARCHITECTURE "tfm")

set(CMAKE_BUILD_TYPE "Release")

set(SCP_ENABLE_NOTIFICATIONS TRUE)

set(SCP_ENABLE_NOTIFICATIONS_INIT TRUE)

set(SCP_ENABLE_SCMI_NOTIFICATIONS_INIT TRUE)

set(SCP_ENABLE_SCMI_NOTIFICATIONS TRUE)

set(SCP_ENABLE_SCMI_SENSOR_EVENTS_INIT FALSE)

set(SCP_ENABLE_FAST_CHANNELS_INIT FALSE)

set(SCP_ENABLE_SCMI_RESET_INIT TRUE)

set(SCP_ENABLE_IPO_INIT FALSE)

list(PREPEND SCP_MODULE_PATHS "${CMAKE_CURRENT_LIST_DIR}/../module/tfm_regu_consumer")
list(PREPEND SCP_MODULE_PATHS "${CMAKE_CURRENT_LIST_DIR}/../module/tfm_clock")
list(PREPEND SCP_MODULE_PATHS "${CMAKE_CURRENT_LIST_DIR}/../module/tfm_mbx")
list(PREPEND SCP_MODULE_PATHS "${CMAKE_CURRENT_LIST_DIR}/../module/tfm_reset")
list(PREPEND SCP_MODULE_PATHS "${CMAKE_CURRENT_LIST_DIR}/../module/stm32_pd")
list(PREPEND SCP_MODULE_PATHS "${CMAKE_CURRENT_LIST_DIR}/../module/tfm_console")
list(PREPEND SCP_MODULE_PATHS "${CMAKE_CURRENT_LIST_DIR}/../module/tfm_smt")

get_filename_component(_scp_module_base "${CMAKE_CURRENT_LIST_DIR}/../../../../module" ABSOLUTE)
list(APPEND SCP_MODULE_EXCLUDE_PATHS
    "${_scp_module_base}/posix_mqueue"
    "${_scp_module_base}/posix_transport")


# The order of the modules in the following list is the order in which the
# modules are initialized, bound, started during the pre-runtime phase.
# any change in the order will cause firmware initialization errors.

list(APPEND SCP_MODULES "tfm-mbx")
list(APPEND SCP_MODULES "msg-smt")
list(APPEND SCP_MODULES "scmi")
list(APPEND SCP_MODULES "tfm-clock")
list(APPEND SCP_MODULES "clock")
list(APPEND SCP_MODULES "scmi-clock")
list(APPEND SCP_MODULES "tfm-regu-consumer")
list(APPEND SCP_MODULES "tfm-smt")
list(APPEND SCP_MODULES "voltage-domain")
list(APPEND SCP_MODULES "scmi-voltage-domain")
list(APPEND SCP_MODULES "tfm-reset")
list(APPEND SCP_MODULES "reset-domain")
list(APPEND SCP_MODULES "scmi-reset-domain")
if(CFG_SCPFW_MOD_POWER_DOMAIN)
list(APPEND SCP_MODULES "power-domain")
list(APPEND SCP_MODULES "stm32-pd")
list(APPEND SCP_MODULES "scmi-power-domain")
endif(CFG_SCPFW_MOD_POWER_DOMAIN)
list(APPEND SCP_MODULES "tfm-console")
if(CFG_SCPFW_MOD_SCMI_SYSTEM_POWER)
list(APPEND SCP_MODULES "scmi-system-power")
endif(CFG_SCPFW_MOD_SCMI_SYSTEM_POWER)
