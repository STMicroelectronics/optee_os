/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Configuration data for module 'noc_s3'.
 */

#include <mod_noc_s3.h>

#include <fwk_element.h>
#include <fwk_module.h>

struct fwk_module_config config_noc_s3 = {
    .elements = FWK_MODULE_STATIC_ELEMENTS({ { 0 } })
};
