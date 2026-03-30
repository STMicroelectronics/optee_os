/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <mod_posix_mqueue.h>
#include <mod_posix_transport.h>

#include <fwk_element.h>
#include <fwk_id.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>

static const struct fwk_element
    element_table[] = {
        [0] = {
            .name = "OSPM_CHANNEL",
            .data = &(struct mod_posix_transport_channel_config){
                    .rx_dev.dev_driver_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_POSIX_MQUEUE, 0),
                    .rx_dev.dev_driver_api_id = FWK_ID_API_INIT(FWK_MODULE_IDX_POSIX_MQUEUE,MOD_POSIX_MQUEUE_API_IDX_DRIVER),
                    .tx_dev.dev_driver_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_POSIX_MQUEUE, 1),
                    .tx_dev.dev_driver_api_id = FWK_ID_API_INIT(FWK_MODULE_IDX_POSIX_MQUEUE,MOD_POSIX_MQUEUE_API_IDX_DRIVER),
                },
        },
        [1] = {0},
    };

const struct fwk_module_config config_posix_transport = {
    .elements = FWK_MODULE_STATIC_ELEMENTS_PTR(element_table),
};
