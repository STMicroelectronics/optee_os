/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2026, STMicroelectronics - All Rights Reserved
 */

#ifndef __PTA_CALIB_H
#define __PTA_CALIB_H

#define PTA_CALIB_UUID	{ 0xee4e317c, 0x568b, 0x485f, { \
			  0xb0, 0xb3, 0xd4, 0x92, 0x42, 0x6c, 0x47, 0x96 } }

/* See request_calibration() */
#define PTA_CALIB_REQUEST		0

void stm32mp_start_clock_calibration(void);

#endif /*__PTA_CALIB_H*/
