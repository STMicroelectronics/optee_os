/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "ni_710ae_apu_drv.h"
#include "ni_710ae_apu_reg.h"

#include <fwk_assert.h>

#include <stddef.h>

/* PRID Fields */
/* ID Value Fields */
#define PRID_ID_MASK    (0xFFU)
#define PRID_ID0_OFFSET (0U)
#define PRID_ID1_OFFSET (16U)
#define PRID_ID2_OFFSET (0U)
#define PRID_ID3_OFFSET (16U)

/* Permission Fields */
#define PRID_PERM_MASK       (0x0FU)
#define PRID_PERM_ID0_OFFSET (8U)
#define PRID_PERM_ID1_OFFSET (24U)
#define PRID_PERM_ID2_OFFSET (8U)
#define PRID_PERM_ID3_OFFSET (24U)

/* PRBAR Fields */
#define PRBAR_BASEADDR_MASK      (0xFFFFFFC0U)
#define PRBAR_REGION_ENABLE_MASK (1U)
#define PRBAR_BACKGROUND_MASK    (1U << 1)
#define PRBAR_LOCK_MASK          (1U << 2)

/* PRLAR Fields */
#define PRLAR_ENDADDR_MASK    (0xFFFFFFC0U)
#define PRLAR_ID_VALID_MASK   (0xFU)
#define PRLAR_ID_VALID_OFFSET (0U)

/* APU_CTLR Fields */
#define APU_CTLR_ENABLE_MASK   (1U) /* Bit 0: APU enable */
#define APU_CTLR_SYNC_ERR_MASK (1U << 2) /* Bit 2: Sync error enable*/

enum bit_action_t { CLEAR_BIT = 0, SET_BIT = 1 };

/*
 * APU Programming
 */

/**
 * \brief Set a bitfield in a 32-bit hardware register.
 *
 * This function performs a read-modify-write on a 32-bit register to update
 * a specific field defined by a bitmask and offset.
 *
 * \param[in,out] reg     Pointer to the register to modify.
 * \param[in]     mask    Bitmask for the field to modify (unshifted).
 * \param[in]     offset  Bit offset of the field within the register.
 * \param[in]     value   New value to write to the field (unshifted).
 *
 * \retval ::FWK_SUCCESS Operation succeeded.
 * \retval ::FWK_E_PARAM One or more parameters were invalid.
 */
static inline int set_register_bitfield(
    volatile uint32_t *reg,
    uint32_t mask,
    uint32_t offset,
    uint32_t value)
{
    if (reg == NULL) {
        return FWK_E_PARAM;
    }

    uint32_t reg_value = *reg;

    reg_value &= ~(mask << offset);
    reg_value |= (value & mask) << offset;

    *reg = reg_value;

    return FWK_SUCCESS;
}

/**
 * \brief Conditionally sets or clears a bit in a register.
 *
 * \param[in,out] reg    Pointer to the target register.
 * \param[in]     mask   Bitmask of the bit to modify.
 * \param[in]     action Action -> SET_BIT or CLEAR_BIT.
 *
 * \retval ::FWK_SUCCESS Operation succeeded.
 * \retval ::FWK_E_PARAM One or more parameters were invalid.
 */
static inline int modify_register_bit(
    volatile uint32_t *reg,
    uint32_t mask,
    enum bit_action_t action)
{
    if (reg == NULL) {
        return FWK_E_PARAM;
    }

    if (action == SET_BIT) {
        *reg |= mask;
    } else if (action == CLEAR_BIT) {
        *reg &= ~mask;
    } else {
        return FWK_E_PARAM;
    }

    return FWK_SUCCESS;
}

/**
 * \brief Sets NI-710AE APU address range,
 *
 * \param[in] dev           NI-710AE APU device struct \ref ni710ae_apu_dev_t
 * \param[in] region        APU region number to be initialized
 * \param[in] base_addr     Base address of the region
 * \param[in] end_addr      End address of the region
 *
 * \retval ::FWK_SUCCESS Operation succeeded.
 * \retval ::FWK_E_PARAM One or more parameters were invalid.
 */
static int ni710ae_apu_set_addr_range(
    struct ni710ae_apu_dev_t *dev,
    uint8_t region,
    uint64_t base_addr,
    uint64_t end_addr)
{
    struct ni710ae_apu_reg_map_t *reg;

    if ((dev == NULL) || (dev->base == (uintptr_t)NULL) ||
        (region >= NI_710AE_MAX_APU_REGIONS)) {
        return FWK_E_PARAM;
    }

    reg = (struct ni710ae_apu_reg_map_t *)dev->base;

    /* Set base address */
    reg->region[region].prbar_high = (uint32_t)(base_addr >> 32U);
    reg->region[region].prbar_low = (uint32_t)(PRBAR_BASEADDR_MASK & base_addr);

    /* Set end address */
    reg->region[region].prlar_high = (uint32_t)(end_addr >> 32U);
    reg->region[region].prlar_low = (uint32_t)(PRLAR_ENDADDR_MASK & end_addr);

    return FWK_SUCCESS;
}

/**
 * \brief Sets NI-710AE APU access permissions
 *
 * \param[in] dev           NI-710AE APU device struct \ref ni710ae_apu_dev_t
 * \param[in] region        APU region number to be initialized
 * \param[in] permission    Or'ing of required access permissions
 * \param[in] id_select     Specify entity id number
 *
 * \retval ::FWK_SUCCESS Operation succeeded.
 * \retval ::FWK_E_PARAM One or more parameters were invalid.
 */
static int ni710ae_apu_set_access_perms(
    struct ni710ae_apu_dev_t *dev,
    uint8_t region,
    uint8_t permission,
    enum nci_id_select id_select)
{
    struct ni710ae_apu_reg_map_t *reg;
    int status = FWK_SUCCESS;

    if ((dev == NULL) || (dev->base == (uintptr_t)NULL)) {
        return FWK_E_PARAM;
    }

    if (permission == NCI_ID_PERMISSION_UNASSIGNED) {
        return FWK_SUCCESS;
    }

    reg = (struct ni710ae_apu_reg_map_t *)dev->base;

    fwk_assert(region < NI_710AE_MAX_APU_REGIONS);

    switch (id_select) {
    case NCI_ID_0_SELECT:
        status = set_register_bitfield(
            &reg->region[region].prid_low,
            PRID_PERM_MASK,
            PRID_PERM_ID0_OFFSET,
            permission);
        break;
    case NCI_ID_1_SELECT:
        status = set_register_bitfield(
            &reg->region[region].prid_low,
            PRID_PERM_MASK,
            PRID_PERM_ID1_OFFSET,
            permission);
        break;
    case NCI_ID_2_SELECT:
        status = set_register_bitfield(
            &reg->region[region].prid_high,
            PRID_PERM_MASK,
            PRID_PERM_ID2_OFFSET,
            permission);
        break;
    case NCI_ID_3_SELECT:
        status = set_register_bitfield(
            &reg->region[region].prid_high,
            PRID_PERM_MASK,
            PRID_PERM_ID3_OFFSET,
            permission);
        break;
    default:
        status = FWK_E_PARAM;
        break;
    }

    return status;
}

/**
 * \brief Sets NI-710AE APU lock
 *
 * \param[in] dev           NI-710AE APU device struct \ref ni710ae_apu_dev_t
 * \param[in] region        APU region number to be initialized
 * \param[in] lock          Lock or Unlock an APU region \ref
 * ni710ae_apu_lock_type_t
 *
 * \retval ::FWK_SUCCESS Operation succeeded.
 * \retval ::FWK_E_PARAM One or more parameters were invalid.
 */
static int ni710ae_apu_set_lock(
    struct ni710ae_apu_dev_t *dev,
    uint8_t region,
    enum ni710ae_apu_lock_type_t lock)
{
    struct ni710ae_apu_reg_map_t *reg;
    int status = FWK_SUCCESS;

    if ((dev == NULL) || (dev->base == (uintptr_t)NULL)) {
        return FWK_E_PARAM;
    }
    reg = (struct ni710ae_apu_reg_map_t *)dev->base;

    fwk_assert(region < NI_710AE_MAX_APU_REGIONS);

    if (lock == NCI_LOCK) {
        status = modify_register_bit(
            &reg->region[region].prbar_low, PRBAR_LOCK_MASK, SET_BIT);
    } else {
        status = modify_register_bit(
            &reg->region[region].prbar_low, PRBAR_LOCK_MASK, CLEAR_BIT);
    }

    return status;
}

/**
 * \brief Sets NI-710AE APU background/foreground type for a region
 *
 * \param[in] dev           NI-710AE APU device struct \ref ni710ae_apu_dev_t
 * \param[in] region        APU region number to be initialized
 * \param[in] background    Specify if the region is backgroun or a foreground
 *                          region \ref ni710ae_apu_br_type_t
 *
 * \retval ::FWK_SUCCESS Operation succeeded.
 * \retval ::FWK_E_PARAM One or more parameters were invalid.
 */
static int ni710ae_apu_set_br(
    struct ni710ae_apu_dev_t *dev,
    uint8_t region,
    enum ni710ae_apu_br_type_t background)
{
    struct ni710ae_apu_reg_map_t *reg;
    int status = FWK_SUCCESS;

    if ((dev == NULL) || (dev->base == (uintptr_t)NULL)) {
        return FWK_E_PARAM;
    }
    reg = (struct ni710ae_apu_reg_map_t *)dev->base;

    fwk_assert(region < NI_710AE_MAX_APU_REGIONS);

    if (background == NCI_BACKGROUND) {
        status = modify_register_bit(
            &reg->region[region].prbar_low, PRBAR_BACKGROUND_MASK, SET_BIT);
    } else {
        status = modify_register_bit(
            &reg->region[region].prbar_low, PRBAR_BACKGROUND_MASK, CLEAR_BIT);
    }

    return status;
}

/**
 * \brief Enables NI-710AE APU region
 *
 * \param[in] dev           NI-710AE APU device struct \ref ni710ae_apu_dev_t
 * \param[in] region        APU region number to be initialized
 * \param[in] region_enable Specify if the region needs to be enabled or not.
 *                          \ref ni710ae_apu_region_enable_type_t
 *
 * \retval ::FWK_SUCCESS Operation succeeded.
 * \retval ::FWK_E_PARAM One or more parameters were invalid.
 */
static int ni710ae_apu_set_region_enable(
    struct ni710ae_apu_dev_t *dev,
    uint8_t region,
    enum ni710ae_apu_region_enable_type_t region_enable)
{
    struct ni710ae_apu_reg_map_t *reg;
    int status = FWK_SUCCESS;

    if ((dev == NULL) || (dev->base == (uintptr_t)NULL)) {
        return FWK_E_PARAM;
    }
    reg = (struct ni710ae_apu_reg_map_t *)dev->base;

    fwk_assert(region < NI_710AE_MAX_APU_REGIONS);

    if (region_enable == NCI_REGION_ENABLE) {
        status = modify_register_bit(
            &reg->region[region].prbar_low, PRBAR_REGION_ENABLE_MASK, SET_BIT);
    } else {
        status = modify_register_bit(
            &reg->region[region].prbar_low,
            PRBAR_REGION_ENABLE_MASK,
            CLEAR_BIT);
    }

    return status;
}

/**
 * \brief Sets NI-710AE APU id_valid for a region
 *
 * \param[in] dev           NI-710AE APU device struct \ref ni710ae_apu_dev_t
 * \param[in] region        APU region number to be initialized
 * \param[in] valid         Or'ing of entity id's where these ids are valid
 *
 * \retval ::FWK_SUCCESS Operation succeeded.
 * \retval ::FWK_E_PARAM One or more parameters were invalid.
 */
static int ni710ae_apu_set_id_valid(
    struct ni710ae_apu_dev_t *dev,
    uint8_t region,
    uint32_t valid)
{
    struct ni710ae_apu_reg_map_t *reg;
    int status = FWK_SUCCESS;

    if ((dev == NULL) || (dev->base == (uintptr_t)NULL)) {
        return FWK_E_PARAM;
    }
    reg = (struct ni710ae_apu_reg_map_t *)dev->base;

    fwk_assert(region < NI_710AE_MAX_APU_REGIONS);

    status = set_register_bitfield(
        &reg->region[region].prlar_low,
        PRLAR_ID_VALID_MASK,
        PRLAR_ID_VALID_OFFSET,
        valid);

    return status;
}

int ni710ae_apu_enable(struct ni710ae_apu_dev_t *dev)
{
    struct ni710ae_apu_reg_map_t *reg;
    int status = FWK_SUCCESS;

    if ((dev == NULL) || (dev->base == (uintptr_t)NULL)) {
        return FWK_E_PARAM;
    }
    reg = (struct ni710ae_apu_reg_map_t *)dev->base;

    status = modify_register_bit(&reg->apu_ctlr, APU_CTLR_ENABLE_MASK, SET_BIT);

    return status;
}

int ni710ae_apu_sync_err_enable(struct ni710ae_apu_dev_t *dev)
{
    struct ni710ae_apu_reg_map_t *reg;
    int status = FWK_SUCCESS;

    if ((dev == NULL) || (dev->base == (uintptr_t)NULL)) {
        return FWK_E_PARAM;
    }
    reg = (struct ni710ae_apu_reg_map_t *)dev->base;

    status =
        modify_register_bit(&reg->apu_ctlr, APU_CTLR_SYNC_ERR_MASK, SET_BIT);

    return status;
}

/**
 * \brief Assign custom NI-710AE APU id values
 * \param[in] dev           NI-710AE APU device struct \ref ni710ae_apu_dev_t
 * \param[in] region        APU region number to be initialized
 * \param[in] id_value      New ID value
 * \param[in] id_select     Specify enitity id number
 *
 * \retval ::FWK_SUCCESS Operation succeeded.
 * \retval ::FWK_E_PARAM One or more parameters were invalid.
 */

static int ni710ae_apu_assign_id(
    struct ni710ae_apu_dev_t *dev,
    uint8_t region,
    uint8_t id_value,
    enum nci_id_select id_select)
{
    struct ni710ae_apu_reg_map_t *reg;
    int status = FWK_SUCCESS;

    if ((dev == NULL) || (dev->base == (uintptr_t)NULL)) {
        return FWK_E_PARAM;
    }

    if (id_value == NCI_ID_UNASSIGNED) {
        return FWK_SUCCESS;
    }

    reg = (struct ni710ae_apu_reg_map_t *)dev->base;

    fwk_assert(region < NI_710AE_MAX_APU_REGIONS);

    switch (id_select) {
    case NCI_ID_0_SELECT:
        status = set_register_bitfield(
            &reg->region[region].prid_low,
            PRID_ID_MASK,
            PRID_ID0_OFFSET,
            id_value);
        break;
    case NCI_ID_1_SELECT:
        status = set_register_bitfield(
            &reg->region[region].prid_low,
            PRID_ID_MASK,
            PRID_ID1_OFFSET,
            id_value);
        break;
    case NCI_ID_2_SELECT:
        status = set_register_bitfield(
            &reg->region[region].prid_high,
            PRID_ID_MASK,
            PRID_ID2_OFFSET,
            id_value);
        break;
    case NCI_ID_3_SELECT:
        status = set_register_bitfield(
            &reg->region[region].prid_high,
            PRID_ID_MASK,
            PRID_ID3_OFFSET,
            id_value);
        break;
    default:
        status = FWK_E_PARAM;
        break;
    }

    return status;
}

int ni710ae_apu_initialize_region(
    struct ni710ae_apu_dev_t *dev,
    const struct ni710ae_apu_region *cfg)
{
    int status;

    if ((dev == NULL) || (cfg == NULL) || (cfg->permissions == NULL) ||
        (cfg->entity_ids == NULL)) {
        return FWK_E_PARAM;
    }

    status = ni710ae_apu_set_addr_range(
        dev, cfg->region, cfg->base_addr, cfg->end_addr);
    if (status != FWK_SUCCESS)
        return status;

    status = ni710ae_apu_set_br(dev, cfg->region, cfg->background);
    if (status != FWK_SUCCESS)
        return status;

    status = ni710ae_apu_set_id_valid(dev, cfg->region, cfg->id_valid);
    if (status != FWK_SUCCESS)
        return status;

    for (uint32_t id_idx = 0U; id_idx < NCI_MAX_NUMBER_OF_ID; ++id_idx) {
        status = ni710ae_apu_set_access_perms(
            dev, cfg->region, cfg->permissions[id_idx], id_idx);
        if (status != FWK_SUCCESS)
            return status;

        status = ni710ae_apu_assign_id(
            dev, cfg->region, cfg->entity_ids[id_idx], id_idx);
        if (status != FWK_SUCCESS)
            return status;
    }

    status =
        ni710ae_apu_set_region_enable(dev, cfg->region, cfg->region_enable);
    if (status != FWK_SUCCESS)
        return status;

    return ni710ae_apu_set_lock(dev, cfg->region, cfg->lock);
}
