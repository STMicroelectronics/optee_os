/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Configuration data for module 'cmn_cyprus'.
 */

#include "cmn_node_id.h"
#include "si0_mmap.h"

#include <mod_cmn_cyprus.h>

#include <fwk_id.h>
#include <fwk_macros.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>

static const unsigned int snf_table[] = {
    NODE_ID_SBSX1, /* Maps to HN-S logical node 0  */
    NODE_ID_SBSX2, /* Maps to HN-S logical node 1  */
    NODE_ID_SBSX3, /* Maps to HN-S logical node 2  */
    NODE_ID_SBSX4, /* Maps to HN-S logical node 3  */
    NODE_ID_SBSX5, /* Maps to HN-S logical node 4  */
    NODE_ID_SBSX6, /* Maps to HN-S logical node 5  */
    NODE_ID_SBSX7, /* Maps to HN-S logical node 6  */
    NODE_ID_SBSX8, /* Maps to HN-S logical node 7  */
};

static const struct mod_cmn_cyprus_mem_region_map mmap[] = {
    {
        /*
         * System cache backed region
         * Map: 0x0000_0000_0000 - 0xFFFF_FFFF_FFFF (256 TiB)
         */
        .base = UINT64_C(0x000000000000),
        .size = UINT64_C(256) * FWK_TIB,
        .type = MOD_CMN_CYPRUS_MEM_REGION_TYPE_SYSCACHE,
        .hns_pos_start = { 0, 0, 0 },
        .hns_pos_end = { MESH_SIZE_X - 1, MESH_SIZE_Y - 1, 1 },
    },
    {
        /*
         * Shared SRAM
         * Map: 0x0000_0000_0000 - 0x0000_07FF_FFFF (128 MB)
         */
        .base = UINT64_C(0x000000000000),
        .size = UINT64_C(128) * FWK_MIB,
        .type = MOD_CMN_CYPRUS_MEM_REGION_TYPE_SYSCACHE_SUB,
        .node_id = NODE_ID_SBSX0,
    },
    {
        /*
         * Peripherals
         * Map: 0x00_1000_0000 - 0x00_3FFF_FFFF (768 MB)
         */
        .base = UINT64_C(0x0010000000),
        .size = UINT64_C(768) * FWK_MIB,
        .type = MOD_CMN_CYPRUS_MEM_REGION_TYPE_IO,
        .node_id = NODE_ID_HND,
    },
    {
        /*
         * System Management Domain
         * Map: 0x00_4000_0000 - 0x00_4FFF_FFFF (256 MB)
         */
        .base = UINT64_C(0x0040000000),
        .size = UINT64_C(256) * FWK_MIB,
        .type = MOD_CMN_CYPRUS_MEM_REGION_TYPE_IO,
        .node_id = NODE_ID_HNI1,
    },
    {
        /*
         * CMN-S3-AE GPV
         * Map: 0x01_0000_0000 - 0x01_3FFF_FFFF (1 GB)
         */
        .base = UINT64_C(0x0100000000),
        .size = UINT64_C(1) * FWK_GIB,
        .type = MOD_CMN_CYPRUS_MEM_REGION_TYPE_IO,
        .node_id = NODE_ID_HND,
    },
    {
        /*
         * Cluster Utility Memory region
         * Map: 0x1_4000_0000 - 0x1_7FFF_FFFF (1 GB)
         */
        .base = UINT64_C(0x140000000),
        .size = UINT64_C(1) * FWK_GIB,
        .type = MOD_CMN_CYPRUS_MEM_REGION_TYPE_IO,
        .node_id = NODE_ID_HND,
    },
    {
        /*
         * Memory Controller Memory region
         * Map: 0x1_8000_0000 - 0x1_BFFF_FFFF (1 GB)
         */
        .base = UINT64_C(0x180000000),
        .size = UINT64_C(1) * FWK_GIB,
        .type = MOD_CMN_CYPRUS_MEM_REGION_TYPE_IO,
        .node_id = NODE_ID_HND,
    },
};

static struct mod_cmn_cyprus_config cmn_config_table[1] = {
    [0] = {
        .periphbase = SI0_CMN_BASE,
        .mesh_size_x = MESH_SIZE_X,
        .mesh_size_y = MESH_SIZE_Y,
        .mmap_table = mmap,
        .mmap_count = FWK_ARRAY_SIZE(mmap),
        .hns_cal_mode = true,
        .hnf_sam_config = {
            .snf_table = snf_table,
            .snf_count = FWK_ARRAY_SIZE(snf_table),
            .hnf_sam_mode = MOD_CMN_CYPRUS_HNF_SAM_MODE_DIRECT_MAPPING,
        },
        .rnsam_scg_config = {
            .scg_hashing_mode = MOD_CMN_CYPRUS_RNSAM_SCG_POWER_OF_TWO_HASHING,
        },
    },
};

static struct mod_cmn_cyprus_config_table cmn_config_data = {
    .chip_config_data = cmn_config_table,
    .chip_count = 1U,
    .timer_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_TIMER, 0),
};

const struct fwk_module_config config_cmn_cyprus = {
    .data = (void *)&cmn_config_data,
};
