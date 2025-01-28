/*
 * Copyright (c) 2016-2019, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef INT_COM_PORT_DRIVER_H_
#define INT_COM_PORT_DRIVER_H_

#include <stdint.h>
#include <stddef.h>

typedef enum icpd_rc_t {
    ICPD_SUCCESS,
    ICPD_DEBUGGER_NOT_CONNECTED,
    ICPD_FAIL,
} icpd_rc_t;

typedef enum icpd_rx_rc_t {
    ICPD_RX_SUCCESS,
    ICPD_RX_PROT_ERROR,
    ICPD_RX_BUFFER_OVERFLOW,
    ICPD_RX_DISCONNECT,
    ICPD_RX_FAIL,
} icpd_rx_rc_t;

typedef enum icpd_tx_rc_t {
    ICPD_TX_SUCCESS,
    ICPD_TX_TIMEOUT,
    ICPD_TX_DISCONNECT,
    ICPD_TX_FAIL,
} icpd_tx_rc_t;

/**
 * This function is called by the Secure Debug Handler to initiate the Internal COM port
 * driver and the COM port link. The Internal COM port driver checks if there is any
 * RX symbol in the Internal COM Port FIFO to find if the debugger platform is connected
 * and check if link is established by the External COM Port (LPH2RA symbol in the Internal
 * COM Port FIFO).
 *
 * Once the driver detects LPH2RA, the driver transmits LPH2RA to the debugger, establishes
 * the link to the external COM port and waits for IDA command. Once received, the driver
 * responds with the high level protocol provided value.
 *
 * @param IDBuffer          [in]
 * @param IDBufferLength    [in]
 * @return
 */
icpd_rc_t IComPortInit(uint8_t *IDBuffer, size_t IDBufferLength);

/**
 * At its receive side, the Internal COM port driver receives from the SDC-600 Internal
 * COM port receiver a protocol message (which is stuffed by the required ESC flag bytes)
 * that starts with START of message and ends with END of message flags. The receive side
 * of the driver strips off the START and END of message flags and writes just the message
 * content to the buffer it received from the Secure Debug Handler. While receiving, when
 * the receiver detects ESC flag it drops it and replace the following received byte to
 * its original value by flipping the MS bit.
 *
 * In case the receiver detects a message that does not start with START it
 *     drops it and reports an RX error.
 * In case the receiver detects a LPH2RL flag it drops it and reports link
 *     dropper RX error.
 * In case the receiver detects a message that starts with START but it filled
 *     the receive buffer to its end prior to the detection of END, it drops it
 *     and reports an RX error.
 *
 * @param RxBuffer          [out]
 * @param RxBufferLength    [in]
 * @param ActualLength      [out]
 * @return
 */
icpd_rx_rc_t IComPortRx(uint8_t *RxBuffer, size_t RxBufferLength, size_t *ActualLength);

/**
 * The transmit side of the driver receives from the Secure Debug Handler a message
 * to transmit in the provided buffer. The transmitter is transparent to the caller
 * and the provided buffer may include any byte values (including values that are
 * SDC-600 flag bytes). The IComPortTx provides transparent transmit interface.
 *
 * The IComPortTx function is responsible to inject to the Internal COM Port transmit
 * FIFO HW a START flag, followed by the message bytes from the buffer and at the end
 * it is responsible to inject to the transmit HW an END flag. While transmitting any
 * byte from the buffer, the driver must detect if it is one of the SDC-600 COM Port
 * flag values (bytes with an upper 3 bits of b101 are classified as Flag bytes).
 * In such case the driver must inject ESC flag to the transmitter and flips the MS
 * bit of the byte and transmits the modified byte.
 * At the end of transmission of the buffer the IComPortTx function is responsible to
 * inject to the External COM Port transmit FIFO HW an END flag.
 *
 * @param TxBuffer          [in]
 * @param TxBufferLength    [in]
 * @param ActualLength      [out]
 * @return
 */
icpd_tx_rc_t IComPortTx(uint8_t *TxBuffer, size_t TxBufferLength, size_t *ActualLength);

#endif /* INT_COM_PORT_DRIVER_H_ */
