/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOD_POSIX_MQUEUE_H
#define MOD_POSIX_MQUEUE_H

#include <mqueue.h>

#include <fwk_id.h>

#include <signal.h>
#include <stddef.h>
#include <stdint.h>

/*!
 * \addtogroup GroupModules Modules
 * @{
 */

/*!
 * \defgroup GroupMQueue MQueue
 *
 * \brief Driver for one of the IPC mechanisms MQUEUE
 * @{
 */

/*!
 * \brief Configuration of a mqueue.
 */
struct mod_posix_mqueue_queue_config {
    /*! Path of the MQueue */
    const char *mqueue_pathname;
    /*! Flag indicates direction of the queue */
    const bool receive;
    /*! Max MSG size */
    unsigned int max_msg_size;
    /*! Max number of msg in the queue */
    unsigned int max_msg_num;
    /*! MQueue IRQ number */
    unsigned int irq;
    /*! MQueue IRQ posix signal number */
    int posix_signo;
};

/*!
 * \brief Posix MQueue driver APIs.
 */
enum mod_posix_mqueue_driver_api_idx {
    MOD_POSIX_MQUEUE_API_IDX_DRIVER,
    MOD_POSIX_MQUEUE_API_IDX_COUNT,
};

/*!
 * @}
 */

/*!
 * @}
 */

#endif /* MOD_POSIX_MQUEUE_H */
