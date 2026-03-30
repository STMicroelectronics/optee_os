/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOD_POSIX_TRANSPORT_EXTRA
#define MOD_POSIX_TRANSPORT_EXTRA
#include <mod_posix_transport.h>

#include <fwk_id.h>

int posix_dev_get_message(
    struct mod_posix_transport_message *message,
    fwk_id_t device_id);

int posix_dev_send_message(
    struct mod_posix_transport_message *message,
    fwk_id_t device_id);

int listener_signal_message(fwk_id_t service_id);

#endif /* MOD_POSIX_TRANSPORT_EXTRA */
