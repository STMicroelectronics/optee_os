// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2026, STMicroelectronics
 */

#include <io.h>
#include <kernel/delay.h>
#include <stm32_util.h>

#include "clk-calibration-core.h"

#define CALIBRATION_TIMEOUT_US		100000

static unsigned int stm32_calib_get_cal(struct stm32_calib_config *data)
{
	uint32_t cal = io_read32(stm32_rcc_base() + data->reg);

	return (cal & data->cal_mask) >> data->cal_shift;
}

int stm32_calib_get_trim(struct stm32_calib_config *data)
{
	uint32_t trim = 0;
	uint32_t trim_field_mask = data->trim_mask >> data->trim_shift;
	uint32_t sign_bit = trim_field_mask & ~(trim_field_mask >> 1);

	trim = (io_read32(stm32_rcc_base() + data->reg) &
		data->trim_mask) >> data->trim_shift;

	if (data->trim_min < 0 && trim & sign_bit)
		return (int)(trim | ~trim_field_mask);

	return (int)trim;
}

void stm32_calib_set_trim(struct stm32_calib_config *data, int val)
{
	uint32_t trim = 0;

	trim = (val << data->trim_shift) & data->trim_mask;

	io_clrsetbits32(stm32_rcc_base() + data->reg,
			data->trim_mask, trim);
}

static bool stm32_calib_is_forbidden(struct stm32_calib_config *data, int cal)
{
	const uint16_t *fbv = data->fbv;
	unsigned int i = 0;

	/* Check if trim is in forbidden values */
	for (i = 0; fbv[i] != 0; i++) {
		if (fbv[i] == cal)
			return true;
	}

	return false;
}

int stm32_calib_get_next_trim(struct stm32_calib_config *data, int trim,
			      int direction)
{
	int new_trim = trim + direction;
	int cal = 0;

	cal = stm32_calib_get_cal(data) + new_trim;

	if (stm32_calib_is_forbidden(data, cal))
		new_trim += direction;

	if (new_trim <= data->trim_min)
		return data->trim_min;

	if (new_trim >= data->trim_max)
		return data->trim_max;

	return new_trim;
}

void stm32_calib_start_calibration(struct stm32_calib_config *data)
{
	uint64_t timeout_ref = 0;
	unsigned long min = data->ref_freq - data->freq_margin;
	unsigned long max = data->ref_freq + data->freq_margin;
	unsigned long freq = 0UL;
	unsigned long delta_min = ULONG_MAX;
	unsigned long delta = 0UL;
	int direction = 0;
	int best_trim = 0;
	int trim = 0;
	bool calibration_done = false;
	bool was_in_range = true;

	freq = data->read_frequency();

	was_in_range = (freq >= min && freq <= max);
	trim = stm32_calib_get_trim(data);
	best_trim = trim;

	/* We are in range not necessary to find best trim */
	if (was_in_range && data->init_done)
		return;

	data->init_done = true;

	timeout_ref = timeout_init_us(CALIBRATION_TIMEOUT_US);
	do {
		/* Check whether the next trim value is better */
		if (freq == data->ref_freq) {
			best_trim = trim;
			calibration_done = true;
			break;
		}
		if (freq < data->ref_freq)
			direction = 1;
		else
			direction = -1;

		trim = stm32_calib_get_next_trim(data, best_trim, direction);

		/* No change when trim is outside of limit */
		if (trim == best_trim) {
			calibration_done = true;
			break;
		}

		stm32_calib_set_trim(data, trim);

		freq = data->read_frequency();
		delta = (data->ref_freq < freq) ?
			freq - data->ref_freq : data->ref_freq - freq;

		/*
		 * The latest trim is not better than the previous one, so stop
		 * and restore the previous trim, which was the best one.
		 * If we were out of range, continue searching the best trim.
		 */
		if (delta > delta_min && was_in_range) {
			calibration_done = true;

		} else {
			/*
			 * The latest trim is better; continue searching for
			 * the best one
			 */
			delta_min = delta;
			best_trim = trim;
		}

		was_in_range = (freq >= min && freq <= max);

	} while (!calibration_done && !timeout_elapsed(timeout_ref));

	stm32_calib_set_trim(data, best_trim);

	freq = data->read_frequency();
	if (freq < min || freq > max) {
		EMSG("%s failed: Freq %lu , trim %i timeout = %s",
		     data->name, freq, best_trim,
		     timeout_elapsed(timeout_ref) ? "yes" : "no");
		panic();
	}

	DMSG("Calibration Freq %lu , trim %i", freq, best_trim);
};
