srcs-y += stm32mp25_syscfg.c
srcs-$(CFG_STM32_PWR) += stm32mp25_pwr.c
srcs-$(CFG_STM32_PWR_IRQ) += stm32mp25_pwr_irq.c
srcs-$(CFG_STM32_PWR_REGUL) += stm32mp25_pwr_regul.c
srcs-$(CFG_STM32_TAMP) += stm32mp2_tamp.c
srcs-$(CFG_STPMIC2) += stm32mp2_pmic2.c
