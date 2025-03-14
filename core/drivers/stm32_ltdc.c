// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2024, STMicroelectronics
 * Author(s): Yannick Fertre <yannick.fertre@foss.st.com>
 *            Cedric VINCENT <cedric.vincent@st.com>
 *
 * LTDC (display controller) provides output signals to interface directly
 * a variety of LCD and TFT panels. These output signals are: RGB signals
 * (up to 24bpp), vertical & horizontal synchronisations, data enable
 * and the pixel clock.
 * This driver provides the interface to control the secure layer.
 */

#include <display.h>
#include <drivers/clk.h>
#include <drivers/clk_dt.h>
#include <drivers/firewall.h>
#include <drivers/firewall_device.h>
#include <drivers/frame_buffer.h>
#include <drivers/stm32_gpio.h>
#include <drivers/stm32mp_dt_bindings.h>
#include <io.h>
#include <kernel/boot.h>
#include <kernel/delay.h>
#include <kernel/dt.h>
#include <kernel/interrupt.h>
#include <kernel/panic.h>
#include <libfdt.h>
#include <mm/core_memprot.h>
#include <stm32_util.h>
#include <trace.h>
#include <types_ext.h>

enum {
	LTDC_ITR_STATUS,
	LTDC_ITR_ERROR,
	LTDC_ITR_NUM
};

struct ltdc_device {
	vaddr_t regs;				/* registers*/
	struct clk *clock;			/* pixel clock */
	struct pinctrl_state *pinctrl;		/* pins control settings */
	struct itr_chip *itr_chip[LTDC_ITR_NUM];/* chip interrupt */
	size_t itr_num[LTDC_ITR_NUM];		/* number interrupt */
	struct itr_handler *itr[LTDC_ITR_NUM];	/* handler interrupt */
	bool end_of_frame;			/* end of frame flag */
	bool activate;				/* activate flag */
	struct firewall_alt_conf *nsec_conf;	/* no secure config */
	struct firewall_alt_conf *sec_conf;	/* secure config */
};

#define LTDC_IDR	0x0000
#define LTDC_LCR	0x0004
#define LTDC_SSCR	0x0008
#define LTDC_BPCR	0x000C
#define LTDC_AWCR	0x0010
#define LTDC_TWCR	0x0014
#define LTDC_GCR	0x0018
#define LTDC_SRCR	0x0024
#define LTDC_IER2	0x0064
#define LTDC_ISR2	0x0068
#define LTDC_ICR2	0x006C
#define IER_LIE		BIT(0)	/* Line Interrupt Enable */
#define IER_FUWIE	BIT(1)	/* Fifo Underrun Warning Interrupt Enable */
#define IER_TERRIE	BIT(2)	/* Transfer ERRor Interrupt Enable */
#define IER_RRIE	BIT(3)	/* Register Reload Interrupt Enable */
#define IER_FUKIE	BIT(6)	/* Fifo Underrun Killing Interrupt Enable */
#define IER_CRCIE	BIT(7)	/* CRC Error Interrupt Enable */
#define IER_FURIE	BIT(8)	/* Fifo Underrun at Rotation Interrupt Enable */
#define ISR_LIF		BIT(0)	/* Line Interrupt Flag */
#define ISR_FUWIF	BIT(1)	/* Fifo Underrun Warning Interrupt Flag */
#define ISR_TERRIF	BIT(2)	/* Transfer ERRor Interrupt Flag */
#define ISR_RRIF	BIT(3)	/* Register Reload Interrupt Flag */
#define ISR_FUKIF	BIT(6)	/* Fifo Underrun Killing Interrupt Flag */
#define ISR_CRCIF	BIT(7)	/* CRC Error Interrupt Flag */
#define ISR_FURIF	BIT(8)	/* Fifo Underrun at Rotation Interrupt Flag */

#define ID_HWVER_40100		0x040100
#define ID_HWVER_40101		0x040101
#define GCR_LTDCEN		BIT(0)
#define LTDC_BPCR_AHBP		GENMASK_32(27, 16)
#define LTDC_BPCR_AVBP		GENMASK_32(10, 0)
#define LTDC_LCR_LNBR		GENMASK_32(7, 0)
#define LTDC_LXWHPCR_WHSTPOS	GENMASK_32(11, 0)
#define LTDC_LXWHPCR_WHSPPOS	GENMASK_32(31, 16)
#define LTDC_LXWVPCR_WVSTPOS	GENMASK_32(11, 0)
#define LTDC_LXWVPCR_WVSPPOS	GENMASK_32(31, 16)
#define LTDC_LXCFBLR_CFBLL	GENMASK_32(12, 0)
#define LTDC_LXCFBLR_CFBP	GENMASK_32(28, 16)
#define LTDC_LXPFCR_PF		GENMASK_32(2,  0)
#define LTDC_LXCACR_CONSTA	GENMASK_32(7, 0)
#define LXBFCR_BF2		GENMASK_32(2, 0)
#define LXBFCR_BF1		GENMASK_32(10, 8)
#define LTDC_LXCFBLNR_CFBLNBR	GENMASK_32(10, 0)
#define LTDC_LXDCCR_DCBLUE	GENMASK_32(7, 0)
#define LTDC_LXDCCR_DCGREEN	GENMASK_32(15, 8)
#define LTDC_LXDCCR_DCRED	GENMASK_32(23, 16)
#define LTDC_LXDCCR_DCALPHA	GENMASK_32(31, 24)

enum ltdc_pix_fmt {
	LXPFCR_PF_ARGB8888,
	LXPFCR_PF_ABGR8888,
	LXPFCR_PF_RGBA8888,
	LXPFCR_PF_BGRA8888,
	LXPFCR_PF_RGB565,
	LXPFCR_PF_BGR565,
	LXPFCR_PF_RGB888
};

/* Within mask LTDC_LXBFCR_BF1 */
#define LXBFCR_BF1_PAXCA	0x600	/* Pixel Alpha x Constant Alpha */
#define LXBFCR_BF1_CA		0x400	/* Constant Alpha */
/* Within mask LTDC_LXBFCR_BF2 */
#define LXBFCR_BF2_PAXCA	0x007	/* 1 - (Pixel Alpha x Constant Alpha) */
#define LXBFCR_BF2_CA		0x005	/* 1 - Constant Alpha */

#if defined(CFG_STM32MP25) || defined(CFG_STM32MP23) || defined(CFG_STM32MP21)
#define LAY_OFS(a)	(3 * 0x100 + (a))
#else
#define LAY_OFS(a)	(2 * 0x100 + (a))
#endif /* CFG_STM32MP25 | CFG_STM32MP23 | CFG_STM32MP21 */

#define LTDC_LXRCR	LAY_OFS(0x08)
#define LXCR_RCR_IMR	BIT(0)
#define LXCR_RCR_VBR	BIT(1)
#define LTDC_LXCR	LAY_OFS(0x0c)
#define LXCR_LEN	BIT(0)
#define LTDC_LXWHPCR	LAY_OFS(0x10)
#define LTDC_LXWVPCR	LAY_OFS(0x14)
#define LTDC_LXPFCR	LAY_OFS(0x1c)
#define LTDC_LXCACR	LAY_OFS(0x20)
#define LTDC_LXDCCR	LAY_OFS(0x24)
#define LTDC_LXBFCR	LAY_OFS(0x28)
#define LTDC_LXCFBAR	LAY_OFS(0x34)
#define LTDC_LXCFBLR	LAY_OFS(0x38)
#define LTDC_LXCFBLNR	LAY_OFS(0x3c)

/* Timeout when polling on status */
#define LTDC_TIMEOUT_US	U(100000)

static TEE_Result stm32_ltdc_init(void *device)
{
	struct ltdc_device *ldev = device;
	TEE_Result ret = TEE_ERROR_GENERIC;
	uint32_t gcr = 0;

	ret = clk_enable(ldev->clock);
	if (ret)
		return ret;

	gcr = io_read32(ldev->regs + LTDC_GCR);
	if (!(gcr & GCR_LTDCEN)) {
		EMSG("CRTC must be started first");
		ret = TEE_ERROR_GENERIC;
		goto err;
	}

	/* Force the LTDC to secure access */
	ret = firewall_set_alternate_conf(ldev->sec_conf);
	if (ret)
		goto err;

	if (ldev->pinctrl) {
		ret = pinctrl_apply_state(ldev->pinctrl);
		if (ret) {
			firewall_set_alternate_conf(ldev->nsec_conf);
			goto err;
		}
	}

	return TEE_SUCCESS;
err:
	clk_disable(ldev->clock);

	return ret;
}

static TEE_Result stm32_ltdc_final(void *device)
{
	struct ltdc_device *ldev = device;
	TEE_Result ret = TEE_ERROR_GENERIC;
	uint64_t timeout_ref = 0;

	if (!ldev->activate)
		goto set_firewall;

	ret = clk_enable(ldev->clock);
	if (ret)
		return ret;

	interrupt_enable(ldev->itr_chip[LTDC_ITR_STATUS],
			 ldev->itr[LTDC_ITR_STATUS]->it);

	interrupt_enable(ldev->itr_chip[LTDC_ITR_ERROR],
			 ldev->itr[LTDC_ITR_ERROR]->it);

	/* Disable secure layer */
	io_clrbits32(ldev->regs + LTDC_LXCR, LXCR_LEN);

	/* Reload configuration at next vertical blanking. */
	io_write32(ldev->regs + LTDC_LXRCR, LXCR_RCR_VBR);

	ldev->end_of_frame = false;

	/* Enable line IRQ */
	io_setbits32(ldev->regs + LTDC_IER2, IER_LIE);

	/* wait end of frame */
	timeout_ref = timeout_init_us(LTDC_TIMEOUT_US);
	while (!timeout_elapsed(timeout_ref))
		if (ldev->end_of_frame)
			break;

	/* Disable line IRQ */
	io_clrbits32(ldev->regs + LTDC_IER2, IER_LIE);

	/* Allow an almost silent failure here */
	if (!ldev->end_of_frame)
		EMSG("ltdc: Did not receive end of frame interrupt");

	interrupt_disable(ldev->itr_chip[LTDC_ITR_STATUS],
			  ldev->itr[LTDC_ITR_STATUS]->it);

	interrupt_disable(ldev->itr_chip[LTDC_ITR_ERROR],
			  ldev->itr[LTDC_ITR_ERROR]->it);

	clk_disable(ldev->clock);
set_firewall:
	/* Force the LTDC to non secure access */
	if (firewall_set_alternate_conf(ldev->nsec_conf))
		panic("Firewall: Cannot set ltdc to no secure");

	ldev->activate = false;

	return TEE_SUCCESS;
}

static TEE_Result stm32_ltdc_activate(void *device,
				      const struct frame_buffer *fb,
				      uint32_t x0, uint32_t y0)
{
	struct ltdc_device *ldev = device;
	paddr_t fb_pbase = 0;
	TEE_Result ret = TEE_ERROR_GENERIC;
	uint32_t value = 0;
	uint32_t x1 = 0;
	uint32_t y1 = 0;
	uint32_t width_crtc = 0;
	uint32_t height_crtc = 0;
	uint32_t bpcr = 0;
	uint32_t awcr = 0;

	assert(ldev && ldev->regs && fb);

	ret = clk_enable(ldev->clock);
	if (ret)
		return ret;

	x1 = x0 + fb->width;
	y1 = y0 + fb->height;

	/* Check framebuffer size */
	awcr = io_read32(ldev->regs + LTDC_AWCR);
	bpcr = io_read32(ldev->regs + LTDC_BPCR);

	height_crtc = (awcr & 0xffff) - (bpcr & 0xffff);
	width_crtc = (awcr >> 16) - (bpcr >> 16);

	if (fb->height > height_crtc || fb->width > width_crtc || !fb->base) {
		ret = TEE_ERROR_GENERIC;
		goto err;
	}

	fb_pbase = virt_to_phys(fb->base);

	DMSG("LTDC base %"PRIxVA", FB base %#"PRIxPA, ldev->regs, fb_pbase);

	/* Configure the horizontal start and stop position */
	value = (x0 + ((bpcr & LTDC_BPCR_AHBP) >> 16) + 1) |
		((x1 + ((bpcr & LTDC_BPCR_AHBP) >> 16)) << 16);
	io_clrsetbits32(ldev->regs + LTDC_LXWHPCR,
			LTDC_LXWHPCR_WHSTPOS | LTDC_LXWHPCR_WHSPPOS, value);

	/* Configure the vertical start and stop position */
	value = (y0 + (bpcr & LTDC_BPCR_AVBP) + 1) |
		((y1 + (bpcr & LTDC_BPCR_AVBP)) << 16);

	io_clrsetbits32(ldev->regs + LTDC_LXWVPCR,
			LTDC_LXWVPCR_WVSTPOS | LTDC_LXWVPCR_WVSPPOS, value);

	/* Specifies the pixel format, hard coded */
	io_clrsetbits32(ldev->regs + LTDC_LXPFCR, LTDC_LXPFCR_PF,
			LXPFCR_PF_ARGB8888);

	/* Configure the default color values, hard coded */
	io_clrsetbits32(ldev->regs + LTDC_LXDCCR,
			LTDC_LXDCCR_DCBLUE | LTDC_LXDCCR_DCGREEN |
			LTDC_LXDCCR_DCRED | LTDC_LXDCCR_DCALPHA,
			0x00FFFFFF);

	/* Specifies the constant alpha value, hard coded. */
	io_clrsetbits32(ldev->regs + LTDC_LXCACR, LTDC_LXCACR_CONSTA, 0xFF);

	/* Specifies the blending factors, hard coded. */
	io_clrsetbits32(ldev->regs + LTDC_LXBFCR, LXBFCR_BF2 | LXBFCR_BF1,
			LXBFCR_BF1_PAXCA | LXBFCR_BF2_PAXCA);

	/* Configure the color frame buffer start address. */
	io_write32(ldev->regs + LTDC_LXCFBAR, fb_pbase);

	/* Configure the color frame buffer pitch in byte, assuming ARGB32. */
	value =	((fb->width * 4) << 16) | (((x1 - x0) * 4)  + 3);
	io_clrsetbits32(ldev->regs + LTDC_LXCFBLR,
			LTDC_LXCFBLR_CFBLL | LTDC_LXCFBLR_CFBP, value);

	/* Configure the frame buffer line number. */
	io_clrsetbits32(ldev->regs + LTDC_LXCFBLNR,
			LTDC_LXCFBLNR_CFBLNBR, fb->height);

	/* Enable LTDC_Layer by setting LEN bit. */
	io_setbits32(ldev->regs + LTDC_LXCR, LXCR_LEN);

	/* Reload configuration at next vertical blanking. */
	io_write32(ldev->regs + LTDC_LXRCR, LXCR_RCR_VBR);

	/* Enable IRQs */
	io_setbits32(ldev->regs + LTDC_IER2, IER_FUKIE | IER_TERRIE);

	clk_disable(ldev->clock);

	ldev->activate = true;

	return TEE_SUCCESS;
err:
	clk_disable(ldev->clock);

	return ret;
}

static TEE_Result stm32_ltdc_get_display_size(void *device,
					      uint32_t *width,
					      uint32_t *height)
{
	struct ltdc_device *ldev = device;
	TEE_Result ret = TEE_ERROR_GENERIC;
	uint32_t bpcr = 0;
	uint32_t awcr = 0;
	uint32_t gcr = 0;

	assert(ldev && ldev->regs);

	if (!width || !height)
		return TEE_ERROR_BAD_PARAMETERS;

	ret = clk_enable(ldev->clock);
	if (ret)
		return ret;

	gcr = io_read32(ldev->regs + LTDC_GCR);
	if (!(gcr & GCR_LTDCEN)) {
		EMSG("CRTC must be started first");
		ret = TEE_ERROR_GENERIC;
		goto out;
	}

	awcr = io_read32(ldev->regs + LTDC_AWCR);
	bpcr = io_read32(ldev->regs + LTDC_BPCR);

	*height = (awcr & 0xffff) - (bpcr & 0xffff);
	*width = (awcr >> 16) - (bpcr >> 16);
out:
	clk_disable(ldev->clock);

	return ret;
}

static enum itr_return stm32_ltdc_it_handler(struct itr_handler *handler)
{
	struct ltdc_device *ldev = handler->data;
	uint32_t irq_status = 0;

	assert(ldev && ldev->regs);

	irq_status = io_read32(ldev->regs + LTDC_ISR2);
	io_write32(ldev->regs + LTDC_ICR2, irq_status);

	if (irq_status & ISR_FUKIF)
		EMSG("ltdc fifo underrun: please verify display mode");

	if (irq_status & ISR_TERRIF)
		EMSG("ltdc transfer error");

	if (irq_status & ISR_LIF)
		ldev->end_of_frame = true;

	return ITRR_HANDLED;
}
DECLARE_KEEP_PAGER(stm32_ltdc_it_handler);

static struct disp_dev_list ltdc_dev = {
	.device_init = stm32_ltdc_init,
	.device_final = stm32_ltdc_final,
	.device_activate = stm32_ltdc_activate,
	.device_get_display_size = stm32_ltdc_get_display_size,
};

static TEE_Result stm32_ltdc_probe(const void *fdt, int node,
				   const void *compat_data __unused)
{
	struct ltdc_device *ldev = NULL;
	struct dt_node_info dt_info = { };
	uint32_t hwid = 0;
	TEE_Result ret = TEE_ERROR_GENERIC;
	struct io_pa_va io_base = { };

	ldev = calloc(1, sizeof(*ldev));
	if (!ldev)
		return TEE_ERROR_OUT_OF_MEMORY;

	fdt_fill_device_info(fdt, &dt_info, node);

	if (dt_info.reg == DT_INFO_INVALID_REG ||
	    dt_info.reg_size == DT_INFO_INVALID_REG_SIZE)
		goto err1;

	io_base.pa = dt_info.reg;
	ldev->regs = io_pa_or_va_secure(&io_base, dt_info.reg_size);

	ret = clk_dt_get_by_index(fdt, node, 0, &ldev->clock);
	if (ret)
		goto err1;

	ret = clk_enable(ldev->clock);
	if (ret)
		panic("Cannot access LTDC clock");

	hwid = io_read32(ldev->regs + LTDC_IDR);

	clk_disable(ldev->clock);

	if (hwid != ID_HWVER_40100 && hwid != ID_HWVER_40101) {
		EMSG("LTDC hardware version not supported: 0x%x", hwid);
		ret = TEE_ERROR_NOT_SUPPORTED;
		goto err1;
	}

	ret = interrupt_dt_get_by_index(fdt, node, LTDC_ITR_STATUS,
					&ldev->itr_chip[LTDC_ITR_STATUS],
					&ldev->itr_num[LTDC_ITR_STATUS]);
	if (ret)
		goto err1;

	ret = interrupt_dt_get_by_index(fdt, node, LTDC_ITR_ERROR,
					&ldev->itr_chip[LTDC_ITR_ERROR],
					&ldev->itr_num[LTDC_ITR_ERROR]);
	if (ret)
		goto err1;

	ret = interrupt_alloc_add_handler(ldev->itr_chip[LTDC_ITR_STATUS],
					  ldev->itr_num[LTDC_ITR_STATUS],
					  stm32_ltdc_it_handler,
					  ITRF_TRIGGER_LEVEL, ldev,
					  &ldev->itr[LTDC_ITR_STATUS]);
	if (ret)
		goto err1;

	ret = interrupt_alloc_add_handler(ldev->itr_chip[LTDC_ITR_ERROR],
					  ldev->itr_num[LTDC_ITR_ERROR],
					  stm32_ltdc_it_handler,
					  ITRF_TRIGGER_LEVEL, ldev,
					  &ldev->itr[LTDC_ITR_ERROR]);
	if (ret)
		goto err2;

	ret = pinctrl_get_state_by_name(fdt, node, "default", &ldev->pinctrl);
	if (ret && ret != TEE_ERROR_ITEM_NOT_FOUND)
		goto err3;

	ret = firewall_dt_get_alternate_conf(fdt, node, "sec",
					     &ldev->sec_conf);
	if (ret)
		goto err3;

	ret = firewall_dt_get_alternate_conf(fdt, node, "nsec",
					     &ldev->nsec_conf);
	if (ret)
		goto err4;

	/* Force the LTDC to non secure access */
	ret = firewall_set_alternate_conf(ldev->nsec_conf);
	if (ret)
		goto err5;

	ltdc_dev.device = ldev;
	display_register_device(&ltdc_dev);

	return TEE_SUCCESS;
err5:
	firewall_alternate_conf_put(ldev->sec_conf);
err4:
	firewall_alternate_conf_put(ldev->nsec_conf);
err3:
	interrupt_remove_free_handler(ldev->itr[LTDC_ITR_ERROR]);
err2:
	interrupt_remove_free_handler(ldev->itr[LTDC_ITR_STATUS]);
err1:
	free(ldev);

	return ret;
}

static const struct dt_device_match ltdc_match_table[] = {
	{ .compatible = "st,stm32-ltdc" },
	{ }
};

DEFINE_DT_DRIVER(stm32_ltdc_dt_driver) = {
	.name = "stm32-ltdc",
	.match_table = ltdc_match_table,
	.probe = stm32_ltdc_probe,
};
