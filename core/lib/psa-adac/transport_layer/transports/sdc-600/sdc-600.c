/*
 * Copyright (c) 2020-2023 Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "psa_adac_debug.h"
#include "platform/msg_interface.h"
#include "static_buffer_msg.h"
#include "int_com_port_driver.h"
#include "platform/platform.h"

#ifndef PSA_ADAC_AUTHENTICATOR_IMPLICIT_TRANSPORT
#error "Unsupported environment"
#endif

#ifndef SDC600_BASE
#define SDC600_BASE (0x50002000)
#endif
#define SDC600 ((SDC600_Type *)SDC600_BASE)

/* ! PSA Debug Access Control protocol ID: 'PSADBG' */
static const char APBCOM_ID[6] = {0x50, 0x53, 0x41, 0x44, 0x42, 0x47};

int psa_adac_detect_debug_request(void)
{
    int rc = 0;

    /* Initialize the IComPort driver. */
    icpd_rc_t r = IComPortInit((uint8_t *) APBCOM_ID, sizeof(APBCOM_ID));

    switch (r) {
        case ICPD_SUCCESS:
            rc = 1;
            PSA_ADAC_LOG_INFO("sdc-600", "IComPortInit: Success\r\n");
            break;

        case ICPD_DEBUGGER_NOT_CONNECTED:
            PSA_ADAC_LOG_INFO("sdc-600", "IComPortInit: Debugger not connected\r\n");
            break;

        case ICPD_FAIL:
            PSA_ADAC_LOG_INFO("sdc-600", "IComPortInit: Failure\r\n");
    }

    return rc;
}

void psa_adac_acknowledge_debug_request(void)
{
    /* Nothing */
}

int msg_interface_init(void *ctx, uint8_t buffer[], size_t buffer_size)
{
    return psa_adac_static_buffer_msg_init(buffer, buffer_size);
}

int msg_interface_free(void *ctx)
{
    return psa_adac_static_buffer_msg_release();
}

int request_packet_send(void)
{
    return -1;
}

request_packet_t *request_packet_receive(void *ctx)
{
    size_t max = 0, length = 0;
    request_packet_t *r = request_packet_lock(&max);
    if (r != NULL) {
        if (IComPortRx((uint8_t *) r, max, &length) == ICPD_RX_SUCCESS) {
            PSA_ADAC_LOG_DEBUG("request_packet_receive", "Received message of length %d\n", length);
            return r;
        }
        PSA_ADAC_LOG_DEBUG("request_packet_receive", "Error Receiving Request\n");
        request_packet_release(r);
    } else {
        PSA_ADAC_LOG_DEBUG("request_packet_receive", "Error locking Request\n");
    }
    return NULL;
}

int response_packet_send(response_packet_t *p)
{
    response_packet_t *packet = psa_adac_static_buffer_msg_get_response();
    return (IComPortTx((uint8_t *) packet,
                       sizeof(response_packet_t) + packet->data_count * sizeof(uint32_t),
                       NULL) == ICPD_TX_SUCCESS ? 0 : -1);
}

response_packet_t *response_packet_receive()
{
    return NULL;
}
