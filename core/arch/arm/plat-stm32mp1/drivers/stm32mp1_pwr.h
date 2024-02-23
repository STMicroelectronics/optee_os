/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2018-2023, STMicroelectronics
 */

#ifndef __STM32MP1_PWR_H
#define __STM32MP1_PWR_H

#include <drivers/regulator.h>
#include <types_ext.h>
#include <util.h>

#define PWR_CR1_OFF		0x00
#define PWR_CR2_OFF		0x08
#define PWR_CR3_OFF		0x0c
#define PWR_MPUCR_OFF		0x10
#define PWR_MCUCR_OFF		0x14
#define PWR_WKUPCR_OFF		0x20
#define PWR_MPUWKUPENR_OFF	0x28

#ifdef CFG_STM32MP13
/* CR1 register bitfield for STM32MP13 variants */
#define PWR_CR1_MPU_RAM_LOW_SPEED	BIT(9)
#define PWR_MPU_RAM_LOW_SPEED_THRESHOLD 1320000
#endif

/* CR3 register bitfield for STM32MP13 variants */
#define PWR_CR3_VDDSD1EN	BIT(13)
#define PWR_CR3_VDDSD1RDY	BIT(14)
#define PWR_CR3_VDDSD2EN	BIT(15)
#define PWR_CR3_VDDSD2RDY	BIT(16)
#define PWR_CR3_VDDSD1VALID	BIT(22)
#define PWR_CR3_VDDSD2VALID	BIT(23)

#define PWR_OFFSET_MASK		GENMASK_32(5, 0)

#define PWR_CR1_LPDS		BIT(0)
#define PWR_CR1_LPCFG		BIT(1)
#define PWR_CR1_LVDS		BIT(2)
#define PWR_CR1_STOP2		BIT(3)
#define PWR_CR1_DBP		BIT(8)
#define PWR_CR1_MPU_RAM_LOW_SPEED	BIT(9)

#define PWR_CR2_BREN		BIT(0)
#define PWR_CR2_RREN		BIT(1)
#define PWR_CR2_MONEN		BIT(4)
#define PWR_CR2_BRRDY		BIT(16)
#define PWR_CR2_RRRDY		BIT(17)

#define PWR_CR3_VBE		BIT(8)
#define PWR_CR3_VBRS		BIT(9)
#define PWR_CR3_DDRSREN		BIT(10)
#define PWR_CR3_DDRSRDIS	BIT(11)
#define PWR_CR3_DDRRETEN	BIT(12)
#define PWR_CR3_USB33DEN	BIT(24)
#define PWR_CR3_REG18EN		BIT(28)
#define PWR_CR3_REG11EN		BIT(30)

#define PWR_CR3_POPL_SHIFT	17
#define PWR_CR3_POPL_MASK	GENMASK_32(21, 17)

#define PWR_MPUCR_PDDS		BIT(0)
#define PWR_MPUCR_CSTDBYDIS	BIT(3)
#define PWR_MPUCR_CSSF		BIT(9)

#define PWR_MCUCR_PDDS		BIT(0)

#define PWR_WKUPCR_MASK		(GENMASK_32(27, 16) | \
				 GENMASK_32(13, 8) | GENMASK_32(5, 0))

#define PWR_MPUWKUPENR_MASK	GENMASK_32(5, 0)

enum pwr_regulator {
	PWR_REG11 = 0,
	PWR_REG18,
	PWR_USB33,
	PWR_REGU_COUNT
};

vaddr_t stm32_pwr_base(void);

unsigned int stm32mp1_pwr_regulator_mv(enum pwr_regulator id);
void stm32mp1_pwr_regulator_set_state(enum pwr_regulator id, bool enable);
bool stm32mp1_pwr_regulator_is_enabled(enum pwr_regulator id);

/* Returns the registered regulator related to @id or NULL */
struct regulator *stm32mp1_pwr_get_regulator(enum pwr_regulator id);

void stm32mp_pwr_monitoring_enable(void);
#endif /*__STM32MP1_PWR_H*/
