srcs-$(CFG_STPMIC1) += stm32mp1_stpmic1.c
srcs-$(CFG_STPMIC2) += stm32mp1_stpmic2.c
srcs-y 	+= stm32mp1_pwr.c
srcs-$(CFG_STM32_PWR_IRQ) += stm32mp1_pwr_irq.c
srcs-y 	+= stm32mp1_syscfg.c
