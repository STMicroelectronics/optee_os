/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <mod_posix_mqueue.h>

#include <fwk_element.h>
#include <fwk_id.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>

#include <fmw_posix_scp.h>

static const struct fwk_element
    mqueue[] = {
        [0] = {
            .name = "OSPM",
            .data = &(struct mod_posix_mqueue_queue_config){
                    .mqueue_pathname = "/OSPM2P",
                    .receive = true,
                    .max_msg_num = 10,
                    .max_msg_size = 512,
                    .irq = PosixMqueue0_IRQ,
                    .posix_signo = SIGUSR1,
                },
        },
        [1] = {
            .name = "P2OSPM",
            .data = &(struct mod_posix_mqueue_queue_config){
                    .mqueue_pathname = "/P2OSPM",
                    .receive = false,
                    .max_msg_num = 10,
                    .max_msg_size = 512,
                    .irq = PosixMqueue1_IRQ,
                    .posix_signo = SIGUSR1,
                },
        },
        [2] = {0},
    };

const struct fwk_module_config config_posix_mqueue = {
    .elements = FWK_MODULE_STATIC_ELEMENTS_PTR(mqueue),
};
