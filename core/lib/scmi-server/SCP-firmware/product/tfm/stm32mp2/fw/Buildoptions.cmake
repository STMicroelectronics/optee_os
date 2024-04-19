#
# Arm SCP/MCP Software
# Copyright (c) 2024, STMicroelectronics and the Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

#
# Ensure TF-M build-system does not override requested C standard.
#
add_compile_options(-std=c11)
