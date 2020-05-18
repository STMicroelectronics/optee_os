/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2024, STMicroelectronics
 * Author: Yannick Fertre <yannick.fertre@foss.st.com> for STMicroelectronics.
 *
 * LTDC (display controller) provides output signals to interface directly
 * to a variety of LCD and TFT panels. These output signals are: RGB signals
 * (up to 24bpp), vertical & horizontal synchronisations, data enable
 * and the pixel clock.
 * This driver provides the interface to control the secure layer.
 */

#ifndef DRIVERS_STM32_LTDC_H
#define DRIVERS_STM32_LTDC_H

#include <drivers/frame_buffer.h>
#include <tee_api_types.h>

TEE_Result stm32_ltdc_layer2_init(const struct frame_buffer *frame_buffer,
				  uint32_t x0, uint32_t y0);
TEE_Result stm32_ltdc_layer2_disable(void);

#endif /* DRIVERS_STM32_LTDC_H */
