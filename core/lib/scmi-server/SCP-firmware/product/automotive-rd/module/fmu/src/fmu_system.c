/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "internal/fmu_common.h"

#include <fwk_assert.h>
#include <fwk_log.h>
#include <fwk_macros.h>
#include <fwk_math.h>
#include <fwk_mmio.h>
#include <fwk_status.h>

#include <stddef.h>
#include <stdint.h>

#define FMU_FIELD_ERR_FR(n)     (0x0000 + (64 * (n)))
#define FMU_FIELD_ERR_CTRL(n)   (0x0008 + (64 * (n)))
#define FMU_FIELD_ERR_STATUS(n) (0x0010 + (64 * (n)))
#define FMU_FIELD_ERRIMPDEF(n)  (0x8000 + (8 * (n)))
#define FMU_FIELD_ERRGSR_L(n)   (0xE000 + (8 * (n)))
#define FMU_FIELD_ERRGSR_H(n)   (0xE004 + (8 * n))
#define FMU_FIELD_SYS_KEY       0x8BFC

#define FMU_ERR_FR_ED_MASK FWK_GEN_MASK(1, 0)

#define FMU_ERR_STATUS_IERR_MASK               FWK_GEN_MASK(12, 8)
#define FMU_ERR_STATUS_IERR_SHIFT              8
#define FMU_ERR_STATUS_IERR_APB_SW_MASK        FWK_BIT(8)
#define FMU_ERR_STATUS_IERR_INC_SEQ_MASK       FWK_BIT(9)
#define FMU_ERR_STATUS_IERR_APB_PARITY_MASK    FWK_BIT(10)
#define FMU_ERR_STATUS_IERR_ERR_IN_PARITY_MASK FWK_BIT(11)
#define FMU_ERR_STATUS_IERR_ERR_IN_MASK        FWK_BIT(12)

#define FMU_ERR_CTRL_ED_MASK  FWK_BIT(0)
#define FMU_ERR_CTRL_FI_MASK  FWK_BIT(3)
#define FMU_ERR_CTRL_UE_MASK  FWK_BIT(4)
#define FMU_ERR_CTRL_CFI_MASK FWK_BIT(8)
#define FMU_ERR_CTRL_CI_MASK  FWK_BIT(13)
#define FMU_ERR_CTRL_ENABLE_MASK \
    (FMU_ERR_CTRL_ED_MASK | FMU_ERR_CTRL_FI_MASK | FMU_ERR_CTRL_UE_MASK | \
     FMU_ERR_CTRL_CFI_MASK | FMU_ERR_CTRL_CI_MASK)

#define FMU_ERRIMPDEF_UE_MASK   FWK_BIT(0)
#define FMU_ERRIMPDEF_IC_MASK   FWK_GEN_MASK(3, 2)
#define FMU_ERRIMPDEF_IE_MASK   FWK_GEN_MASK(13, 9)
#define FMU_ERRIMPDEF_IE_SHIFT  9
#define FMU_ERRIMPDEF_THR_MASK  FWK_GEN_MASK(31, 24)
#define FMU_ERRIMPDEF_THR_SHIFT 24
#define FMU_ERRIMPDEF_CNT_MASK  FWK_GEN_MASK(23, 16)
#define FMU_ERRIMPDEF_CNT_SHIFT 16

#define FMU_ERRGSR_MAX      5
#define FMU_ERRGSR_NUM_BITS 32

#define FMU_SYS_KEY_UNLOCK 0xBE

static inline uint32_t fmu_read_32(uintptr_t base, uintptr_t offset)
{
    return fwk_mmio_read_32(base + offset);
}

static inline void fmu_write_32(
    uintptr_t base,
    uintptr_t offset,
    uint32_t value)
{
    fwk_mmio_write_32(base + FMU_FIELD_SYS_KEY, FMU_SYS_KEY_UNLOCK);
    fwk_mmio_write_32(base + offset, value);
}

static unsigned int find_active_node(const struct mod_fmu_dev_config *config)
{
    uint64_t errgsr;
    unsigned int errgsr_idx;

    fwk_assert(config != NULL);

    /* Determine fault record idx */
    for (errgsr_idx = 0; errgsr_idx <= FMU_ERRGSR_MAX; errgsr_idx++) {
        errgsr = fmu_read_32(config->base, FMU_FIELD_ERRGSR_L(errgsr_idx)) |
            ((uint64_t)fmu_read_32(config->base, FMU_FIELD_ERRGSR_H(errgsr_idx))
             << FMU_ERRGSR_NUM_BITS);

        if (errgsr != 0) {
            return (errgsr_idx * FMU_ERRGSR_NUM_BITS * 2) +
                fwk_math_log2(LSB_GET(errgsr));
        }
    }

    return MOD_FMU_PARENT_NONE;
}

static bool fault_peek(
    const struct mod_fmu_dev_config *config,
    unsigned int *node_idx)
{
    unsigned int next_node_idx;

    fwk_assert(config != NULL);
    fwk_assert(node_idx != NULL);

    /* If current FMU has an active fault record */
    next_node_idx = find_active_node(config);
    if (next_node_idx == MOD_FMU_PARENT_NONE) {
        return false;
    }
    *node_idx = next_node_idx;

    return true;
}

static void fault_ack(
    const struct mod_fmu_dev_config *config,
    struct mod_fmu_fault *fault,
    unsigned int node_idx,
    bool *fault_tracked)
{
    uint32_t val;

    fwk_assert(config != NULL);
    fwk_assert(fault != NULL);
    fwk_assert(fault_tracked != NULL);

    /* Acknowledge the fault */
    val = fmu_read_32(config->base, FMU_FIELD_ERRIMPDEF(node_idx));
    val |= FMU_ERRIMPDEF_IC_MASK;
    fmu_write_32(config->base, FMU_FIELD_ERRIMPDEF(node_idx), val);

    if (!(*fault_tracked)) {
        fault->node_idx = node_idx;
        fault->sm_idx =
            (fmu_read_32(config->base, FMU_FIELD_ERR_STATUS(node_idx)) &
             FMU_ERR_STATUS_IERR_MASK) >>
            FMU_ERR_STATUS_IERR_SHIFT;
        *fault_tracked = true;
    }
}

/*
 * API Handlers
 */
static int inject(
    const struct mod_fmu_dev_config *config,
    const struct mod_fmu_fault *fault)
{
    uint32_t val;

    fwk_assert(config != NULL);
    fwk_assert(fault != NULL);

    val = fmu_read_32(config->base, FMU_FIELD_ERRIMPDEF(fault->node_idx));
    val |= ((uint32_t)fault->sm_idx << FMU_ERRIMPDEF_IE_SHIFT) &
        FMU_ERRIMPDEF_IE_MASK;
    /* Ensure injection ack bits are cleared */
    val &= ~(FMU_ERRIMPDEF_IC_MASK);

    fmu_write_32(config->base, FMU_FIELD_ERRIMPDEF(fault->node_idx), val);

    return FWK_SUCCESS;
}

static int get_enabled(
    const struct mod_fmu_dev_config *config,
    const struct mod_fmu_fault *fault,
    bool *enabled)
{
    uint32_t val;

    fwk_assert(config != NULL);
    fwk_assert(fault != NULL);
    fwk_assert(enabled != NULL);

    if (fault->sm_idx != MOD_FMU_SM_ALL) {
        return FWK_E_SUPPORT;
    }

    val = fmu_read_32(config->base, FMU_FIELD_ERR_CTRL(fault->node_idx));
    *enabled = (val & FMU_ERR_CTRL_ED_MASK) != 0;

    return FWK_SUCCESS;
}

static int set_enabled(
    const struct mod_fmu_dev_config *config,
    const struct mod_fmu_fault *fault,
    bool enabled)
{
    uint32_t val;

    fwk_assert(config != NULL);
    fwk_assert(fault != NULL);

    if (fault->sm_idx != MOD_FMU_SM_ALL) {
        return FWK_E_SUPPORT;
    }

    val = fmu_read_32(config->base, FMU_FIELD_ERR_CTRL(fault->node_idx));
    if (enabled) {
        val |= FMU_ERR_CTRL_ENABLE_MASK;
    } else {
        val &= ~FMU_ERR_CTRL_ENABLE_MASK;
    }
    fmu_write_32(config->base, FMU_FIELD_ERR_CTRL(fault->node_idx), val);

    return FWK_SUCCESS;
}

static int get_count(
    const struct mod_fmu_dev_config *config,
    uint16_t node_id,
    uint8_t *count)
{
    uint32_t val;

    fwk_assert(config != NULL);
    fwk_assert(count != NULL);

    val = fmu_read_32(config->base, FMU_FIELD_ERRIMPDEF(node_id));
    *count = (val & FMU_ERRIMPDEF_CNT_MASK) >> FMU_ERRIMPDEF_CNT_SHIFT;

    return FWK_SUCCESS;
}

static int set_count(
    const struct mod_fmu_dev_config *config,
    uint16_t node_id,
    uint8_t count)
{
    uint32_t val;

    fwk_assert(config != NULL);

    val = fmu_read_32(config->base, FMU_FIELD_ERRIMPDEF(node_id));
    val &= ~FMU_ERRIMPDEF_CNT_MASK;
    val |= ((uint32_t)count << FMU_ERRIMPDEF_CNT_SHIFT);
    fmu_write_32(config->base, FMU_FIELD_ERRIMPDEF(node_id), val);

    return FWK_SUCCESS;
}

static int get_threshold(
    const struct mod_fmu_dev_config *config,
    uint16_t node_id,
    uint8_t *threshold)
{
    uint32_t val;

    fwk_assert(config != NULL);
    fwk_assert(threshold != NULL);

    val = fmu_read_32(config->base, FMU_FIELD_ERRIMPDEF(node_id));
    *threshold = (val & FMU_ERRIMPDEF_THR_MASK) >> FMU_ERRIMPDEF_THR_SHIFT;

    return FWK_SUCCESS;
}

static int set_threshold(
    const struct mod_fmu_dev_config *config,
    uint16_t node_id,
    uint8_t threshold)
{
    uint32_t val;

    fwk_assert(config != NULL);

    val = fmu_read_32(config->base, FMU_FIELD_ERRIMPDEF(node_id));
    val &= ~FMU_ERRIMPDEF_THR_MASK;
    val |= ((uint32_t)threshold << FMU_ERRIMPDEF_THR_SHIFT);
    fmu_write_32(config->base, FMU_FIELD_ERRIMPDEF(node_id), val);

    return FWK_SUCCESS;
}

static int get_upgrade_enabled(
    const struct mod_fmu_dev_config *config,
    uint16_t node_id,
    bool *enabled)
{
    uint32_t val;

    fwk_assert(config != NULL);
    fwk_assert(enabled != NULL);

    val = fmu_read_32(config->base, FMU_FIELD_ERRIMPDEF(node_id));
    *enabled = (val & FMU_ERRIMPDEF_UE_MASK) != 0;

    return FWK_SUCCESS;
}

static int set_upgrade_enabled(
    const struct mod_fmu_dev_config *config,
    uint16_t node_id,
    bool enabled)
{
    uint32_t val;

    fwk_assert(config != NULL);

    val = fmu_read_32(config->base, FMU_FIELD_ERRIMPDEF(node_id));
    if (enabled) {
        val |= FMU_ERRIMPDEF_UE_MASK;
    } else {
        val &= ~FMU_ERRIMPDEF_UE_MASK;
    }
    fmu_write_32(config->base, FMU_FIELD_ERRIMPDEF(node_id), val);

    return FWK_SUCCESS;
}

struct mod_fmu_impl_api mod_fmu_system_api = {
    .fault_peek = fault_peek,
    .fault_ack = fault_ack,
    .inject = inject,
    .get_enabled = get_enabled,
    .set_enabled = set_enabled,
    .get_count = get_count,
    .set_count = set_count,
    .get_threshold = get_threshold,
    .set_threshold = set_threshold,
    .get_upgrade_enabled = get_upgrade_enabled,
    .set_upgrade_enabled = set_upgrade_enabled,
};
