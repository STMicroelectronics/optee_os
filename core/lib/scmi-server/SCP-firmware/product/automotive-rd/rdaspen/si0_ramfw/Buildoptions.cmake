#
# Arm SCP/MCP Software
# Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

cmake_dependent_option(
    SCP_ENABLE_SCMI_PFDI_MONITOR "Enable SCMI PFDI Monitor"
    "${SCP_ENABLE_SCMI_PFDI_MONITOR_INIT}"
    "DEFINED SCP_ENABLE_SCMI_PFDI_MONITOR_INIT"
    "${SCP_ENABLE_SCMI_PFDI_MONITOR}")
