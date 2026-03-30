/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "internal/fmu_common.h"

#include <mod_timer.h>

#include <fwk_assert.h>
#include <fwk_log.h>
#include <fwk_math.h>
#include <fwk_mmio.h>
#include <fwk_module.h>
#include <fwk_status.h>
#include <fwk_time.h>

#include <stddef.h>
#include <stdint.h>

#define FMU_FIELD_ERRSTATUS(n) (0x010 + (n * 64))
#define FMU_FIELD_ERRGSR       0xE00
#define FMU_FIELD_SMEN         0xF00
#define FMU_FIELD_SMERR        0xF04
#define FMU_FIELD_SMCR         0xF08
#define FMU_FIELD_STATUS       0xF1C
#define FMU_FIELD_KEY          0xF20
#define FMU_FIELD_ERRUPDATE    0xF28

#define FMU_ERRSTATUS_V_MASK            FWK_BIT(30)
#define FMU_ERRSTATUS_BLKID_MASK        FWK_GEN_MASK_64(43, 32)
#define FMU_ERRSTATUS_BLKID_SHIFT       32
#define FMU_ERRSTATUS_SMID_MASK         FWK_GEN_MASK(15, 8)
#define FMU_ERRSTATUS_SMID_SHIFT        8
#define FMU_STATUS_OFX_MASK             FWK_BIT_64(47)
#define FMU_ERRUPDATE_RECORD_PAIR_MASK  FWK_GEN_MASK(30, 28)
#define FMU_ERRUPDATE_RECORD_PAIR_SHIFT 28
#define FMU_ERRUPDATE_CRITICAL_MASK     FWK_BIT(2)
#define FMU_ERRUPDATE_NON_CRITICAL_MASK FWK_BIT(1)
#define FMU_STATUS_BUSY_MASK            FWK_BIT(0)
#define FMU_STATUS_TIMEOUT_MASK         FWK_BIT(1)
#define FMU_STATUS_BLKID_PWROFF_MASK    FWK_BIT(2)
#define FMU_STATUS_BLKID_ERR_MASK       FWK_BIT(3)
#define FMU_STATUS_PROTID_ERR_MASK      FWK_BIT(4)
#define FMU_SMEN_BLKTYPE_MASK           FWK_GEN_MASK(30, 28)
#define FMU_SMEN_BLKTYPE_SHIFT          28
#define FMU_SMEN_BLKID_MASK             FWK_GEN_MASK(27, 16)
#define FMU_SMEN_BLKID_SHIFT            16
#define FMU_SMEN_SMID_MASK              FWK_GEN_MASK(15, 8)
#define FMU_SMEN_SMID_SHIFT             8

#define FMU_SMEN_ENABLED   0x1
#define FMU_SMCR_CRITICAL  0x1
#define FMU_SYS_KEY_UNLOCK 0xBE

static fwk_id_t timer_id;
static struct mod_timer_api *timer_api;

static inline void fmu_write_32(
    uintptr_t base,
    uintptr_t offset,
    uint32_t value)
{
    fwk_mmio_write_32(base + FMU_FIELD_KEY, FMU_SYS_KEY_UNLOCK);
    fwk_mmio_write_32(base + offset, value);
}

static inline void fmu_write_64(
    uintptr_t base,
    uintptr_t offset,
    uint64_t value)
{
    fwk_mmio_write_32(base + FMU_FIELD_KEY, FMU_SYS_KEY_UNLOCK);
    fwk_mmio_write_64(base + offset, value);
}

static bool fmu_busy(void *data)
{
    const struct mod_fmu_dev_config *config = data;

    fwk_assert(config != NULL);

    return (fwk_mmio_read_32(config->base + FMU_FIELD_STATUS) &
            FMU_STATUS_BUSY_MASK) == 0;
}

static int fmu_device_wait_busy(const struct mod_fmu_dev_config *config)
{
    uint32_t status;

    fwk_assert(config != NULL);

    /* Poll while busy */
    timer_api->wait(timer_id, FWK_MS(100), fmu_busy, (void *)config);

    status = fwk_mmio_read_32(config->base + FMU_FIELD_STATUS);

    /* Inspect the resultant status */
    if (status & FMU_STATUS_TIMEOUT_MASK) {
        FWK_LOG_ERR(MOD_NAME "FMU transaction timed out\n");
        return FWK_E_TIMEOUT;
    } else if (status & FMU_STATUS_BLKID_PWROFF_MASK) {
        FWK_LOG_ERR(MOD_NAME "FMU block powered off\n");
        return FWK_E_DEVICE;
    } else if (status & FMU_STATUS_BLKID_ERR_MASK) {
        FWK_LOG_ERR(MOD_NAME "Invalid FMU block ID\n");
        return FWK_E_PARAM;
    } else if (status & FMU_STATUS_PROTID_ERR_MASK) {
        FWK_LOG_ERR(MOD_NAME "Invalid FMU protection ID\n");
        return FWK_E_PARAM;
    }

    return FWK_SUCCESS;
}

static int fmu_device_update(
    const struct mod_fmu_dev_config *config,
    uint32_t record_id)
{
    fwk_assert(config != NULL);

    uint32_t errupdate = (record_id << FMU_ERRUPDATE_RECORD_PAIR_SHIFT) |
        ((record_id % 2 == 0) ? FMU_ERRUPDATE_CRITICAL_MASK :
                                FMU_ERRUPDATE_NON_CRITICAL_MASK);

    fmu_write_32(config->base, errupdate, FMU_FIELD_ERRUPDATE);
    return fmu_device_wait_busy(config);
}

static bool fault_peek(
    const struct mod_fmu_dev_config *config,
    unsigned int *node_idx)
{
    uint64_t errgsr, status;
    uint32_t record_id;

    fwk_assert(config != NULL);

    errgsr = fwk_mmio_read_64(config->base + FMU_FIELD_ERRGSR);
    /* Check if there is a record */
    if (errgsr == 0) {
        return false;
    }

    record_id = fwk_math_log2(LSB_GET(errgsr));
    status = fwk_mmio_read_64(config->base + FMU_FIELD_ERRSTATUS(record_id));
    /* Check for spurious fault */
    if ((status & FMU_ERRSTATUS_V_MASK) == 0) {
        return false;
    }
    return true;
}

static void fault_ack(
    const struct mod_fmu_dev_config *config,
    struct mod_fmu_fault *fault,
    unsigned int node_idx,
    bool *fault_tracked)
{
    uint64_t errgsr, status;
    uint32_t record_id, blktype, smid;

    fwk_assert(config != NULL);
    fwk_assert(fault != NULL);
    fwk_assert(fault_tracked != NULL);

    errgsr = fwk_mmio_read_64(config->base + FMU_FIELD_ERRGSR);
    record_id = fwk_math_log2(LSB_GET(errgsr));
    status = fwk_mmio_read_64(config->base + FMU_FIELD_ERRSTATUS(record_id));
    status &= ~FMU_ERRSTATUS_V_MASK;
    fmu_write_64(config->base, FMU_FIELD_ERRSTATUS(record_id), status);
    fmu_device_wait_busy(config);

    /* 2 error records per BLKTYPE, one critical and one non-critical */
    blktype = record_id / 2;
    smid = (status & FMU_ERRSTATUS_SMID_MASK) >> FMU_ERRSTATUS_SMID_SHIFT;
    if (!(*fault_tracked)) {
        fault->node_idx = blktype;
        fault->sm_idx = smid;
        *fault_tracked = true;
    }

    /* Check if error buffer has overflowed and should be updated */
    if (status & FMU_STATUS_OFX_MASK) {
        fmu_device_update(config, record_id);
    }
}

static int configure(const struct mod_fmu_config *config)
{
    fwk_assert(config != NULL);

    timer_id = config->timer_id;

    return FWK_SUCCESS;
}

static int bind(fwk_id_t id)
{
    int status;

    status = fwk_module_bind(
        timer_id,
        FWK_ID_API(FWK_MODULE_IDX_TIMER, MOD_TIMER_API_IDX_TIMER),
        &timer_api);
    if (status != FWK_SUCCESS) {
        fwk_unexpected();
        return status;
    }

    return FWK_SUCCESS;
}

/*
 * API Handlers
 */
static int inject(
    const struct mod_fmu_dev_config *config,
    const struct mod_fmu_fault *fault)
{
    uint32_t prot_id;

    fwk_assert(config != NULL);
    fwk_assert(fault != NULL);

    prot_id =
        ((fault->node_idx << FMU_SMEN_BLKTYPE_SHIFT) & FMU_SMEN_BLKTYPE_MASK) |
        (fault->sm_idx << FMU_SMEN_SMID_SHIFT);

    fmu_write_32(config->base, FMU_FIELD_SMERR, prot_id);

    return fmu_device_wait_busy(config);
}

static int set_enabled(
    const struct mod_fmu_dev_config *config,
    const struct mod_fmu_fault *fault,
    bool enabled)
{
    uint32_t prot_id;

    fwk_assert(config != NULL);
    fwk_assert(fault != NULL);

    prot_id =
        ((fault->node_idx << FMU_SMEN_BLKTYPE_SHIFT) & FMU_SMEN_BLKTYPE_MASK) |
        (fault->sm_idx << FMU_SMEN_SMID_SHIFT);

    if (enabled) {
        prot_id |= FMU_SMEN_ENABLED;
    } else {
        prot_id &= ~FMU_SMEN_ENABLED;
    }

    fmu_write_32(config->base, FMU_FIELD_SMEN, prot_id);

    return fmu_device_wait_busy(config);
}

static int set_critical(
    const struct mod_fmu_dev_config *config,
    const struct mod_fmu_fault *fault,
    bool critical)
{
    uint32_t prot_id;

    fwk_assert(config != NULL);
    fwk_assert(fault != NULL);

    prot_id =
        ((fault->node_idx << FMU_SMEN_BLKTYPE_SHIFT) & FMU_SMEN_BLKTYPE_MASK) |
        (fault->sm_idx << FMU_SMEN_SMID_SHIFT);

    if (critical) {
        prot_id |= FMU_SMCR_CRITICAL;
    } else {
        prot_id &= ~FMU_SMCR_CRITICAL;
    }

    fmu_write_32(config->base, FMU_FIELD_SMCR, prot_id);

    return fmu_device_wait_busy(config);
}

struct mod_fmu_impl_api mod_fmu_gic_mhu_api = {
    .configure = configure,
    .bind = bind,
    .fault_peek = fault_peek,
    .fault_ack = fault_ack,
    .inject = inject,
    .set_enabled = set_enabled,
    .set_critical = set_critical,
};
