/*
 * Copyright (c) 2016-2019, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef _COM_PORT_DEF_H_
#define _COM_PORT_DEF_H_

#include "int_com_port_hal.h"

/* internal apbcom variant */
#define APBCOM_REG_VIDR        0xD00
#define APBCOM_REG_FIDTXR      0xD08
#define APBCOM_REG_FIDRXR      0xD0C
#define APBCOM_REG_ICSR        0xD10
#define APBCOM_REG_DR          0xD20
#define APBCOM_REG_SR          0xD2C
#define APBCOM_REG_DBR         0xD30
#define APBCOM_REG_SR_ALIAS    0xD3C

#define APBCOM_REG_ITSTATUS    0xEFC
#define APBCOM_REG_ITCTRL  0   0xF00
#define APBCOM_REG_CLAIMSET    0xFA0
#define APBCOM_REG_CLAIMCLR    0xFA4
#define APBCOM_REG_AUTHSTATUS  0xFB8
#define APBCOM_REG_DEVARCH     0xFBC
#define APBCOM_REG_DEVID       0xFC8
#define APBCOM_REG_PIDR4       0xFD0
#define APBCOM_REG_PIDR0       0xFE0
#define APBCOM_REG_PIDR1       0xFE4
#define APBCOM_REG_PIDR2       0xFE8
#define APBCOM_REG_PIDR3       0xFEC
#define APBCOM_REG_CIDR0       0xFF0
#define APBCOM_REG_CIDR1       0xFF4
#define APBCOM_REG_CIDR2       0xFF8
#define APBCOM_REG_CIDR3       0xFFC

/* status register */
#define SR_TXS_OFFSET          0
#define SR_TXS_LEN             8
#define SR_TXOE_OFFSET         13
#define SR_TXOE_LEN            1
#define SR_TXLE_OFFSET         14
#define SR_TXLE_LEN            1
#define SR_RXF_OFFSET          16
#define SR_RXF_LEN             8
#define SR_RXLE_OFFSET         30
#define SR_RXLE_LEN            1

#define APBCOM_ADDR(_a) \
                ((unsigned int)(HAL_APBCOM_BASE + _a))

#define APBCOM_READ_WORD(_a) \
                *((volatile unsigned int*)(APBCOM_ADDR(_a)))

#define APBCOM_WRITE_WORD(_a, _val) \
                *((unsigned int*)(APBCOM_ADDR(_a))) = *(uint32_t*)_val

#define APBCOM_READ_FEILD(_a, _offset, _len) \
                ((APBCOM_READ_WORD(_a) & (((1 << (_len)) - 1) << _offset)) >> _offset)

#define APBCOM_GET_FEILD(_val, _offset, _len) \
                (((_val) & (((1 << (_len)) - 1) << _offset)) >> _offset)

// Flags bytes
#define FLAG_IDR        0xA0
#define FLAG_IDA        0xA1
#define FLAG_LPH1RA     0xA6
#define FLAG_LPH1RL     0xA7
#define FLAG_LPH2RA     0xA8
#define FLAG_LPH2RL     0xA9
#define FLAG_LPH2RR     0xAA
#define FLAG_START      0xAC
#define FLAG_END        0xAD
#define FLAG_ESC        0xAE
#define FLAG__NULL      0xAF

// Reboot types
enum ResetType {
    NONE,
    nSRSTReset,
    COMPortReset
};

static inline const char* apbcomflagToStr(uint8_t flag)
{
#define FLAG_TO_STR(_a) case _a: return #_a
    switch (flag) {
        FLAG_TO_STR(FLAG_IDR    );
        FLAG_TO_STR(FLAG_IDA    );
        FLAG_TO_STR(FLAG_LPH1RA );
        FLAG_TO_STR(FLAG_LPH1RL );
        FLAG_TO_STR(FLAG_LPH2RA );
        FLAG_TO_STR(FLAG_LPH2RL );
        FLAG_TO_STR(FLAG_LPH2RR );
        FLAG_TO_STR(FLAG_START  );
        FLAG_TO_STR(FLAG_END    );
        FLAG_TO_STR(FLAG_ESC    );
        FLAG_TO_STR(FLAG__NULL  );
        default: return "other";
    }
#undef FLAG_TO_STR
}
#endif /* _COM_PORT_DEF_H_ */
