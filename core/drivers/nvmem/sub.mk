srcs-y += nvmem.c
srcs-$(CFG_ATMEL_SFC) += atmel_sfc.c
srcs-$(CFG_NVMEM_DIE_ID) += nvmem_die_id.c
srcs-$(CFG_NVMEM_HUK) += nvmem_huk.c
srcs-$(CFG_STM32_TAMP_NVRAM) += stm32_tamp_nvram.c
