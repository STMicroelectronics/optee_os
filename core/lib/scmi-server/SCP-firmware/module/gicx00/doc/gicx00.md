\ingroup GroupModules Modules
\addtogroup GroupGICx00 Arm GICx00

# Arm GICx00 configuration module

This module complements the base interrupt support in the AArch64 architecture.
The module initializes the GIC (Generic Interrupt Controller), then the
architecture interface is used to configure interrupts, via the
`fwk_interrupt_*` functions.

The module takes a configuration struct with base addresses to the GIC
distributor and GIC redistributor corresponding to the execution core. There are
no runtime APIs.

Registers specific to certain GIC hardware are detected at runtime based on the
IIDR register.

`gicx00` should be listed in a product's `SCP_MODULES` definition prior to
any module that uses interrupts.
