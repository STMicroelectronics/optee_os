/*
 * Arm SCP/MCP Software
 * Copyright (c) 2023-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MGI_VALIDATE_HEADER_H
#define MGI_VALIDATE_HEADER_H

#include "mgi.h"

#include <fwk_assert.h>

#include <stddef.h>

static_assert(
    MGI_GRP_ID_OFFSET == offsetof(struct smcf_mgi_reg, GRP_ID),
    "[SMCF] MGI_GRP_ID_OFFSET mismatch");
static_assert(
    MGI_DATA_INFO_OFFSET == offsetof(struct smcf_mgi_reg, DATA_INFO),
    "[SMCF] MGI_DATA_INFO_OFFSET mismatch");
static_assert(
    MGI_FEAT0_OFFSET == offsetof(struct smcf_mgi_reg, FEAT0),
    "[SMCF] MGI_FEAT0_OFFSET mismatch");
static_assert(
    MGI_FEAT1_OFFSET == offsetof(struct smcf_mgi_reg, FEAT1),
    "[SMCF] MGI_FEAT1_OFFSET mismatch");
static_assert(
    MGI_SMP_EN_OFFSET == offsetof(struct smcf_mgi_reg, SMP_EN),
    "[SMCF] MGI_SMP_EN_OFFSET mismatch");
static_assert(
    MGI_SMP_CFG_OFFSET == offsetof(struct smcf_mgi_reg, SMP_CFG),
    "[SMCF] MGI_SMP_CFG_OFFSET mismatch");
static_assert(
    MGI_SMP_PER_OFFSET == offsetof(struct smcf_mgi_reg, SMP_PER),
    "[SMCF] MGI_SMP_PER_OFFSET mismatch");
static_assert(
    MGI_SMP_DLY_OFFSET == offsetof(struct smcf_mgi_reg, SMP_DLY),
    "[SMCF] MGI_SMP_DLY_OFFSET mismatch");
static_assert(
    MGI_MON_REQ_OFFSET == offsetof(struct smcf_mgi_reg, MON_REQ),
    "[SMCF] MGI_MON_REQ_OFFSET mismatch");
static_assert(
    MGI_MON_STAT_OFFSET == offsetof(struct smcf_mgi_reg, MON_STAT),
    "[SMCF] MGI_MON_STAT_OFFSET mismatch");
static_assert(
    MGI_MODE_BCAST_OFFSET == offsetof(struct smcf_mgi_reg, MODE_BCAST),
    "[SMCF] MGI_MODE_BCAST_OFFSET mismatch");
static_assert(
    MGI_MODE_REQ0_OFFSET == offsetof(struct smcf_mgi_reg, MODE_REQ0),
    "[SMCF] MGI_MODE_REQ0_OFFSET mismatch");
static_assert(
    MGI_MODE_REQ1_OFFSET == offsetof(struct smcf_mgi_reg, MODE_REQ1),
    "[SMCF] MGI_MODE_REQ1_OFFSET mismatch");
static_assert(
    MGI_MODE_REQ2_OFFSET == offsetof(struct smcf_mgi_reg, MODE_REQ2),
    "[SMCF] MGI_MODE_REQ2_OFFSET mismatch");
static_assert(
    MGI_MODE_REQ3_OFFSET == offsetof(struct smcf_mgi_reg, MODE_REQ3),
    "[SMCF] MGI_MODE_REQ3_OFFSET mismatch");
static_assert(
    MGI_MODE_STAT0_OFFSET == offsetof(struct smcf_mgi_reg, MODE_STAT0),
    "[SMCF] MGI_MODE_STAT0_OFFSET mismatch");
static_assert(
    MGI_MODE_STAT1_OFFSET == offsetof(struct smcf_mgi_reg, MODE_STAT1),
    "[SMCF] MGI_MODE_STAT1_OFFSET mismatch");
static_assert(
    MGI_MODE_STAT2_OFFSET == offsetof(struct smcf_mgi_reg, MODE_STAT2),
    "[SMCF] MGI_MODE_STAT2_OFFSET mismatch");
static_assert(
    MGI_MODE_STAT3_OFFSET == offsetof(struct smcf_mgi_reg, MODE_STAT3),
    "[SMCF] MGI_MODE_STAT3_OFFSET mismatch");
static_assert(
    MGI_IRQ_STAT_OFFSET == offsetof(struct smcf_mgi_reg, IRQ_STAT),
    "[SMCF] MGI_IRQ_STAT_OFFSET mismatch");
static_assert(
    MGI_IRQ_MASK_OFFSET == offsetof(struct smcf_mgi_reg, IRQ_MASK),
    "[SMCF] MGI_IRQ_MASK_OFFSET mismatch");
static_assert(
    MGI_TRG_MASK_OFFSET == offsetof(struct smcf_mgi_reg, TRG_MASK),
    "[SMCF] MGI_TRG_MASK_OFFSET mismatch");
static_assert(
    MGI_ERR_CODE_OFFSET == offsetof(struct smcf_mgi_reg, ERR_CODE),
    "[SMCF] MGI_ERR_CODE_OFFSET mismatch");
static_assert(
    MGI_WREN_OFFSET == offsetof(struct smcf_mgi_reg, WREN),
    "[SMCF] MGI_WREN_OFFSET mismatch");
static_assert(
    MGI_WRCFG_OFFSET == offsetof(struct smcf_mgi_reg, WRCFG),
    "[SMCF] MGI_WRCFG_OFFSET mismatch");
static_assert(
    MGI_WADDR0_OFFSET == offsetof(struct smcf_mgi_reg, WADDR0),
    "[SMCF] MGI_WADDR0_OFFSET mismatch");
static_assert(
    MGI_WADDR1_OFFSET == offsetof(struct smcf_mgi_reg, WADDR1),
    "[SMCF] MGI_WADDR1_OFFSET mismatch");
static_assert(
    MGI_RADDR0_OFFSET == offsetof(struct smcf_mgi_reg, RADDR0),
    "[SMCF] MGI_RADDR0_OFFSET mismatch");
static_assert(
    MGI_RADDR1_OFFSET == offsetof(struct smcf_mgi_reg, RADDR1),
    "[SMCF] MGI_RADDR1_OFFSET mismatch");
static_assert(
    MGI_DISCON_ID_OFFSET == offsetof(struct smcf_mgi_reg, DISCON_ID),
    "[SMCF] MGI_DISCON_ID_OFFSET mismatch");
static_assert(
    MGI_CON_STAT_OFFSET == offsetof(struct smcf_mgi_reg, CON_STAT),
    "[SMCF] MGI_CON_STAT_OFFSET mismatch");
static_assert(
    MGI_CMD_SEND0_OFFSET == offsetof(struct smcf_mgi_reg, CMD_SEND0),
    "[SMCF] MGI_CMD_SEND0_OFFSET mismatch");
static_assert(
    MGI_CMD_SEND1_OFFSET == offsetof(struct smcf_mgi_reg, CMD_SEND1),
    "[SMCF] MGI_CMD_SEND1_OFFSET mismatch");
static_assert(
    MGI_CMD_RECV0_OFFSET == offsetof(struct smcf_mgi_reg, CMD_RECV0),
    "[SMCF] MGI_CMD_RECV0_OFFSET mismatch");
static_assert(
    MGI_CMD_RECV1_OFFSET == offsetof(struct smcf_mgi_reg, CMD_RECV1),
    "[SMCF] MGI_CMD_RECV1_OFFSET mismatch");
static_assert(
    MGI_ATYP0_OFFSET == offsetof(struct smcf_mgi_reg, ATYP0),
    "[SMCF] MGI_ATYP0_OFFSET mismatch");
static_assert(
    MGI_ATYP1_OFFSET == offsetof(struct smcf_mgi_reg, ATYP1),
    "[SMCF] MGI_ATYP1_OFFSET mismatch");
static_assert(
    MGI_AVAL_LOW0_OFFSET == offsetof(struct smcf_mgi_reg, AVAL_LOW0),
    "[SMCF] MGI_AVAL_LOW0_OFFSET mismatch");
static_assert(
    MGI_AVAL_LOW1_OFFSET == offsetof(struct smcf_mgi_reg, AVAL_LOW1),
    "[SMCF] MGI_AVAL_LOW1_OFFSET mismatch");
static_assert(
    MGI_AVAL_LOW2_OFFSET == offsetof(struct smcf_mgi_reg, AVAL_LOW2),
    "[SMCF] MGI_AVAL_LOW2_OFFSET mismatch");
static_assert(
    MGI_AVAL_LOW3_OFFSET == offsetof(struct smcf_mgi_reg, AVAL_LOW3),
    "[SMCF] MGI_AVAL_LOW3_OFFSET mismatch");
static_assert(
    MGI_AVAL_HIGH6_OFFSET == offsetof(struct smcf_mgi_reg, AVAL_HIGH6),
    "[SMCF] MGI_AVAL_HIGH6_OFFSET mismatch");
static_assert(
    MGI_DATA_OFFSET == offsetof(struct smcf_mgi_reg, DATA),
    "[SMCF] MGI_DATA_OFFSET mismatch");
static_assert(
    MGI_DVLD_OFFSET == offsetof(struct smcf_mgi_reg, DVLD),
    "[SMCF] MGI_DVLD_OFFSET mismatch");
static_assert(
    MGI_TAG0_OFFSET == offsetof(struct smcf_mgi_reg, TAG0),
    "[SMCF] MGI_TAG0_OFFSET mismatch");
static_assert(
    MGI_TAG1_OFFSET == offsetof(struct smcf_mgi_reg, TAG1),
    "[SMCF] MGI_TAG1_OFFSET mismatch");
static_assert(
    MGI_SMPID_START_OFFSET == offsetof(struct smcf_mgi_reg, SMPID_START),
    "[SMCF] MGI_SMPID_START_OFFSET mismatch");
static_assert(
    MGI_SMPID_END_OFFSET == offsetof(struct smcf_mgi_reg, SMPID_END),
    "[SMCF] MGI_SMPID_END_OFFSET mismatch");
static_assert(
    MGI_IIDR_OFFSET == offsetof(struct smcf_mgi_reg, IIDR),
    "[SMCF] MGI_IIDR_OFFSET mismatch");
static_assert(
    MGI_AIDR_OFFSET == offsetof(struct smcf_mgi_reg, AIDR),
    "[SMCF] MGI_AIDR_OFFSET mismatch");

#endif /* MGI_VALIDATE_HEADER_H */
