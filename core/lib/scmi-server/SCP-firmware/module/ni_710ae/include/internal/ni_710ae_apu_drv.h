/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef NI_710AE_APU_DRV_H
#define NI_710AE_APU_DRV_H

#include "ni_710ae_drv.h"

#include <fwk_status.h>

#include <stdint.h>

/*!
 * \brief NI-710AE APU device structure
 */
struct ni710ae_apu_dev_t {
    const uintptr_t base;
};

/*!
 * \brief NI-710AE APU background type enumerations
 */
enum ni710ae_apu_br_type_t { NCI_FOREGROUND = 0x0, NCI_BACKGROUND = 0x1 };

/*!
 * \brief NI-710AE APU lock type enumerations
 */
enum ni710ae_apu_lock_type_t {
    /* program specific values */
    NCI_UNLOCK = 0x0,
    NCI_LOCK = 0x1,
};

/*!
 * \brief NI-710AE APU region enable type enumerations
 */
enum ni710ae_apu_region_enable_type_t {
    /* program specific values */
    NCI_REGION_DISABLE = 0x0,
    NCI_REGION_ENABLE = 0x1,
};

struct ni710ae_apu_region {
    uint8_t region;
    uint64_t base_addr;
    uint64_t end_addr;
    enum ni710ae_apu_br_type_t background;
    const uint8_t *permissions;
    const uint8_t *entity_ids;
    uint8_t id_valid;
    enum ni710ae_apu_region_enable_type_t region_enable;
    enum ni710ae_apu_lock_type_t lock;
};

/*!
 * \brief NI-710AE APU access permission defines
 */
/* As per spec */
#define NCI_N_SEC_W 0x01U
#define NCI_SEC_W   0x02U
#define NCI_N_SEC_R 0x04U
#define NCI_SEC_R   0x08U

#define NCI_N_SEC_RW (NCI_N_SEC_R | NCI_N_SEC_W)
#define NCI_SEC_RW   (NCI_SEC_R | NCI_SEC_W)

#define NCI_ALL_PERM (NCI_N_SEC_RW | NCI_SEC_RW)

#define NCI_ID_PERMISSION_UNASSIGNED 0xFU

/*!
 * \brief NI-710AE APU entity selection defines
 */
enum nci_id_select {
    NCI_ID_0_SELECT = 0x0u,
    NCI_ID_1_SELECT = 0x1u,
    NCI_ID_2_SELECT = 0x2u,
    NCI_ID_3_SELECT = 0x3u,

    NCI_MAX_NUMBER_OF_ID = 0x4u,
    NCI_ID_UNASSIGNED = 0xFFu,
};

/*!
 * \brief NI-710AE APU entity valid defines
 */
/* program specific values */
#define NCI_ID_VALID_NONE 0x0U
#define NCI_ID_0_VALID    0x1U
#define NCI_ID_1_VALID    0x2U
#define NCI_ID_2_VALID    0x4U
#define NCI_ID_3_VALID    0x8U

/*!
 * \brief Enables NI-710AE APU
 *
 * \param[in] dev           NI-710AE APU device struct \ref ni710ae_apu_dev_t
 *
 * \retval ::FWK_SUCCESS Operation succeeded.
 * \retval ::FWK_E_PARAM One or more parameters were invalid.
 */
int ni710ae_apu_enable(struct ni710ae_apu_dev_t *dev);

/*!
 * \brief Enables NI-710AE APU SLVERR response.
 *
 * \param[in] dev           NI-710AE APU device struct \ref ni710ae_apu_dev_t
 *
 * \retval ::FWK_SUCCESS Operation succeeded.
 * \retval ::FWK_E_PARAM One or more parameters were invalid.
 */
int ni710ae_apu_sync_err_enable(struct ni710ae_apu_dev_t *dev);

/*!
 * \brief Initialize NI-710AE APU
 *
 * \param[in] dev        NI-710AE APU device struct \ref ni710ae_apu_dev_t
 * \param[in] cfg        APU region configuration
 * \retval ::FWK_SUCCESS Operation succeeded.
 * \retval ::FWK_E_PARAM One or more parameters were invalid.
 */
int ni710ae_apu_initialize_region(
    struct ni710ae_apu_dev_t *dev,
    const struct ni710ae_apu_region *cfg);

#endif /* NI_710AE_APU_DRV_H */
