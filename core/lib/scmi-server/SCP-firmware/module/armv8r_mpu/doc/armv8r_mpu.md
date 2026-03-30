\ingroup GroupModules
\addtogroup GroupMPUARMv8R MPU (ARMv8-R64)

# Armv8-R MPU configuration module

This module can be used to configure the PMSA (Protected Memory System
Architecture) in SCP-firmware products using the Armv8-R64 architecture support.
It only defines an init function, which configures the PMSA.

Note that an initial MPU memory map is configured in the early architecture
code, but this module is still required in order to configure additional memory
regions, e.g. for peripherals.

`armv8r_mpu` should be listed as the first entry in a product's `SCP_MODULES`
definition.
