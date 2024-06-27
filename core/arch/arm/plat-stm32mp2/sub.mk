global-incdirs-y += .

srcs-y += main.c
srcs-y += stm32mp_pm.c
subdirs-y += drivers
subdirs-$(CFG_STM32_PSA_SERVICE) += psa_service

subdirs-$(CFG_PSA_ADAC) += psa-adac

srcs-$(CFG_DISPLAY) += display.c
