$(call force,CFG_SCPFW_MOD_CLOCK,y)
$(call force,CFG_SCPFW_MOD_DVFS,y)
$(call force,CFG_SCPFW_MOD_MSG_SMT,y)
$(call force,CFG_SCPFW_MOD_OPTEE_CLOCK,y)
$(call force,CFG_SCPFW_MOD_OPTEE_CONSOLE,y)
$(call force,CFG_SCPFW_MOD_OPTEE_MBX,y)
$(call force,CFG_SCPFW_MOD_OPTEE_RESET,y)
$(call force,CFG_SCPFW_MOD_OPTEE_SMT,y)
$(call force,CFG_SCPFW_MOD_OPTEE_VOLTD_REGULATOR,y)
$(call force,CFG_SCPFW_MOD_PSU,y)
$(call force,CFG_SCPFW_MOD_PSU_OPTEE_REGULATOR,y)
$(call force,CFG_SCPFW_MOD_RESET_DOMAIN,y)
$(call force,CFG_SCPFW_MOD_SCMI,y)
$(call force,CFG_SCPFW_MOD_SCMI_CLOCK,y)
$(call force,CFG_SCPFW_MOD_SCMI_PERF,y)
$(call force,CFG_SCPFW_MOD_SCMI_RESET_DOMAIN,y)
$(call force,CFG_SCPFW_MOD_SCMI_VOLTAGE_DOMAIN,y)
$(call force,CFG_SCPFW_MOD_STM32_OPP,y)

ifeq ($(call cfg-one-enabled, CFG_STM32MP23 CFG_STM32MP25),y)
$(call force,CFG_SCPFW_MOD_POWER_DOMAIN,y)
$(call force,CFG_SCPFW_MOD_SCMI_POWER_DOMAIN,y)
$(call force,CFG_SCPFW_MOD_STM32_PD,y)
endif

$(call force,CFG_SCPFW_MOD_VOLTAGE_DOMAIN,y)
$(call force,CFG_SCPFW_SCMI_PERF_PROTOCOL_OPS,y)

ifeq (,$(filter 0 1 2,$(CFG_TEE_CORE_LOG_LEVEL)))
# For OP-TEE TRACE >= DEBUG (3), the scp-firmware Info(1) level is sufficient
# and for lower mode, the default value is defined in scmi-server/conf.mk
CFG_SCPFW_LOG_LEVEL ?= 1
endif

$(call force,CFG_SCPFW_NOTIFICATION,n)
$(call force,CFG_SCPFW_FAST_CHANNEL,n)

# Default configuration values for product specific modules
CFG_SCPFW_MOD_PSU_OPTEE_REGULATOR ?= n
CFG_SCPFW_MOD_STM32_PD ?= n
