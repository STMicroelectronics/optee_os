/*
 * Copyright (c) 2016-2019, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "sdc600_types.h"
#include "sdc600_log.h"

#include "int_com_port_driver.h"
#include "int_com_port_def.h"

/******************************************************************************************************
 *
 * Macros
 *
 ******************************************************************************************************/
#define ENTITY_NAME "ICPD"

#define BLOCKED         true
#define NOT_BLOCKED     false

#define SKIP_ERROR_CHECK    true
#define ERROR_CHECK         false

#define ICPD_ASSERT(_cmd, _exp, _error) \
                do { \
                    int _res = 0; \
                    if (SDC600_TRACE) SDC600_LOG_DEBUG(ENTITY_NAME, "running[%s]\n", #_cmd); \
                    if ((_res = (int)(_cmd)) != _exp) \
                    { \
                        SDC600_LOG_ERR(ENTITY_NAME, "failed to run[%s] res[%d]\n", #_cmd, _res); \
                        res = _error; \
                        goto bail; \
                    } \
                } while (0)

#define ICPD_ASSERT_PASS(_cmd, _exp) \
                do { \
                    int _res = 0; \
                    if (SDC600_TRACE) SDC600_LOG_DEBUG(ENTITY_NAME, "running[%s]\n", #_cmd); \
                    if ((_res = (int)(_cmd)) != _exp) \
                    { \
                        SDC600_LOG_ERR(ENTITY_NAME, "failed to run[%s] res[%d]\n", #_cmd, _res); \
                        goto bail; \
                    } \
                } while (0)

/******************************************************************************************************
 *
 * Static declarations
 *
 ******************************************************************************************************/
static icpd_rx_rc_t IComPortRxInt(uint8_t startFlag,
                                  uint8_t *RxBuffer,
                                  size_t RxBufferLength,
                                  size_t *ActualLength);

static icpd_rx_rc_t IComPortReadByte(uint8_t *byte, bool_t isBlocked, bool_t isSkipErrorsCheck);

static bool_t IComPortIsFlag(uint8_t buffer, uint8_t flag);

static icpd_tx_rc_t IComSendByte(uint8_t byte);

static icpd_tx_rc_t IComPortTxInt(uint8_t startFlag,
                                  uint8_t *TxBuffer,
                                  size_t TxBufferLength,
                                  size_t *actualLength);

/******************************************************************************************************
 *
 * variables
 *
 ******************************************************************************************************/
bool_t gIsComPortInited = false;

/******************************************************************************************************
 *
 * private
 *
 ******************************************************************************************************/
static icpd_rx_rc_t IComPortReadByte(uint8_t *byte, bool_t isBlocked, bool_t isSkipErrorsCheck) {
    icpd_rx_rc_t res = ICPD_RX_SUCCESS;

    bool_t txOverflow = false;
    uint8_t rxData = 0;
    uint32_t sr = 0;

    do {
        sr = APBCOM_READ_WORD(APBCOM_REG_SR);

        if (isSkipErrorsCheck == ERROR_CHECK) {

            // SDC600_LOG_DEBUG(ENTITY_NAME, "SR[0x%08x]\n", sr);

            txOverflow = APBCOM_GET_FEILD(sr, SR_TXOE_OFFSET, SR_TXOE_LEN) > 0;
            if (txOverflow) {
                SDC600_LOG_ERR(ENTITY_NAME, "buffer overflow detected [%u]\n", txOverflow);
                res = ICPD_RX_BUFFER_OVERFLOW;
                goto bail;
            }

        }

        rxData = APBCOM_GET_FEILD(sr, SR_RXF_OFFSET, SR_RXF_LEN);
        if (rxData > 0) {
            break;
        }

    } while (isBlocked);

    /* this driver is hard coded to work with fifo size 1 */
    *byte = APBCOM_READ_WORD(APBCOM_REG_DR) & 0xFF;

    bail:
    return res;
}

static bool_t IComPortIsFlag(uint8_t byte, uint8_t flag) {

    SDC600_LOG_DEBUG(ENTITY_NAME, "expecting flag[%s]\n", apbcomflagToStr(flag));

    if (byte == flag) {
        SDC600_LOG_DEBUG(ENTITY_NAME, "flag[%s] found\n", apbcomflagToStr(flag));
        return true;
    }

    SDC600_LOG_DEBUG(ENTITY_NAME, "flag[%s] not found\n", apbcomflagToStr(flag));

    return false;
}

static icpd_tx_rc_t IComSendByte(uint8_t byte) {
    icpd_tx_rc_t res = ICPD_TX_SUCCESS;
    uint8_t txFifoFreeSapceInBytes = 0;

    const uint32_t MAX_NUM_OF_RETRIES = 50000;
    uint32_t numOfRetries = 0;
    uint32_t sr = 0;


    while (numOfRetries++ < MAX_NUM_OF_RETRIES) {
        sr = APBCOM_READ_WORD(APBCOM_REG_SR);

        if (APBCOM_GET_FEILD(sr, SR_TXLE_OFFSET, SR_TXLE_LEN) > 0) {
            SDC600_LOG_ERR(ENTITY_NAME, "link error detected\n");
            res = ICPD_TX_DISCONNECT;
            goto bail;
        }

        if (APBCOM_GET_FEILD(sr, SR_TXOE_OFFSET, SR_TXOE_LEN) > 0) {
            SDC600_LOG_ERR(ENTITY_NAME, "overflow detected\n");
            res = ICPD_TX_FAIL;
            goto bail;
        }

        txFifoFreeSapceInBytes = APBCOM_GET_FEILD(sr, SR_TXS_OFFSET, SR_TXS_LEN);
        if (txFifoFreeSapceInBytes > 0) {
            break;
        }
    }

    if (numOfRetries >= MAX_NUM_OF_RETRIES) {
        SDC600_LOG_ERR(ENTITY_NAME, "write operation timed  out\n");
        res = ICPD_TX_TIMEOUT;
        goto bail;
    }

    uint8_t txData[4] = {byte, FLAG__NULL, FLAG__NULL, FLAG__NULL};
    APBCOM_WRITE_WORD(APBCOM_REG_DBR, txData);

    bail:
    return res;
}

static icpd_tx_rc_t IComPortTxInt(uint8_t startFlag, uint8_t *TxBuffer, size_t TxBufferLength, size_t *actualLength) {
    icpd_tx_rc_t res = ICPD_TX_SUCCESS;
    size_t input_index = 0, output_index = 0;

    SDC600_LOG_BUF("  <-----  ", TxBuffer, TxBufferLength, "data_to_send");

    res = IComSendByte(startFlag);
    if (res != ICPD_TX_SUCCESS) {
        SDC600_LOG_ERR(ENTITY_NAME, "failed to send byte num[%u][0x%02x]\n", output_index, startFlag);
        goto bail;
    }
    ++output_index;

    /* send all bytes */
    for (input_index = 0; input_index < TxBufferLength; ++input_index, ++output_index) {
        uint8_t current = TxBuffer[input_index];

        /* Each Message byte that matches one of the Flag bytes is
         * immediately preceded by the ESC Flag byte, and bit [7] of the Message byte is inverted. */
        if (current >= 0xA0 && current < 0xC0) {
            res = IComSendByte(FLAG_ESC);
            if (res != ICPD_TX_SUCCESS) {
                SDC600_LOG_ERR(ENTITY_NAME, "failed to send byte num[%u][0x%02x]\n", output_index, FLAG_ESC);
                goto bail;
            }
            ++output_index;
            current = current & ~0x80UL;
        }
        res = IComSendByte(current);
        if (res != ICPD_TX_SUCCESS) {
            SDC600_LOG_ERR(ENTITY_NAME, "failed to send byte num[%u][0x%02x]\n", output_index, current);
            goto bail;
        }
    }

    res = IComSendByte(FLAG_END);
    if (res != ICPD_TX_SUCCESS) {
        SDC600_LOG_ERR(ENTITY_NAME, "failed to send byte num[%u][0x%02x]\n", output_index, FLAG_END);
        goto bail;
    }
    ++output_index;

    if(actualLength !=NULL) {
        *actualLength = output_index;
    }

bail:
    return res;

}

static icpd_rx_rc_t IComPortRxInt(uint8_t startFlag, uint8_t *RxBuffer, size_t RxBufferLength, size_t *ActualLength) {
    icpd_rx_rc_t res = ICPD_RX_SUCCESS;

    uint8_t readByte = 0;
    bool_t isDone = false;
    uint16_t buffer_idx = 0;
    bool_t isStartRecv = false;
    bool_t isEndRecv = false;
    bool_t isEscRecv = false;

    ICPD_ASSERT(gIsComPortInited == true, true, ICPD_RX_FAIL);

    while (isDone == false) {
        /* if this fails it means the buffer is empty */
        res = IComPortReadByte(&readByte, BLOCKED, ERROR_CHECK);
        if (res != ICPD_RX_SUCCESS) {
            SDC600_LOG_ERR(ENTITY_NAME, "IComPortReadBuffer failed with code: %d\n", res);
            goto bail;
        }

        /* com port is 1 byte fifo width */
        if (readByte == FLAG_END) {
            isDone = true;
            isEndRecv = true;
            continue;
        } else if (readByte == FLAG__NULL) {
            continue;
        } else if (readByte == FLAG_ESC) {
            isEscRecv = true;
        } else if (readByte == startFlag) {
            buffer_idx = 0;
            isStartRecv = true;
        } else {
            if (isEscRecv == true) {
                readByte |= 0x80UL;
                isEscRecv = false;
            }

            if (buffer_idx >= RxBufferLength) {
                SDC600_LOG_ERR(ENTITY_NAME, "RxBufferLength[%u] buffer_idx[%u]\n", (uint32_t) RxBufferLength,
                               buffer_idx);
                res = ICPD_RX_BUFFER_OVERFLOW;
                goto bail;
            }

            RxBuffer[buffer_idx] = readByte;

            buffer_idx += 1;
            *ActualLength = buffer_idx;
        }
    }

    SDC600_LOG_BUF("  ----->  ", RxBuffer, *ActualLength, "data_recv");

    bail:

    if (res == ICPD_RX_SUCCESS) {
        if (isStartRecv ^ isEndRecv)
            res = ICPD_RX_PROT_ERROR;
    }

    return res;

}

/******************************************************************************************************
 *
 * public
 *
 ******************************************************************************************************/
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
icpd_rc_t IComPortInit(uint8_t *IDBuffer, size_t IDBufferLength) {
    icpd_rc_t res = ICPD_SUCCESS;
    uint8_t readByte;

#ifdef PROFILE_ENABLED
    ICPD_ASSERT(IDBuffer != NULL, true, ICPD_FAIL);

    {
        volatile uint32_t *SYST_CSR = 0xE000E010;
        volatile uint32_t *SYST_RVR = 0xE000E014;
        volatile uint32_t *SYST_CVR = 0xE000E018;
        {

            /* enable systick */

            *SYST_CSR = 5;
            *SYST_RVR = 0x00ffffff;

        }
        {
            uint32_t sr;
            uint32_t var;
            uint32_t time = *SYST_CVR;

            for (var = 0; var < 1000; ++var) {
                sr = APBCOM_READ_WORD(APBCOM_REG_SR);
            }

            SDC600_LOG_ERR(ENTITY_NAME, "SR tick[%u]\n", time - *SYST_CVR);
        }
        {
            uint32_t sr;
            uint32_t var;
            uint32_t time = *SYST_CVR;

            uint8_t txData[4] = { FLAG__NULL, FLAG__NULL, FLAG__NULL, FLAG__NULL };
            for (var = 0; var < 1000; ++var) {
                APBCOM_WRITE_WORD(APBCOM_REG_DR, txData);
            }

            SDC600_LOG_ERR(ENTITY_NAME, "DR tick[%u]\n", time - *SYST_CVR);
        }
    }
#endif

    // 1.  The Internal COM Port device HW detects the status of the LINKEST signal.
    //     If it is set to 1 the HW inserts LPH2RA flag to the Internal COM Port's RX FIFO,
    //     otherwise the RX FIFO remains empty. If LPH2RA symbol is inserted to the RX FIFO
    //     then the Internal COM Port device interrupts its driver (in the boot case, interrupts
    //     are not enabled). Note: if the Internal COM Port CPU is asleep,
    //     this interrupt is assumed to wake it up.

    // 2.  Driver checks if the RX FIFO is not empty (RXF field of the Status register is not 0).
    // 3.  If no symbol found (FIFO is empty or FIFO returned value is NULL),
    //     return with Debugger not connected status.

    // 4.  If the driver reads a LPH2RA flag then it knows that the debugger
    //     may be connected as the link from the External COM Port is set.
    //     Driver goes to step 6 below.

    // 5.  If the read symbol from the FIFO is anything else (garbage, leftover
    //     from previous attempts), the driver drops it and returns to step 2 above.

    /* FIXME: BLOCKED reading is active: if platform has a support to reset from
     * the host, BLOCKED reading can be removed from NOT_BLOCKED. */
    SDC600_LOG_WARN("init", "%s: Blocked reading of LPH2RA is active.\r\n", __func__);
    SDC600_LOG_WARN("init", "%s: Blocked reading LPH2RA\r\n", __func__);
    ICPD_ASSERT(IComPortReadByte(&readByte, BLOCKED, SKIP_ERROR_CHECK), ICPD_RX_SUCCESS, ICPD_FAIL);
    ICPD_ASSERT(IComPortIsFlag(readByte, FLAG_LPH2RA), true, ICPD_DEBUGGER_NOT_CONNECTED);
    SDC600_LOG_INFO("init", "%s: LPH2RA received\r\n", __func__);

    // Set the link back to the debugger:
    // 6.  The Internal COM Port device driver writes LPH2RA flag to the Internal COM Port device TX.
    ICPD_ASSERT(IComSendByte(FLAG_LPH2RA), ICPD_TX_SUCCESS, ICPD_FAIL);

    /* TODO implement a timeout */
    // Wait for External COM Port driver check of the debugged system protocol:
    /**
     * 7.  Driver polls and reads a byte from the Internal COM Port FIFO.
     *         After timeout the driver returns with Debugger not connected
     *         status.
     */
    ICPD_ASSERT(IComPortReadByte(&readByte, BLOCKED, SKIP_ERROR_CHECK), ICPD_RX_SUCCESS, ICPD_DEBUGGER_NOT_CONNECTED);

    /**
     * 8.  If the driver reads an IDR flag then it goes to step 10 below.
     *         Otherwise it returns with unexpected symbol received ststus.
     */
    if (IComPortIsFlag(readByte, FLAG_IDR) == true) {
        size_t actualLength = 0;

        /**
         * IDR was detected
         * 9.  The driver responds and transmits to the debugger with
         *         Identification response message from the IDBuffer.
         *         Note: this response message format has a special format. It
         *         starts with IDA flag, followed by 6 bytes of debugged system
         *         ID hex value, and an END flag. If any of the platform ID
         *         bytes has MS bits value of 101b then the transmit driver must
         *         send an ESC flag following a flip of the MS bit of the byte
         *         to transmit.
         * 10. Return with success code.
         */
        ICPD_ASSERT_PASS(
                IComPortTxInt(FLAG_IDA, IDBuffer, IDBufferLength, &actualLength),
                ICPD_TX_SUCCESS);

        gIsComPortInited = true;
    } else {
        res = ICPD_FAIL;
        goto bail;
    }

    bail:
    return res;
}

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
icpd_rx_rc_t IComPortRx(uint8_t *RxBuffer, size_t RxBufferLength, size_t *ActualLength) {
    return IComPortRxInt(FLAG_START, RxBuffer, RxBufferLength, ActualLength);
}

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
icpd_tx_rc_t IComPortTx(uint8_t *TxBuffer, size_t TxBufferLength, size_t *ActualLength) {
    return IComPortTxInt(FLAG_START, TxBuffer, TxBufferLength, ActualLength);
}
