/*
 * Arm SCP/MCP Software
 * Copyright (c) 2023-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Definitions for mock PPU module configuration.
 */

#ifndef TC4_MOCK_PPU_H
#define TC4_MOCK_PPU_H

#include <tc4_core.h>

#define MOCK_PPU_ENUM_CME_GEN(_cme) MOCK_PPU_ELEMENT_IDX_CME##_cme

enum mock_ppu_static_element_idx {
#if defined(PLAT_FVP)
    TC4_FOR_EACH_CME(MOCK_PPU_ENUM_CME_GEN),
#endif
    MOCK_PPU_ELEMENT_IDX_SYS0,
    MOCK_PPU_ELEMENT_IDX_GPUCGRP,
    MOCK_PPU_ELEMENT_IDX_COUNT
};

#endif /* TC4_MOCK_PPU_H */
