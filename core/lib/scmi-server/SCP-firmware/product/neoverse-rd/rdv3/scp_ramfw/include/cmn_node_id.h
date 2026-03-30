/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     CMN node ID.
 */

#ifndef CMN_NODE_ID
#define CMN_NODE_ID

/*
 * CMN node ids
 */
#if (PLATFORM_VARIANT == 0)
#    define MEM_CNTRL0_ID 64
#    define MEM_CNTRL1_ID 128
#    define MEM_CNTRL2_ID 192
#    define MEM_CNTRL3_ID 256
#    define MEM_CNTRL4_ID 104
#    define MEM_CNTRL5_ID 168
#    define MEM_CNTRL6_ID 232
#    define MEM_CNTRL7_ID 296

#    define NODE_ID_HNI_GIC 236

#    define NODE_ID_HND 300

#    define NODE_ID_HNT0 4
#    define NODE_ID_HNT1 260
#    define NODE_ID_HNT2 44

#    define NODE_ID_HNP0 324
#    define NODE_ID_HNP1 332
#    define NODE_ID_HNP2 340
#    define NODE_ID_HNP3 348
#    define NODE_ID_HNP4 356
#    define NODE_ID_HNP5 364

#    define NODE_ID_SBSX 172

#    define NODE_ID_NON_PCIE_IO_MACRO NODE_ID_HNP4

#    define MESH_SIZE_X 7
#    define MESH_SIZE_Y 6

#    define IOVB_NODE_ID0 NODE_ID_HNP0
#    define IOVB_NODE_ID1 NODE_ID_HNP2
#    define IOVB_NODE_ID2 NODE_ID_HNP4
#    define IOVB_NODE_ID3 NODE_ID_HNT0
#    define IOVB_NODE_ID4 NODE_ID_HNT2

#elif (PLATFORM_VARIANT == 1)
#    define MEM_CNTRL0_ID 32
#    define MEM_CNTRL1_ID 64

#    define NODE_ID_HNI_GIC 236

#    define NODE_ID_HND 68

#    define NODE_ID_HNT0 0
#    define NODE_ID_HNP0 2
#    define NODE_ID_HNP1 3

#    define NODE_ID_SBSX 66

#    define MESH_SIZE_X 3
#    define MESH_SIZE_Y 3

#    define IOVB_NODE_ID0 NODE_ID_HNP0

#elif (PLATFORM_VARIANT == 2)
#    define MEM_CNTRL0_ID   0x40
#    define MEM_CNTRL1_ID   0x80
#    define MEM_CNTRL2_ID   0xC0
#    define MEM_CNTRL3_ID   0x100
#    define MEM_CNTRL4_ID   0x68
#    define MEM_CNTRL5_ID   0xA8
#    define MEM_CNTRL6_ID   0xE8
#    define MEM_CNTRL7_ID   0x128
#    define NODE_ID_HNI_GIC 0xEC

#    define NODE_ID_HND 0x12C

#    define NODE_ID_HNT0 0x4
#    define NODE_ID_HNT1 0x104
#    define NODE_ID_HNT2 0x2c

#    define NODE_ID_HNP0 0x144
#    define NODE_ID_HNP1 0x14C
#    define NODE_ID_HNP2 0x154
#    define NODE_ID_HNP3 0x15C
#    define NODE_ID_HNP4 0x164
#    define NODE_ID_HNP5 0x16C

#    define NODE_ID_SBSX 0xAC

#    define NODE_ID_NON_PCIE_IO_MACRO NODE_ID_HNP4

#    define NODE_ID_CCG0 0x184
#    define NODE_ID_CCG1 0x8
#    define NODE_ID_CCG2 0x18c
#    define NODE_ID_CCG3 0x10
#    define NODE_ID_CCG4 0x194
#    define NODE_ID_CCG5 0x18
#    define NODE_ID_CCG6 0x19c
#    define NODE_ID_CCG7 0x20
#    define NODE_ID_CCG8 0x1a4
#    define NODE_ID_CCG9 0x1ac

#    define MESH_SIZE_X 7
#    define MESH_SIZE_Y 6

#    define IOVB_NODE_ID0 NODE_ID_HNP0
#endif

#endif /* CMN_NODE_ID */
