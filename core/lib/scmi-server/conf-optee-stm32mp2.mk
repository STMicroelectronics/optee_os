$(call force,CFG_SCPFW_MOD_CLOCK,y)
$(call force,CFG_SCPFW_MOD_MSG_SMT,y)
$(call force,CFG_SCPFW_MOD_OPTEE_CLOCK,y)
$(call force,CFG_SCPFW_MOD_OPTEE_CONSOLE,y)
$(call force,CFG_SCPFW_MOD_OPTEE_MBX,y)
$(call force,CFG_SCPFW_MOD_OPTEE_RESET,y)
$(call force,CFG_SCPFW_MOD_OPTEE_VOLTD_REGULATOR,y)
$(call force,CFG_SCPFW_MOD_RESET_DOMAIN,y)
$(call force,CFG_SCPFW_MOD_SCMI,y)
$(call force,CFG_SCPFW_MOD_SCMI_CLOCK,y)
$(call force,CFG_SCPFW_MOD_SCMI_RESET_DOMAIN,y)
$(call force,CFG_SCPFW_MOD_SCMI_VOLTAGE_DOMAIN,y)
$(call force,CFG_SCPFW_MOD_VOLTAGE_DOMAIN,y)

ifeq (,$(filter 0 1 2,$(CFG_TEE_CORE_LOG_LEVEL)))
# For OP-TEE TRACE >= DEBUG (3), the scp-firmware Info(1) level is sufficient
# and for lower mode, the default value is defined in scmi-server/conf.mk
CFG_SCPFW_LOG_LEVEL ?= 1
endif

$(call force,CFG_SCPFW_NOTIFICATION,n)
$(call force,CFG_SCPFW_FAST_CHANNEL,n)
