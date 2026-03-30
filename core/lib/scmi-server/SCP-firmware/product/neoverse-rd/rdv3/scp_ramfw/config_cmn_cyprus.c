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
#include "platform_core.h"
#include "scp_css_mmap.h"
#include "scp_exp_mmap.h"

#include <mod_cmn_cyprus.h>

#include <fwk_id.h>
#include <fwk_macros.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if (PLATFORM_VARIANT == 0)
#    define MMAP_TABLE_COUNT 18
#    define SNF_TABLE_COUNT  32
#elif (PLATFORM_VARIANT == 1)
#    define MMAP_TABLE_COUNT 14
#    define SNF_TABLE_COUNT  8
#elif (PLATFORM_VARIANT == 2)
#    define MMAP_TABLE_COUNT 14
#    define SNF_TABLE_COUNT  32
#endif

#if (PLATFORM_VARIANT == 2)
static const unsigned int snf_table[SNF_TABLE_COUNT] = {
    MEM_CNTRL0_ID, /* Maps to HN-S logical node 0  */
    MEM_CNTRL0_ID, /* Maps to HN-S logical node 1  */
    MEM_CNTRL0_ID, /* Maps to HN-S logical node 2  */
    MEM_CNTRL0_ID, /* Maps to HN-S logical node 3  */
    MEM_CNTRL0_ID, /* Maps to HN-S logical node 4  */
    MEM_CNTRL0_ID, /* Maps to HN-S logical node 5  */
    MEM_CNTRL0_ID, /* Maps to HN-S logical node 6  */
    MEM_CNTRL0_ID, /* Maps to HN-S logical node 7  */
    MEM_CNTRL1_ID, /* Maps to HN-S logical node 8  */
    MEM_CNTRL1_ID, /* Maps to HN-S logical node 9  */
    MEM_CNTRL1_ID, /* Maps to HN-S logical node 10  */
    MEM_CNTRL1_ID, /* Maps to HN-S logical node 11  */
    MEM_CNTRL1_ID, /* Maps to HN-S logical node 12  */
    MEM_CNTRL1_ID, /* Maps to HN-S logical node 13  */
    MEM_CNTRL1_ID, /* Maps to HN-S logical node 14  */
    MEM_CNTRL1_ID, /* Maps to HN-S logical node 15  */
    MEM_CNTRL2_ID, /* Maps to HN-S logical node 16  */
    MEM_CNTRL2_ID, /* Maps to HN-S logical node 17  */
    MEM_CNTRL2_ID, /* Maps to HN-S logical node 18  */
    MEM_CNTRL2_ID, /* Maps to HN-S logical node 19  */
    MEM_CNTRL2_ID, /* Maps to HN-S logical node 20  */
    MEM_CNTRL2_ID, /* Maps to HN-S logical node 21  */
    MEM_CNTRL2_ID, /* Maps to HN-S logical node 22  */
    MEM_CNTRL2_ID, /* Maps to HN-S logical node 23  */
    MEM_CNTRL3_ID, /* Maps to HN-S logical node 24  */
    MEM_CNTRL3_ID, /* Maps to HN-S logical node 25  */
    MEM_CNTRL3_ID, /* Maps to HN-S logical node 26  */
    MEM_CNTRL3_ID, /* Maps to HN-S logical node 27  */
    MEM_CNTRL3_ID, /* Maps to HN-S logical node 28  */
    MEM_CNTRL3_ID, /* Maps to HN-S logical node 29  */
    MEM_CNTRL3_ID, /* Maps to HN-S logical node 30  */
    MEM_CNTRL3_ID, /* Maps to HN-S logical node 31  */
};

enum rdv3cfg2_cmn_cyprus_ccg_port {
    CCG_PORT_0,
    CCG_PORT_1,
    CCG_PORT_2,
    CCG_PORT_3,
    CCG_PORT_4,
    CCG_PORT_5,
    CCG_PORT_6,
    CCG_PORT_7,
    CCG_PORT_8,
    CCG_PORT_9,
    CCG_PER_CHIP,
};

#else
static const unsigned int snf_table[SNF_TABLE_COUNT] = {
    MEM_CNTRL0_ID, /* Maps to HN-F logical node 0  */
    MEM_CNTRL0_ID, /* Maps to HN-F logical node 1  */
    MEM_CNTRL0_ID, /* Maps to HN-F logical node 2  */
    MEM_CNTRL0_ID, /* Maps to HN-F logical node 3  */
    MEM_CNTRL1_ID, /* Maps to HN-F logical node 4  */
    MEM_CNTRL1_ID, /* Maps to HN-F logical node 5  */
    MEM_CNTRL1_ID, /* Maps to HN-F logical node 6  */
    MEM_CNTRL1_ID, /* Maps to HN-F logical node 7  */
#    if (PLATFORM_VARIANT == 0)
    MEM_CNTRL2_ID, /* Maps to HN-F logical node 8  */
    MEM_CNTRL2_ID, /* Maps to HN-F logical node 9  */
    MEM_CNTRL2_ID, /* Maps to HN-F logical node 10  */
    MEM_CNTRL2_ID, /* Maps to HN-F logical node 11  */
    MEM_CNTRL3_ID, /* Maps to HN-F logical node 12  */
    MEM_CNTRL3_ID, /* Maps to HN-F logical node 13  */
    MEM_CNTRL3_ID, /* Maps to HN-F logical node 14  */
    MEM_CNTRL3_ID, /* Maps to HN-F logical node 15  */
    MEM_CNTRL4_ID, /* Maps to HN-F logical node 16  */
    MEM_CNTRL4_ID, /* Maps to HN-F logical node 17  */
    MEM_CNTRL4_ID, /* Maps to HN-F logical node 18  */
    MEM_CNTRL4_ID, /* Maps to HN-F logical node 19  */
    MEM_CNTRL5_ID, /* Maps to HN-F logical node 20  */
    MEM_CNTRL5_ID, /* Maps to HN-F logical node 21  */
    MEM_CNTRL5_ID, /* Maps to HN-F logical node 22  */
    MEM_CNTRL5_ID, /* Maps to HN-F logical node 23  */
    MEM_CNTRL6_ID, /* Maps to HN-F logical node 24  */
    MEM_CNTRL6_ID, /* Maps to HN-F logical node 25  */
    MEM_CNTRL6_ID, /* Maps to HN-F logical node 26  */
    MEM_CNTRL6_ID, /* Maps to HN-F logical node 27  */
    MEM_CNTRL7_ID, /* Maps to HN-F logical node 28  */
    MEM_CNTRL7_ID, /* Maps to HN-F logical node 29  */
    MEM_CNTRL7_ID, /* Maps to HN-F logical node 30  */
    MEM_CNTRL7_ID, /* Maps to HN-F logical node 31  */
#    endif
};
#endif

static const struct mod_cmn_cyprus_mem_region_map mmap[MMAP_TABLE_COUNT] = {
    {
        /*
         * System cache backed region
         * Map: 0x0000_0000_0000 - 0x03FF_FFFF_FFFF (4 TiB)
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
        .node_id = NODE_ID_SBSX,
    },
    {
        /*
         * Boot Flash
         * Map: 0x00_0800_0000 - 0x00_0FFF_FFFF (128 MB)
         */
        .base = UINT64_C(0x0008000000),
        .size = UINT64_C(128) * FWK_MIB,
        .type = MOD_CMN_CYPRUS_MEM_REGION_TYPE_IO,
#if (PLATFORM_VARIANT == 0 || PLATFORM_VARIANT == 2)
        .node_id = NODE_ID_HNT1,
#else
        .node_id = NODE_ID_HNT0,
#endif
    },
    {
        /*
         * Peripherals
         * Map: 0x00_1000_0000 - 0x00_2EFF_FFFF (496 MB)
         */
        .base = UINT64_C(0x0010000000),
        .size = UINT64_C(496) * FWK_MIB,
        .type = MOD_CMN_CYPRUS_MEM_REGION_TYPE_IO,
        .node_id = NODE_ID_HND,
    },
    {
        /*
         * Shared SRAM
         * Map: 0x00_2F00_0000 - 0x00_2F3F_FFFF (4 MB)
         */
        .base = UINT64_C(0x002F000000),
        .size = UINT64_C(4) * FWK_MIB,
        .type = MOD_CMN_CYPRUS_MEM_REGION_TYPE_SYSCACHE_SUB,
        .node_id = NODE_ID_SBSX,
    },
    {
        /*
         * Peripherals
         * Map: 0x00_2F40_0000 - 0x00_5FFF_0000 (779 MB)
         */
        .base = UINT64_C(0x002F400000),
        .size = UINT64_C(779) * FWK_MIB,
        .type = MOD_CMN_CYPRUS_MEM_REGION_TYPE_IO,
        .node_id = NODE_ID_HND,
    },
    {
        /*
         * HNI for RGIC2LGIC routing from CMN to GIC
         * Map: 0x00_5FFF_0000 - 0x00_5FFF_FFFF (64 KiB)
         */
        .base = UINT64_C(0x005FFF0000),
        .size = UINT64_C(64) * FWK_KIB,
        .type = MOD_CMN_CYPRUS_MEM_REGION_TYPE_IO,
        .node_id = NODE_ID_HNI_GIC,
    },
    {
        /*
         * CMN_CYPRUS GPV
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
         * Map: 0x2_0000_0000 - 0x2_3FFF_FFFF (1 GB)
         */
        .base = UINT64_C(0x200000000),
        .size = UINT64_C(1) * FWK_GIB,
        .type = MOD_CMN_CYPRUS_MEM_REGION_TYPE_IO,
        .node_id = NODE_ID_HND,
    },
    {
        /*
         * Peripherals - Memory Controller
         * Map: 0x2_4000_0000 - 0x2_4FFF_FFFF (256 MB)
         */
        .base = UINT64_C(0x240000000),
        .size = UINT64_C(256) * FWK_MIB,
        .type = MOD_CMN_CYPRUS_MEM_REGION_TYPE_IO,
        .node_id = NODE_ID_HND,
    },
    {
        /*
         * Peripherals, NCI GPV Memory Map 0
         * Map: 0x02_8000_0000 - 0x02_87FF_FFFF (128 MB)
         */
        .base = UINT64_C(0x0280000000),
        .size = UINT64_C(128) * FWK_MIB,
        .type = MOD_CMN_CYPRUS_MEM_REGION_TYPE_IO,
        .node_id = IOVB_NODE_ID0,
    },
#if (PLATFORM_VARIANT == 0)
    {
        /*
         * Peripherals, NCI GPV Memory Map 1
         * Map: 0x02_8800_0000 - 0x02_8FFF_FFFF (128 MB)
         */
        .base = UINT64_C(0x0288000000),
        .size = UINT64_C(128) * FWK_MIB,
        .type = MOD_CMN_CYPRUS_MEM_REGION_TYPE_IO,
        .node_id = IOVB_NODE_ID1,
    },
    {
        /*
         * Peripherals, NCI GPV Memory Map 2
         * Map: 0x02_9000_0000 - 0x02_97FF_FFFF (128 MB)
         */
        .base = UINT64_C(0x0290000000),
        .size = UINT64_C(128) * FWK_MIB,
        .type = MOD_CMN_CYPRUS_MEM_REGION_TYPE_IO,
        .node_id = IOVB_NODE_ID2,
    },
    {
        /*
         * Peripherals, NCI GPV Memory Map 3
         * Map: 0x02_9800_0000 - 0x02_9FFF_FFFF (128 MB)
         */
        .base = UINT64_C(0x0298000000),
        .size = UINT64_C(128) * FWK_MIB,
        .type = MOD_CMN_CYPRUS_MEM_REGION_TYPE_IO,
        .node_id = IOVB_NODE_ID3,
    },
    {
        /*
         * Peripherals, NCI GPV Memory Map 4
         * Map: 0x02_A000_0000 - 0x02_A7FF_FFFF (128 MB)
         */
        .base = UINT64_C(0x02A0000000),
        .size = UINT64_C(128) * FWK_MIB,
        .type = MOD_CMN_CYPRUS_MEM_REGION_TYPE_IO,
        .node_id = IOVB_NODE_ID4,
    },
#endif
    {
        /*
         * GPC_SMMU region
         * Map: 0x03_0000_0000 - 0x03_07FF_FFFF (128 MB)
         */
        .base = UINT64_C(0x300000000),
        .size = UINT64_C(128) * FWK_MIB,
        .type = MOD_CMN_CYPRUS_MEM_REGION_TYPE_IO,
        .node_id = NODE_ID_HND,
    },
    {
        /*
         * Non Secure NOR Flash 0/1
         * Map: 0x06_0000_0000 - 0x06_07FF_FFFF (128 MB)
         */
        .base = UINT64_C(0x0600000000),
        .size = UINT64_C(128) * FWK_MIB,
        .type = MOD_CMN_CYPRUS_MEM_REGION_TYPE_IO,
        .node_id = NODE_ID_HND,
    },
    {
        /*
         * Ethernet Controller PL91x
         * Map: 0x06_0C00_0000 - 0x06_0FFF_FFFF (64 MB)
         */
        .base = UINT64_C(0x060C000000),
        .size = UINT64_C(64) * FWK_MIB,
        .type = MOD_CMN_CYPRUS_MEM_REGION_TYPE_IO,
        .node_id = NODE_ID_HND,
    },
};

#if (PLATFORM_VARIANT == 2)
//clang-format off
/* Multichip related configuration data */
/*
 * Cross chip CCG connections between the chips:
 *
 *  +-----------------------------+             +----------------------------+
 *  |                             |             |                            |
 *  |                         CCG6+-------------+CCG6                        |
 *  |                             |             |                        CCG4|
 *  |CCG4                     CCG7+-------------+CCG7                        |
 *  |        CHIP 0               |             |          CHIP 2            |
 *  |                         CCG8+----+  +-----+CCG8                    CCG5|
 *  |CCG                          |    |  |     |                            |
 *  |                         CCG9+-+  |  |  +--+CCG9                        |
 *  |                             | |  |  |  |  |                            |
 *  |  CCG0  CCG1  CCG2    CCG3   | |  |  |  |  |   CCG0  CCG1  CCG2  CCG3   |
 *  +---+-----+------+------+-----+ |  |  |  |  +-----+-----+-----+-----+----+
 *      |     |      |      |       |  |  |  |        |     |     |     |
 *      |     |      |      |       |  |  |  |        |     |     |     |
 *  +---+-----+------+------+-----+ |  |  |  |  +-----+-----+-----+-----+----+
 *  |  CCG3  CCG2   CCG1   CCG0   | |  |  |  |  |    CCG3 CCG2  CCG1   CCG0  |
 *  |                             | |  |  |  |  |                            |
 *  |                         CCG4+-|--|--+  |  |                            |
 *  |                             | |  |     |  |                            |
 *  |                         CCG5+-|--|-----+  |                            |
 *                                | |  |        |                            |
 *  |CCG8                         | |  +--------+CCG4                    CCG8|
 *  |        CHIP 1               | |           |          CHIP 3            |
 *  |                             | +-----------+CCG5                        |
 *  |CCG9                         |             |                        CCG9|
 *  |                         CCG6+-------------+CCG6                        |
 *  |                             |             |                            |
 *  |                         CCG7+-------------+CCG7                        |
 *  |                             |             |                            |
 *  +-----------------------------+             +----------------------------+
 */
//clang-format off

#    define CML_CPA_MODE true
#    define CCG_PER_CPAG 2

#    define CCG_LDID_LIST(ccg_port_x, ccg_port_y) \
        ((unsigned int[CCG_PER_CPAG]){ ccg_port_x, ccg_port_y })

#    define GET_UNIQUE_RAID(chip_id, ccg_port) \
        ((CCG_PER_CHIP * chip_id) + ccg_port)

#    define CCG_HAID_LIST(chip_id, ccg_port_x, ccg_port_y) \
        ((unsigned int[CCG_PER_CPAG]){ GET_UNIQUE_RAID(chip_id, ccg_port_x), \
                                       GET_UNIQUE_RAID(chip_id, ccg_port_y) })

#    define REMOTE_HASHED_REGION( \
        region1_start, \
        region1_size, \
        region2_start, \
        region2_size, \
        start_pos, \
        end_pos) \
        { \
            .base = region1_start, .size = region1_size, \
            .sec_region_base = region2_start, .sec_region_size = region2_size, \
            .type = MOD_CMN_CYPRUS_MEM_REGION_TYPE_REMOTE_HASHED, \
            .hns_pos_start = start_pos, .hns_pos_end = end_pos, \
        }

#    define MESH_START \
        { \
            0, 0, 0 \
        }

#    define MESH_END \
        { \
            MESH_SIZE_X - 1, MESH_SIZE_Y - 1, 1 \
        }

#    define MULTICHIP_ADDR_OFFSET      0x1000000000 // 64GB (1ULL << 36)
#    define REMOTE_CHIP_ADDR(chip_idx) (MULTICHIP_ADDR_OFFSET * chip_idx)

#    define DRAM2_BASE_ADDR       0x8080000000
#    define DRAM2_REMOTE_ADDR_BIT 44
#    define DRAM2_SIZE            ((UINT64_C(1) << DRAM2_REMOTE_ADDR_BIT))
#    define DRAM2_CHIP_ADDR(chip_idx) \
        (DRAM2_BASE_ADDR + (DRAM2_SIZE * (chip_idx)))

static const struct mod_cmn_cyprus_cml_config \
                      chip0_cml_config[PLATFORM_CHIP_COUNT - 1] = {
    {
        /* Chip 0 to Chip 1 */
        .ccg_ldid = CCG_LDID_LIST(CCG_PORT_0, CCG_PORT_1),
        .haid = CCG_HAID_LIST(PLATFORM_CHIP_0, CCG_PORT_0, CCG_PORT_1),
        .remote_mmap_table = {
            {
               .region_mmap = REMOTE_HASHED_REGION(REMOTE_CHIP_ADDR(1), \
                                MULTICHIP_ADDR_OFFSET, DRAM2_CHIP_ADDR(1), \
                                DRAM2_SIZE, MESH_START, MESH_END),
               .target_haid = CCG_HAID_LIST(PLATFORM_CHIP_1, CCG_PORT_3, CCG_PORT_2),
            },
        },
        .remote_chip_id = PLATFORM_CHIP_1,
        .enable_smp_mode = true,
        .enable_direct_connect_mode = true,
        .enable_cpa_mode = CML_CPA_MODE,
        .cpag_config = {
            .cpag_id = 0,
            .ccg_count = CCG_PER_CPAG,
        },
    },
    {
        /* Chip 0 to Chip 2 */
        .ccg_ldid = CCG_LDID_LIST(CCG_PORT_6, CCG_PORT_7),
        .haid = CCG_HAID_LIST(PLATFORM_CHIP_0, CCG_PORT_6, CCG_PORT_7),
        .remote_mmap_table = {
            {
               .region_mmap = REMOTE_HASHED_REGION(REMOTE_CHIP_ADDR(2), \
                                MULTICHIP_ADDR_OFFSET, DRAM2_CHIP_ADDR(2), \
                                DRAM2_SIZE, MESH_START, MESH_END),
               .target_haid = CCG_HAID_LIST(PLATFORM_CHIP_2, CCG_PORT_6, CCG_PORT_7),
            },
        },
        .remote_chip_id = PLATFORM_CHIP_2,
        .enable_smp_mode = true,
        .enable_direct_connect_mode = true,
        .enable_cpa_mode = CML_CPA_MODE,
        .cpag_config = {
            .cpag_id = 1,
            .ccg_count = CCG_PER_CPAG,
        },
    },
    {
        /* Chip 0 to Chip 3 */
        .ccg_ldid = CCG_LDID_LIST(CCG_PORT_8, CCG_PORT_9),
        .haid = CCG_HAID_LIST(PLATFORM_CHIP_0, CCG_PORT_8, CCG_PORT_9),
        .remote_mmap_table = {
            {
               .region_mmap = REMOTE_HASHED_REGION(REMOTE_CHIP_ADDR(3), \
                                MULTICHIP_ADDR_OFFSET, DRAM2_CHIP_ADDR(3), \
                                DRAM2_SIZE, MESH_START, MESH_END),
               .target_haid = CCG_HAID_LIST(PLATFORM_CHIP_3, CCG_PORT_4, CCG_PORT_5),
            },
        },
        .remote_chip_id = PLATFORM_CHIP_3,
        .enable_smp_mode = true,
        .enable_direct_connect_mode = true,
        .enable_cpa_mode = CML_CPA_MODE,
        .cpag_config = {
            .cpag_id = 2,
            .ccg_count = CCG_PER_CPAG,
        },
    },
};

static const struct mod_cmn_cyprus_cml_config \
                      chip1_cml_config[PLATFORM_CHIP_COUNT - 1] = {
    {
        /* Chip 1 to Chip 0 */
        .ccg_ldid = CCG_LDID_LIST(CCG_PORT_3, CCG_PORT_2),
        .haid = CCG_HAID_LIST(PLATFORM_CHIP_1, CCG_PORT_3, CCG_PORT_2),
        .remote_mmap_table = {
            {
               .region_mmap = REMOTE_HASHED_REGION(REMOTE_CHIP_ADDR(0), \
                                MULTICHIP_ADDR_OFFSET, DRAM2_CHIP_ADDR(0), \
                                DRAM2_SIZE, MESH_START, MESH_END),
               .target_haid = CCG_HAID_LIST(PLATFORM_CHIP_0, CCG_PORT_0, CCG_PORT_1),
            },
        },
        .remote_chip_id = PLATFORM_CHIP_0,
        .enable_smp_mode = true,
        .enable_direct_connect_mode = true,
        .enable_cpa_mode = CML_CPA_MODE,
        .cpag_config = {
            .cpag_id = 0,
            .ccg_count = CCG_PER_CPAG,
        },
    },
    {
        /* Chip 1 to Chip 2 */
        .ccg_ldid = CCG_LDID_LIST(CCG_PORT_4, CCG_PORT_5),
        .haid = CCG_HAID_LIST(PLATFORM_CHIP_1, CCG_PORT_4, CCG_PORT_5),
        .remote_mmap_table = {
            {
               .region_mmap = REMOTE_HASHED_REGION(REMOTE_CHIP_ADDR(2), \
                                MULTICHIP_ADDR_OFFSET, DRAM2_CHIP_ADDR(2), \
                                DRAM2_SIZE, MESH_START, MESH_END),
               .target_haid = CCG_HAID_LIST(PLATFORM_CHIP_2, CCG_PORT_8, CCG_PORT_9),
            },
        },
        .remote_chip_id = PLATFORM_CHIP_2,
        .enable_smp_mode = true,
        .enable_direct_connect_mode = true,
        .enable_cpa_mode = CML_CPA_MODE,
        .cpag_config = {
            .cpag_id = 1,
            .ccg_count = CCG_PER_CPAG,
        },
    },
    {
        /* Chip 1 to Chip 3 */
        .ccg_ldid = CCG_LDID_LIST(CCG_PORT_6, CCG_PORT_7),
        .haid = CCG_HAID_LIST(PLATFORM_CHIP_1, CCG_PORT_6, CCG_PORT_7),
        .remote_mmap_table = {
            {
               .region_mmap = REMOTE_HASHED_REGION(REMOTE_CHIP_ADDR(3), \
                                MULTICHIP_ADDR_OFFSET, DRAM2_CHIP_ADDR(3), \
                                DRAM2_SIZE, MESH_START, MESH_END),
               .target_haid = CCG_HAID_LIST(PLATFORM_CHIP_3, CCG_PORT_6, CCG_PORT_7),
            },
        },
        .remote_chip_id = PLATFORM_CHIP_3,
        .enable_smp_mode = true,
        .enable_direct_connect_mode = true,
        .enable_cpa_mode = CML_CPA_MODE,
        .cpag_config = {
            .cpag_id = 2,
            .ccg_count = CCG_PER_CPAG,
        },
    },
};

static const struct mod_cmn_cyprus_cml_config \
                      chip2_cml_config[PLATFORM_CHIP_COUNT - 1] = {
    {
        /* Chip 2 to Chip 0 */
        .ccg_ldid = CCG_LDID_LIST(CCG_PORT_6, CCG_PORT_7),
        .haid = CCG_HAID_LIST(PLATFORM_CHIP_2, CCG_PORT_6, CCG_PORT_7),
        .remote_mmap_table = {
            {
               .region_mmap = REMOTE_HASHED_REGION(REMOTE_CHIP_ADDR(0), \
                                MULTICHIP_ADDR_OFFSET, DRAM2_CHIP_ADDR(0), \
                                DRAM2_SIZE, MESH_START, MESH_END),
               .target_haid = CCG_HAID_LIST(PLATFORM_CHIP_0, CCG_PORT_6, CCG_PORT_7),
            },
        },
        .remote_chip_id = PLATFORM_CHIP_0,
        .enable_smp_mode = true,
        .enable_direct_connect_mode = true,
        .enable_cpa_mode = CML_CPA_MODE,
        .cpag_config = {
            .cpag_id = 0,
            .ccg_count = CCG_PER_CPAG,
        },
    },
    {
        /* Chip 2 to Chip 1 */
        .ccg_ldid = CCG_LDID_LIST(CCG_PORT_8, CCG_PORT_9),
        .haid = CCG_HAID_LIST(PLATFORM_CHIP_2, CCG_PORT_8, CCG_PORT_9),
        .remote_mmap_table = {
            {
               .region_mmap = REMOTE_HASHED_REGION(REMOTE_CHIP_ADDR(1), \
                                MULTICHIP_ADDR_OFFSET, DRAM2_CHIP_ADDR(1), \
                                DRAM2_SIZE, MESH_START, MESH_END),
               .target_haid = CCG_HAID_LIST(PLATFORM_CHIP_1, CCG_PORT_4, CCG_PORT_5),
            },
        },
        .remote_chip_id = PLATFORM_CHIP_1,
        .enable_smp_mode = true,
        .enable_direct_connect_mode = true,
        .enable_cpa_mode = CML_CPA_MODE,
        .cpag_config = {
            .cpag_id = 1,
            .ccg_count = CCG_PER_CPAG,
        },
    },
    {
        /* Chip 2 to Chip 3 */
        .ccg_ldid = CCG_LDID_LIST(CCG_PORT_0, CCG_PORT_1),
        .haid = CCG_HAID_LIST(PLATFORM_CHIP_2, CCG_PORT_0, CCG_PORT_1),
        .remote_mmap_table = {
            {
               .region_mmap = REMOTE_HASHED_REGION(REMOTE_CHIP_ADDR(3), \
                                MULTICHIP_ADDR_OFFSET, DRAM2_CHIP_ADDR(3), \
                                DRAM2_SIZE, MESH_START, MESH_END),
               .target_haid = CCG_HAID_LIST(PLATFORM_CHIP_3, CCG_PORT_3, CCG_PORT_2),
            },
        },
        .remote_chip_id = PLATFORM_CHIP_3,
        .enable_smp_mode = true,
        .enable_direct_connect_mode = true,
        .enable_cpa_mode = CML_CPA_MODE,
        .cpag_config = {
            .cpag_id = 2,
            .ccg_count = CCG_PER_CPAG,
        },
    },
};

static const struct mod_cmn_cyprus_cml_config \
                      chip3_cml_config[PLATFORM_CHIP_COUNT - 1] = {
    {
        /* Chip 3 to Chip 0 */
        .ccg_ldid = CCG_LDID_LIST(CCG_PORT_4, CCG_PORT_5),
        .haid = CCG_HAID_LIST(PLATFORM_CHIP_3, CCG_PORT_4, CCG_PORT_5),
        .remote_mmap_table = {
            {
               .region_mmap = REMOTE_HASHED_REGION(REMOTE_CHIP_ADDR(0), \
                                MULTICHIP_ADDR_OFFSET, DRAM2_CHIP_ADDR(0), \
                                DRAM2_SIZE, MESH_START, MESH_END),
               .target_haid = CCG_HAID_LIST(PLATFORM_CHIP_0, CCG_PORT_8, CCG_PORT_9),
            },
        },
        .remote_chip_id = PLATFORM_CHIP_0,
        .enable_smp_mode = true,
        .enable_direct_connect_mode = true,
        .enable_cpa_mode = CML_CPA_MODE,
        .cpag_config = {
            .cpag_id = 0,
            .ccg_count = CCG_PER_CPAG,
        },
    },
    {
        /* Chip 3 to Chip 1 */
        .ccg_ldid = CCG_LDID_LIST(CCG_PORT_6, CCG_PORT_7),
        .haid = CCG_HAID_LIST(PLATFORM_CHIP_3, CCG_PORT_6, CCG_PORT_7),
        .remote_mmap_table = {
            {
               .region_mmap = REMOTE_HASHED_REGION(REMOTE_CHIP_ADDR(1), \
                                MULTICHIP_ADDR_OFFSET, DRAM2_CHIP_ADDR(1), \
                                DRAM2_SIZE, MESH_START, MESH_END),
               .target_haid = CCG_HAID_LIST(PLATFORM_CHIP_1, CCG_PORT_6, CCG_PORT_7),
            },
        },
        .remote_chip_id = PLATFORM_CHIP_1,
        .enable_smp_mode = true,
        .enable_direct_connect_mode = true,
        .enable_cpa_mode = CML_CPA_MODE,
        .cpag_config = {
            .cpag_id = 1,
            .ccg_count = CCG_PER_CPAG,
        },
    },
    {
        /* Chip 3 to Chip 2 */
        .ccg_ldid = CCG_LDID_LIST(CCG_PORT_3, CCG_PORT_2),
        .haid = CCG_HAID_LIST(PLATFORM_CHIP_3, CCG_PORT_3, CCG_PORT_2),
        .remote_mmap_table = {
            {
               .region_mmap = REMOTE_HASHED_REGION(REMOTE_CHIP_ADDR(2), \
                                MULTICHIP_ADDR_OFFSET, DRAM2_CHIP_ADDR(2), \
                                DRAM2_SIZE, MESH_START, MESH_END),
               .target_haid = CCG_HAID_LIST(PLATFORM_CHIP_2, CCG_PORT_0, CCG_PORT_1),
            },
        },
        .remote_chip_id = PLATFORM_CHIP_2,
        .enable_smp_mode = true,
        .enable_direct_connect_mode = true,
        .enable_cpa_mode = CML_CPA_MODE,
        .cpag_config = {
            .cpag_id = 2,
            .ccg_count = CCG_PER_CPAG,
        },
    },
};
#endif

static struct mod_cmn_cyprus_config cmn_config_table[PLATFORM_CHIP_COUNT] = {
    [PLATFORM_CHIP_0] = {
        .periphbase = SCP_CMN_BASE,
        .mesh_size_x = MESH_SIZE_X,
        .mesh_size_y = MESH_SIZE_Y,
        .hnf_sam_config = {
            .snf_table = snf_table,
            .snf_count = SNF_TABLE_COUNT,
            .hnf_sam_mode = MOD_CMN_CYPRUS_HNF_SAM_MODE_DIRECT_MAPPING,
        },
        .rnsam_scg_config = {
            .scg_hashing_mode = MOD_CMN_CYPRUS_RNSAM_SCG_POWER_OF_TWO_HASHING,
        },
        .hns_cal_mode = true,
        .mmap_table = mmap,
        .mmap_count = MMAP_TABLE_COUNT,
        .chip_addr_space = UINT64_C(64) * FWK_GIB,
#if (PLATFORM_VARIANT == 2)
        .cml_config_table = chip0_cml_config,
        .cml_table_count =
            FWK_ARRAY_SIZE(chip0_cml_config),
        .cml_poll_timeout_us = UINT32_C(100),
        .enable_lcn = true,
#endif
    },
#if (PLATFORM_VARIANT == 2)
    [PLATFORM_CHIP_1] = {
        .periphbase = SCP_CMN_BASE,
        .mesh_size_x = MESH_SIZE_X,
        .mesh_size_y = MESH_SIZE_Y,
        .hnf_sam_config = {
            .snf_table = snf_table,
            .snf_count = SNF_TABLE_COUNT,
            .hnf_sam_mode = MOD_CMN_CYPRUS_HNF_SAM_MODE_DIRECT_MAPPING,
        },
        .rnsam_scg_config = {
            .scg_hashing_mode = MOD_CMN_CYPRUS_RNSAM_SCG_POWER_OF_TWO_HASHING,
        },
        .mmap_table = mmap,
        .mmap_count = MMAP_TABLE_COUNT,
        .chip_addr_space = UINT64_C(64) * FWK_GIB,
        .cml_config_table = chip1_cml_config,
        .cml_table_count =
            FWK_ARRAY_SIZE(chip1_cml_config),
        .hns_cal_mode = true,
        .cml_poll_timeout_us = UINT32_C(100),
        .enable_lcn = true,
    },
    [PLATFORM_CHIP_2] = {
        .periphbase = SCP_CMN_BASE,
        .mesh_size_x = MESH_SIZE_X,
        .mesh_size_y = MESH_SIZE_Y,
        .hnf_sam_config = {
            .snf_table = snf_table,
            .snf_count = SNF_TABLE_COUNT,
            .hnf_sam_mode = MOD_CMN_CYPRUS_HNF_SAM_MODE_DIRECT_MAPPING,
        },
        .rnsam_scg_config = {
            .scg_hashing_mode = MOD_CMN_CYPRUS_RNSAM_SCG_POWER_OF_TWO_HASHING,
        },
        .mmap_table = mmap,
        .mmap_count = MMAP_TABLE_COUNT,
        .chip_addr_space = UINT64_C(64) * FWK_GIB,
        .cml_config_table = chip2_cml_config,
        .cml_table_count =
            FWK_ARRAY_SIZE(chip2_cml_config),
        .hns_cal_mode = true,
        .cml_poll_timeout_us = UINT32_C(100),
        .enable_lcn = true,
    },
    [PLATFORM_CHIP_3] = {
        .periphbase = SCP_CMN_BASE,
        .mesh_size_x = MESH_SIZE_X,
        .mesh_size_y = MESH_SIZE_Y,
        .hnf_sam_config = {
            .snf_table = snf_table,
            .snf_count = SNF_TABLE_COUNT,
            .hnf_sam_mode = MOD_CMN_CYPRUS_HNF_SAM_MODE_DIRECT_MAPPING,
        },
        .rnsam_scg_config = {
            .scg_hashing_mode = MOD_CMN_CYPRUS_RNSAM_SCG_POWER_OF_TWO_HASHING,
        },
        .mmap_table = mmap,
        .mmap_count = MMAP_TABLE_COUNT,
        .chip_addr_space = UINT64_C(64) * FWK_GIB,
        .cml_config_table = chip3_cml_config,
        .cml_table_count =
            FWK_ARRAY_SIZE(chip3_cml_config),
        .hns_cal_mode = true,
        .cml_poll_timeout_us = UINT32_C(100),
        .enable_lcn = true,
    },
#endif
};

struct mod_cmn_cyprus_config_table cmn_config = {
    .chip_config_data = cmn_config_table,
    .chip_count = PLATFORM_CHIP_COUNT,
    .timer_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_TIMER, 0),
};

const struct fwk_module_config config_cmn_cyprus = {
    .data = (void *)&cmn_config,
};
