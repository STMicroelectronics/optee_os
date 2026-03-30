#
# Arm SCP/MCP Software
# Copyright (c) 2022-2024, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

include_guard()

include(SCPPreprocessSource)

# .rst:
#
# .. command:: scp_module_trace
#
# Checks if SCP_LOG_ENABLE_MOD_<module name>` is set and enables `FWK_LOG_LOCAL`
#
# .. cmake:: scp_module_log(<module_name>)
#
macro(scp_module_log_local module_name)

    # Creates a module name tag moving to upper case and replacing `-` by `_`.
    string(TOUPPER ${module_name} SCP_MODULE_TAG)
    string(REPLACE "-" "_" SCP_MODULE_TAG ${SCP_MODULE_TAG})
    set(MOD_LOG_ENABLE_FLAG "SCP_LOG_LOCAL_ENABLE_MOD_${SCP_MODULE_TAG}")

    # Checks if the enable flag is set.
    if(${MOD_LOG_ENABLE_FLAG})
        add_compile_definitions(PUBLIC FWK_LOG_LOCAL_ENABLE)
    endif()
endmacro()
