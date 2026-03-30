/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Base address definitions for the SCP's sub-system and access extending
 *     into the rest of the CSS.
 */

#ifndef IO_MACRO_LAYOUT
#define IO_MACRO_LAYOUT

#include "scp_mmap.h"

/* Calculate the base address of NCI GVP for IO Block using its index. */
#define NCI_GVP_BLOCK_BASE(idx) \
    (SCP_IO_BLOCK_NCI_GVP_BASE + (idx * SCP_IO_BLOCK_REGISTER_SIZE))
/* Calculate the base address for the TCU in IO block using IO block's index. */
#define TCU_REG_BASE(idx) \
    (SCP_IO_BLOCK_BASE + (idx * SCP_IO_BLOCK_REGISTER_SIZE))
#define TCU_REG_SIZE (0x4000000UL)
/* Calculate the base address of IO control register bank. */
#define CTRL_REG_BASE(idx) \
    (SCP_IO_BLOCK_CONTROL_REG + (idx * SCP_IO_BLOCK_REGISTER_SIZE))
#define CTRL_REG_SIZE (0x10000UL)

enum asni_domain_id {
    /*! CMN completer Node ID. */
    ASNI_CMN,
    /*! PCIe X1 completer Node ID. */
    ASNI_PCIEX1_0,
    /*! PCIe X2_1 completer Node ID. */
    ASNI_PCIEX2_1,
    /*! Maximum number of completer Node IDs*/
    ASNI_DOMAIN_MAX_ID
};

enum amni_domain_id {
    /*! requester Node ID. */
    AMNI_CMN,
    /*! Inline IO configuration requester Node ID. */
    AMNI_INLINE_IO_CFG,
    /*! Inline IO requester Node ID. */
    AMNI_INLINE_IO,
    /*! PCIE x1_0 configurtion requester Node ID. */
    AMNI_PCIEX1_0_CFG,
    /*! PCIe x1_0 requester Node ID. */
    AMNI_PCIEX1_0,
    /*! PCIE x2_0 configurtion requester Node ID. */
    AMNI_PCIEX2_0_CFG,
    /*! PCIe x2_0 requester Node ID. */
    AMNI_PCIEX2_0,
    /*! PCIE x2_1 configurtion requester Node ID. */
    AMNI_PCIEX2_1_CFG,
    /*! PCIe x2_1 requester Node ID. */
    AMNI_PCIEX2_1,
    /*! PCIE x4 configurtion requester Node ID. */
    AMNI_PCIEX4_0_CFG,
    /*! PCIe x4 requester Node ID. */
    AMNI_PCIEX4_0,
    /*! PCIE x8 configurtion requester Node ID. */
    AMNI_PCIEX8_0_CFG,
    /*! PCIe x8 requester Node ID. */
    AMNI_PCIEX8_0,
    /*! PMNI Control requester Node ID. */
    AMNI_PMNI_CTRL_REG_APB,
    /*! TCU APB requester Node ID. */
    AMNI_PMNI_TCU_APB,
    /*! Maximum numbers of requester Node ID. */
    AMNI_DOMAIN_MAX_ID
};

#endif /* IO_MACRO_LAYOUT */
