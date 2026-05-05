asm-defines-y += context_asm_defines.c

srcs-y += context.c
srcs-y += low_power.c
srcs-y += pm_helpers.S
srcs-y += power_config.c
srcs-$(CFG_PSCI_ARM32) += psci.c
