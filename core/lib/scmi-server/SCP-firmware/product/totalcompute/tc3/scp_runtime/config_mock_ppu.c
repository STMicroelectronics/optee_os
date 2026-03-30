/*
 * Arm SCP/MCP Software
 * Copyright (c) 2023-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "scp_mmap.h"
#include "tc3_mock_ppu.h"

#include <mod_mock_ppu.h>
#include <mod_power_domain.h>

#include <fwk_module.h>

/* Need to pass an allocated memory region
 * to the mock PPU which it can then access. It
 * only accesses the first word so that is all
 * we need */
static uint32_t sys0_mock_ppu, gpucgrp_mock_ppu;

static struct fwk_element \
mock_ppu_system_element_table[MOCK_PPU_ELEMENT_IDX_COUNT+1] = {
    [MOCK_PPU_ELEMENT_IDX_SYS0] =
        {
            .name = "SYS0",
            .data = &((struct mod_mock_ppu_pd_config){
                .pd_type = MOD_PD_TYPE_SYSTEM,
                .ppu.reg_base = (uintptr_t)&sys0_mock_ppu,
            }),
        },
    [MOCK_PPU_ELEMENT_IDX_GPUCGRP] =
        {
            .name = "GPUCGRP",
            .data = &((struct mod_mock_ppu_pd_config){
                .pd_type = MOD_PD_TYPE_DEVICE,
                .ppu.reg_base = (uintptr_t)&gpucgrp_mock_ppu,
            }),
        },
    [MOCK_PPU_ELEMENT_IDX_COUNT] = {0},
};

static const struct fwk_element *mock_ppu_get_element_table(fwk_id_t module_id)
{
    return mock_ppu_system_element_table;
}
/*
 * Power module configuration data
 */
const struct fwk_module_config config_mock_ppu = {
    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(mock_ppu_get_element_table),
};
