global-incdirs-y += .

srcs-y += main.c
srcs-y += stm32mp_pm.c
subdirs-y += drivers

subdirs-$(CFG_PSA_ADAC) += psa-adac
