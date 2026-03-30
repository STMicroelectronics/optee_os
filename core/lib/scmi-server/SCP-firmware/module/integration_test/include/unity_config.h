/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef UNITY_CONFIG_H
#define UNITY_CONFIG_H
#include <fwk_io.h>
#define UNITY_OUTPUT_CHAR(a) fwk_io_putch(fwk_io_stdout, a)
#define UNITY_OUTPUT_FLUSH()
#define UNITY_PRINT_EOL() \
    { \
        UNITY_OUTPUT_CHAR('\r'); \
        UNITY_OUTPUT_CHAR('\n'); \
    }

#endif
