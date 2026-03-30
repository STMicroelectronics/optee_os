#
# Arm SCP/MCP Software
# Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

set(SCP_FIRMWARE "posix-host")
set(SCP_FIRMWARE_TARGET "posix-host")

set(SCP_ARCHITECTURE "posix")

list(APPEND SCP_MODULES "stdio")
list(APPEND SCP_MODULES "scmi")
list(APPEND SCP_MODULES "posix-transport")
list(APPEND SCP_MODULES "posix-mqueue")
