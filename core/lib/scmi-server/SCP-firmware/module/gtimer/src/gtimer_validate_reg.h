/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef GTIMER_VALIDATE_REG_H
#define GTIMER_VALIDATE_REG_H

#include "gtimer_reg.h"

static_assert(
    CNTCR_OFFSET == offsetof(struct cntcontrol_reg, CR),
    "[GTIMER] CNTCR_OFFSET mismatch");
static_assert(
    CNTSR_OFFSET == offsetof(struct cntcontrol_reg, SR),
    "[GTIMER] CNTSR_OFFSET mismatch");
static_assert(
    CNTCV_L_OFFSET == offsetof(struct cntcontrol_reg, CVL),
    "[GTIMER] CNTCV_L_OFFSET mismatch");
static_assert(
    CNTCV_H_OFFSET == offsetof(struct cntcontrol_reg, CVH),
    "[GTIMER] CNTCV_H_OFFSET mismatch");
static_assert(
    CNTSCR_OFFSET == offsetof(struct cntcontrol_reg, CSR),
    "[GTIMER] CNTSCR_OFFSET mismatch");
static_assert(
    CNTID_OFFSET == offsetof(struct cntcontrol_reg, ID),
    "[GTIMER] CNTID_OFFSET mismatch");
static_assert(
    CNTFID0_OFFSET == offsetof(struct cntcontrol_reg, FID0),
    "[GTIMER] CNTFID0_OFFSET mismatch");
static_assert(
    IMP_DEF_OFFSET == offsetof(struct cntcontrol_reg, IMP_DEF),
    "[GTIMER] IMP_DEF_OFFSET mismatch");
static_assert(
    COUNTERID_OFFSET == offsetof(struct cntcontrol_reg, PID),
    "[GTIMER] COUNTERID_OFFSET mismatch");

#endif /* GTIMER_VALIDATE_REG_H */
