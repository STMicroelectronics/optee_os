/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef NI_710AE_DRV_H
#define NI_710AE_DRV_H

#include <stdint.h>

/* Domains */
#define NI710AE_NODE_TYPE_GCN (0x00u)
#define NI710AE_NODE_TYPE_VD  (0x01u)
#define NI710AE_NODE_TYPE_PD  (0x02u)
#define NI710AE_NODE_TYPE_CD  (0x03u)

/* Components */
#define NI710AE_NODE_TYPE_ASNI  (0x04u)
#define NI710AE_NODE_TYPE_AMNI  (0x05u)
#define NI710AE_NODE_TYPE_PMU   (0x06u)
#define NI710AE_NODE_TYPE_HSNI  (0x07u)
#define NI710AE_NODE_TYPE_HMNI  (0x08u)
#define NI710AE_NODE_TYPE_PMNI  (0x09u)
#define NI710AE_NODE_TYPE_CC    (0x40u)
#define NI710AE_NODE_TYPE_PC    (0x41u)
#define NI710AE_NODE_TYPE_CFGNI (0x60u)
#define NI710AE_NODE_TYPE_FMU   (0x61u)

/* Subfeatures */
#define NI710AE_NODE_TYPE_SUBFEATURE_APU       (0x00u)
#define NI710AE_NODE_TYPE_SUBFEATURE_SAM       (0x01u)
#define NI710AE_NODE_TYPE_SUBFEATURE_FCU       (0x02u)
#define NI710AE_NODE_TYPE_SUBFEATURE_IDM       (0x03u)
#define NI710AE_NODE_TYPE_SUBFEATURE_RAS       (0x04u)
#define NI710AE_NODE_TYPE_SUBFEATURE_MAX_LIMIT (0x05u)

#endif /* NI_710AE_DRV_H */
