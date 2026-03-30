/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Base address and offset definitions for the various addressable regions
 *     within the LCP Peripheral memory region in the Cluster Utility memory
 *     map space.
 */

#ifndef SCP_CU_LCP_PERIPH_H
#define SCP_CU_LCP_PERIPH_H

#include "scp_atw0_mmap.h"

#include <fwk_macros.h>

#include <stdint.h>

// clang-format off
struct lcp_extended_control_reg {
    FWK_RW  uint32_t  LCP_CFG;
            uint8_t   RESERVED0[0x10-0x4];
    FWK_RW  uint32_t  LCP_RST_SYN;
            uint8_t   RESERVED1[0x18-0x14];
    FWK_RW  uint32_t  LCP_EXTEND_RST_SYN;
            uint8_t   RESERVED2[0x100-0x1C];
    FWK_RW  uint32_t  LCP_SMCF_TRIGGER_CTRL;
            uint8_t   RESERVED3[0x900-0x104];
    FWK_RW  uint32_t  LCP_TCM_ACC_CTRL;
            uint8_t   RESERVED4[0x910-0x904];
    FWK_RW  uint32_t  LCP_CTRL_CFG;
            uint8_t   RESERVED5[0x1000-0x914];
};

struct lcp_external_control_reg {
    FWK_RW  uint32_t  RST_CTRL;
            uint8_t   RESERVED0[0x10-0x4];
    FWK_RW  uint32_t  RST_SYN;
            uint8_t   RESERVED1[0x20-0x14];
    FWK_RW  uint32_t  UART_CTRL;
            uint8_t   RESERVED2[0x30-0x24];
    FWK_R   uint32_t  COMB_IRQ_STAT;
            uint8_t   RESERVED3[0x38-0x34];
    FWK_RW  uint32_t  COMB_IRQ_MASK;
            uint8_t   RESERVED4[0x40-0x3C];
    FWK_R   uint32_t  LCP_APCONSOLIDATED_IRQ_STAT;
            uint8_t   RESERVED5[0x48-0x44];
    FWK_RW  uint32_t  LCP_APCONSOLIDATED_IRQ_MASK;
            uint8_t   RESERVED6[0x50-0x4C];
    FWK_R   uint32_t  COREPPU_IRQ_STAT;
            uint8_t   RESERVED7[0x58-0x54];
    FWK_RW  uint32_t  COREPPU_IRQ_MASK;
            uint8_t   RESERVED8[0x60-0x5C];
    FWK_R   uint32_t  MGI_IRQ_STAT;
            uint8_t   RESERVED9[0x68-0x64];
    FWK_RW  uint32_t  MGI_IRQ_MASK;
            uint8_t   RESERVED10[0x1000-0x6C];
};

struct lcp_control_reg {
            uint8_t   RESERVED0[0x120-0x0];
    FWK_RW  uint32_t  CPUWAIT;
            uint8_t   RESERVED1[0x1000-0x124];
};
// clang-format on

/*
 * Offsets of various blocks within a LCP peripheral space (also referred to
 * as 'LCP Group') in the cluster utility MMAP memory region. These offsets
 * are applicable to each LCP sub-system in the system.
 */

#define SCP_LCP_PERIPH_CONTROL_OFFSET          (0x21000)
#define SCP_LCP_PERIPH_EXTERNAL_CONTROL_OFFSET (0x30000)
#define SCP_LCP_PERIPH_EXTENDED_CONTROL_OFFSET (0x42000)

#define SCP_LCP_EXTENDED_CONTROL_PTR(IDX) \
    ((struct lcp_extended_control_reg \
          *)(SCP_CU_LCP_PERIPH_BASE_N(IDX) + SCP_LCP_PERIPH_EXTENDED_CONTROL_OFFSET))

#define SCP_LCP_EXTERNAL_CONTROL_PTR(IDX) \
    ((struct lcp_external_control_reg \
          *)(SCP_CU_LCP_PERIPH_BASE_N(IDX) + SCP_LCP_PERIPH_EXTERNAL_CONTROL_OFFSET))

#define SCP_LCP_CONTROL_PTR(IDX) \
    ((struct lcp_control_reg \
          *)(SCP_CU_LCP_PERIPH_BASE_N(IDX) + SCP_LCP_PERIPH_CONTROL_OFFSET))

/*
 * LCP external control register block bit-field definitions.
 */
#define LCP_PERIPH_EXTRCTRL_UART_CTRL_EN_SHIFT (0)
#define LCP_PERIPH_EXTRCTRL_UART_CTRL_EN_MASK  (1)
#define LCP_PERIPH_EXTRCTRL_UART_CTRL_EN_VAL   (1)

/*
 * LCP control register block bit-field definitions.
 */
#define LCP_PERIPH_CONTROL_CPU_WAIT_CPU0WAIT_SHIFT (0)
#define LCP_PERIPH_CONTROL_CPU_WAIT_CPU0WAIT_MASK  (1)
#define LCP_PERIPH_CONTROL_CPU_WAIT_CPU0WAIT_VAL   (1)

/*
 * LCP extended control register block bit-field definitions.
 */
#define LCP_PERIPH_EXTDCTRL_LCP_CONFIG_CTRL_RATIO_SHIFT (16)
#define LCP_PERIPH_EXTDCTRL_LCP_CONFIG_CTRL_RATIO_MASK  (0xFF)

#endif /* SCP_CU_LCP_PERIPH_H */
