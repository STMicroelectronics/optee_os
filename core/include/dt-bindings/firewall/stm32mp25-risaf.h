/* SPDX-License-Identifier: (GPL-2.0-only OR BSD-2-Clause) */
/*
 * Copyright (C) 2020-2025, STMicroelectronics
 */

#ifndef _DT_BINDINGS_FIREWALL_STM32MP25_RISAF_H
#define _DT_BINDINGS_FIREWALL_STM32MP25_RISAF_H

/* RISAF region IDs */
#define RISAF_REG_ID(idx)	(idx)

/* RISAF base region enable modes */
#define RIF_BREN_DIS		0x0
#define RIF_BREN_EN		0x1

/* RISAF encryption modes */
#define RIF_ENC_DIS		0x0
#define RIF_ENC_MCE_EN		0x1 /*
				     * STM32MP21 only,
				     * side-channel attack protection.
				     */
#define RIF_ENC_EN		0x2

/* RISAF subregion IDs */
#define RISAF_SUBREG_ID(idx)	(idx)

/* RISAF subregion enable modes */
#define RIF_SREN_DIS		0x0
#define RIF_SREN_EN		0x1

/* RISAF subregion read enable modes */
#define RIF_RDEN_DIS		0x0
#define RIF_RDEN_EN		0x1

/* RISAF subregion write enable modes */
#define RIF_WREN_DIS		0x0
#define RIF_WREN_EN		0x1

/* RISAF subregion delegation control modes */
#define RIF_DCEN_DIS		0x0
#define RIF_DCEN_EN		0x1

/* RISAF subregion resource lock modes */
#define RIF_RLOCK_DIS		0x0
#define RIF_RLOCK_EN		0x1

#define DT_RISAF_EN_SHIFT	4
#define DT_RISAF_SEC_SHIFT	5
#define DT_RISAF_ENC_SHIFT	6
#define DT_RISAF_PRIV_SHIFT	8
#define DT_RISAF_READ_SHIFT	16
#define DT_RISAF_WRITE_SHIFT	24

#define DT_RISAF_REG_ID_MASK		U(0xF)
#define DT_RISAF_EN_MASK		BIT(DT_RISAF_EN_SHIFT)
#define DT_RISAF_SEC_MASK		BIT(DT_RISAF_SEC_SHIFT)
#define DT_RISAF_ENC_MASK		GENMASK_32(7, 6)
#define DT_RISAF_PRIV_MASK		GENMASK_32(15, 8)
#define DT_RISAF_READ_MASK		GENMASK_32(23, 16)
#define DT_RISAF_WRITE_MASK		GENMASK_32(31, 24)

#define RISAFPROT(risaf_region, cid_read_list, cid_write_list, cid_priv_list, \
		  sec, enc, enabled) \
	(((cid_write_list) << DT_RISAF_WRITE_SHIFT) | \
	 ((cid_read_list) << DT_RISAF_READ_SHIFT) | \
	 ((cid_priv_list) << DT_RISAF_PRIV_SHIFT) | \
	 ((enc) << DT_RISAF_ENC_SHIFT) | \
	 ((sec) << DT_RISAF_SEC_SHIFT) | \
	 ((enabled) << DT_RISAF_EN_SHIFT) | \
	 (risaf_region))

#define DT_RISAF_SUB_EN_SHIFT		1
#define DT_RISAF_SUB_SEC_SHIFT		2
#define DT_RISAF_SUB_PRIV_SHIFT		3
#define DT_RISAF_SUB_SRCID_SHIFT	4
#define DT_RISAF_SUB_RDEN_SHIFT		8
#define DT_RISAF_SUB_WREN_SHIFT		9
#define DT_RISAF_SUB_DCEN_SHIFT		16
#define DT_RISAF_SUB_DCCID_SHIFT	17
#define DT_RISAF_SUB_RLOCK_SHIFT	31

#define DT_RISAF_SUB_REG_ID_MASK	BIT(0)
#define DT_RISAF_SUB_EN_MASK		BIT(DT_RISAF_SUB_EN_SHIFT)
#define DT_RISAF_SUB_SEC_MASK		BIT(DT_RISAF_SUB_SEC_SHIFT)
#define DT_RISAF_SUB_PRIV_MASK		BIT(DT_RISAF_SUB_PRIV_SHIFT)
#define DT_RISAF_SUB_SRCID_MASK		GENMASK_32(6, DT_RISAF_SUB_SRCID_SHIFT)
#define DT_RISAF_SUB_RDEN_MASK		BIT(DT_RISAF_SUB_RDEN_SHIFT)
#define DT_RISAF_SUB_WREN_MASK		BIT(DT_RISAF_SUB_WREN_SHIFT)
#define DT_RISAF_SUB_DCEN_MASK		BIT(DT_RISAF_SUB_DCEN_SHIFT)
#define DT_RISAF_SUB_DCCID_MASK		GENMASK_32(19, DT_RISAF_SUB_DCCID_SHIFT)
#define DT_RISAF_SUB_RLOCK_MASK		BIT(DT_RISAF_SUB_RLOCK_SHIFT)

#define RISAFSUBPROT(risaf_subregion, dccid, dcen, rden, wren, srcid, \
		     priv, sec, enabled, rlock) \
	(((rlock) << DT_RISAF_SUB_RLOCK_SHIFT) | \
	 ((dccid) << DT_RISAF_SUB_DCCID_SHIFT) | \
	 ((dcen) << DT_RISAF_SUB_DCEN_SHIFT) | \
	 ((wren) << DT_RISAF_SUB_WREN_SHIFT) | \
	 ((rden) << DT_RISAF_SUB_RDEN_SHIFT) | \
	 ((srcid) << DT_RISAF_SUB_SRCID_SHIFT) | \
	 ((priv) << DT_RISAF_SUB_PRIV_SHIFT) | \
	 ((sec) << DT_RISAF_SUB_SEC_SHIFT) | \
	 ((enabled) << DT_RISAF_SUB_EN_SHIFT) | \
	 (risaf_subregion))

#endif /* _DT_BINDINGS_FIREWALL_STM32MP25_RISAF_H */
