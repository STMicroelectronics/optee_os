/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Configuration data for module 'ssu'.
 */

#ifndef CONFIG_SSU_H
#define CONFIG_SSU_H

#include "fwk_module_idx.h"
#include "si0_ssu.h"

#include <mod_ssu.h>

#include <fwk_element.h>
#include <fwk_id.h>
#include <fwk_macros.h>
#include <fwk_module.h>

static struct fwk_element
    ssu_element_desc_table[] = {
            [CONFIG_SSU_ELEMENT_IDX] = {
                .name = "SSU",
                .data = &((struct mod_ssu_device_config) {
                    .reg_base = SSU_REG_PTR,
                }),
            },
            /* Termination description */
            [CONFIG_SSU_ELEMENT_IDX_COUNT] = { 0 },
};

static const struct fwk_element *ssu_get_element_table(fwk_id_t module_id)
{
    return ssu_element_desc_table;
}

const struct fwk_module_config config_ssu = {
    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(ssu_get_element_table),
};

#endif /* CONFIG_SSU_H */
