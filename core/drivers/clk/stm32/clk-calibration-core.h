/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (C) STMicroelectronics 2026 - All Rights Reserved
 */

#ifndef CLK_CALIBRATION_CORE_H
#define CLK_CALIBRATION_CORE_H

#include <stm32_util.h>

struct stm32_calib_config {
	const char *compatible;
	const char *name;
	uint32_t reg;
	uint32_t cal_mask;
	uint32_t cal_shift;
	uint32_t trim_mask;
	uint32_t trim_shift;
	unsigned int margin_ratio;
	int trim_max;
	int trim_min;
	const uint16_t *fbv;
	unsigned long (*read_frequency)(void);
	unsigned long ref_freq;
	unsigned long freq_margin;
	bool is_used;
	bool init_done;
};

int  stm32_calib_get_trim(struct stm32_calib_config *data);
void stm32_calib_set_trim(struct stm32_calib_config *data, int val);
int stm32_calib_get_next_trim(struct stm32_calib_config *data, int trim,
			      int direction);
void stm32_calib_start_calibration(struct stm32_calib_config *data);

#endif /* CLK_CALIBRATION_CORE_H */
