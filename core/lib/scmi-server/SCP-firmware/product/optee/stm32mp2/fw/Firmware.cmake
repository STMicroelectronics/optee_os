#
# Arm SCP/MCP Software
# Copyright (c) 2025, STMicroelectronics and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

#
# Configure the build system.
#

set(SCP_FIRMWARE "scmi-fw")

set(SCP_FIRMWARE_TARGET "scmi-fw")

set(SCP_TOOLCHAIN_INIT "GNU")

set(SCP_ARCHITECTURE "optee")

set(CMAKE_BUILD_TYPE "Release")

set(SCP_ENABLE_NOTIFICATIONS_INIT FALSE)

set(SCP_ENABLE_SCMI_NOTIFICATIONS_INIT FALSE)

set(SCP_ENABLE_SCMI_SENSOR_EVENTS_INIT FALSE)

set(SCP_ENABLE_FAST_CHANNELS_INIT FALSE)

set(SCP_ENABLE_SCMI_RESET_INIT TRUE)

set(SCP_ENABLE_IPO_INIT FALSE)

# The order of the modules in the following list is the order in which the
# modules are initialized, bound, started during the pre-runtime phase.
# any change in the order will cause firmware initialization errors.

if(CFG_SCPFW_MOD_OPTEE_MBX)
list(APPEND SCP_MODULES "optee-mbx")
endif(CFG_SCPFW_MOD_OPTEE_MBX)

if(CFG_SCPFW_MOD_MSG_SMT)
list(APPEND SCP_MODULES "msg-smt")
endif(CFG_SCPFW_MOD_MSG_SMT)

list(APPEND SCP_MODULES "scmi")

if(CFG_SCPFW_MOD_OPTEE_CLOCK)
list(APPEND SCP_MODULES "optee-clock")
endif(CFG_SCPFW_MOD_OPTEE_CLOCK)

if(CFG_SCPFW_MOD_CLOCK)
list(APPEND SCP_MODULES "clock")
endif(CFG_SCPFW_MOD_CLOCK)

if(CFG_SCPFW_MOD_SCMI_CLOCK)
list(APPEND SCP_MODULES "scmi-clock")
endif(CFG_SCPFW_MOD_SCMI_CLOCK)

if(CFG_SCPFW_MOD_OPTEE_RESET)
list(APPEND SCP_MODULES "optee-reset")
endif(CFG_SCPFW_MOD_OPTEE_RESET)

if(CFG_SCPFW_MOD_RESET_DOMAIN)
list(APPEND SCP_MODULES "reset-domain")
endif(CFG_SCPFW_MOD_RESET_DOMAIN)

if(CFG_SCPFW_MOD_SCMI_RESET_DOMAIN)
list(APPEND SCP_MODULES "scmi-reset-domain")
endif(CFG_SCPFW_MOD_SCMI_RESET_DOMAIN)

list(APPEND SCP_MODULES "optee-console")
