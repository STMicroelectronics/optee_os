/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef NI_710AE_APU_REG_H
#define NI_710AE_APU_REG_H

#include <stdint.h>

#define NI_710AE_MAX_APU_REGIONS 32

/**
 * \brief NI-710AE APU register map
 */
struct ni710ae_apu_reg_map_t {
    struct {
        volatile uint32_t prbar_low;
        volatile uint32_t prbar_high;
        volatile uint32_t prlar_low;
        volatile uint32_t prlar_high;
        volatile uint32_t prid_low;
        volatile uint32_t prid_high;
        const uint32_t reserved_0[2];
    } region[NI_710AE_MAX_APU_REGIONS];

    const uint32_t reserved_1[766];

    volatile uint32_t apu_ctlr;

    const volatile uint32_t apu_iidr;
};

#endif /* NI_710AE_APU_REG_H */
