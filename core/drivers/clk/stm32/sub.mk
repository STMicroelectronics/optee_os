srcs-$(CFG_STM32MP_CLK_CORE) += clk-stm32-core.c
srcs-$(CFG_STM32MP13_CLK) += clk-stm32mp13.c clk-stm32-rcc.c
srcs-$(CFG_STM32MP15_CLK) += clk-stm32mp15.c clk-stm32-rcc.c
srcs-$(CFG_STM32MP21_CLK) += clk-stm32mp21.c clk-stm32-rcc.c
srcs-$(CFG_STM32MP25_CLK) += clk-stm32mp25.c clk-stm32-rcc.c

srcs-$(CFG_STM32MP1_CLK_CAL) += clk-stm32mp1-cal.c
srcs-$(CFG_STM32MP2_CLK_CAL) += clk-stm32mp2-cal.c

srcs-$(CFG_STM32_HSE_MONITORING) += clk-stm32mp-hse-monitoring.c
