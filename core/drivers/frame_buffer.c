// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2016, Linaro Limited
 */

#include <assert.h>
#include <compiler.h>
#include <drivers/frame_buffer.h>
#include <util.h>

size_t frame_buffer_get_image_size(struct frame_buffer *fb __unused,
				   size_t width, size_t height)
{
	return width * height * sizeof(uint32_t);
}

void frame_buffer_clear(struct frame_buffer *fb, uint32_t color)
{
	uint32_t *base = 0;
	size_t n = 0;

	assert(fb && fb->base);
	assert(IS_ALIGNED_WITH_TYPE(fb->base, uint32_t));
	base = fb->base;

	for (n = 0; n < fb->width * fb->height; n++)
		base[n] = color;
}

void frame_buffer_set_image(struct frame_buffer *fb, size_t xpos, size_t ypos,
			    size_t width, size_t height, const void *image)
{
	const uint32_t *img = image;
	uint32_t *base = 0;
	size_t x = 0;
	size_t y = 0;

	assert(fb && fb->base && img);
	assert(IS_ALIGNED_WITH_TYPE(img, uint32_t));
	assert(IS_ALIGNED_WITH_TYPE(fb->base, uint32_t));
	base = fb->base;

	for (y = 0; y < height && (y + ypos) < fb->height; y++)
		for (x = 0; x < width && (x + xpos) < fb->width; x++)
			base[x + xpos + (y + ypos) * fb->width] =
				img[x + y * width];
}
