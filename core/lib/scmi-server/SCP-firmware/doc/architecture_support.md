# Architecture Support

## Overview

Architectural configuration comes under the `arch` directory. This area provides
not only instructions for building the architecture library, but for configuring
any architecture-dependent behaviour required by the framework.

## Structure

The structure of this directory is like so:

    .
    |-- <vendor>
    |   |-- <architecture>
    |   |   |-- arch.mk
    |   |   |-- include
    |   |   |   `-- arch_*.h
    |   |   `-- src
    |   |       |-- arch.ld.S
    |   |       |-- arch.scatter.S
    |   |       |-- arch_*.S
    |   |       |-- arch_*.c
    |   |-- include
    |   |   `-- arch_*.h
    |   |-- src
    |   |   `-- arch_*.c
    |   `-- vendor.mk

### Vendors and architectures

The _vendor_ refers to the umbrella architecture group. For Arm-based
architectures this is "arm".

The _architecture_ refers to the instruction set architecture of the target
platform.

### Build system integration

`vendor.mk` is included by the build system when building the architecture
library, and also when building the firmware. This allows you to configure
sources to compile in the architecture library, and library dependencies
(if any) when linking the final firmware binary.

`arch.ld.S` and `arch.scatter.S` also provide the linker scripts for the final
firmware. They are automatically used by the build system if not targeting the
*none* vendor (which one is chosen is based on the linker in use).

## Platform Initialization Hook

### Purpose

The platform initialization hook (`int platform_init_hook(void *params)`) is an
optional entry point for performing platform-specific initialization *before*
the framework and module initialization phase begins. It is defined as a weak
symbol that can be overridden by platform for setting up critical low-level
hardware before fwk_arch_init() is called.

### Signature

```c
int platform_init_hook(void *params);
```

- The default weak implementation simply returns FWK_SUCCESS.
- Platforms may override it to perform setup tasks and return an appropriate
status.
- On failure (return value ≠ FWK_SUCCESS), the system will panic().

### Usage Guidelines and Limitations

The `int platform_init_hook(void *params)` function executes at a very early
stage in the boot process. As such, it comes with **strict limitations**:

#### Memory

- **Do not** access memory regions that hasn't yet been enabled or configured.

#### API Usage

- **Do not** call module APIs.
- Use the framework APIs responsibly, some of the framework is not initialized
yet. For example fwk_put_event() shouldn’t be used as no event_queue has been
initialized yet.

#### Ordering

- This hook runs *before* any module or element initialization.
- This is the **first opportunity** for the platform to perform low-level setup.

#### Failure Modes

- If the hook returns any code other than **FWK_SUCCESS**, the system will call
**panic()**.
- Platforms should handle recoverable errors locally and only return non-success
for fatal conditions.

### Example – Valid Usage

```c
int platform_init_hook(void *params)
{
    /* Example: Enable SRAM region required for early stack usage */
    fwk_mmio_setbits_32(SOC_SRAM_CTRL, SOC_SRAM_CTRL_EN_MASK);

    return FWK_SUCCESS;
}
```

This example is valid because:
- It uses physical MMIO addresses to known-safe hardware regions.

### Anti-Pattern – Invalid Usage

```c
int platform_init_hook(void *params)
{
    int status;

    /* Invalid: Using framework put event before event_queue is initialized */
    status = fwk_put_event(&event);
    if (status != FWK_SUCCESS) {
        return status;
    }

    /* Invalid: Calling module API to configure GPIOs */
    return gpio_api->set_pin(3, true);
}
```

This is invalid because:
- Scheduling event requires the event queue to be initialized, which has not
happened yet.
- Calling module APIs assumes those modules have already been registered and
initialized, which is not true at this stage.

## Adding a New Architecture

To add a new architecture to the framework, follow these steps:

### 1. Define Architecture-Specific Interrupts

The framework requires architecture-specific interrupt handling to be
implemented. This is achieved by defining the necessary functions in the
`fwk_interrupt` module. These functions include enabling, disabling, and
managing interrupts for the new architecture.

#### Example:
```c
#include <fwk_interrupt.h>

void arch_interrupt_global_enable(unsigned int flags) {
    /* Architecture-specific implementation to enable global interrupts */
}

unsigned int arch_interrupt_global_disable(void) {
    /* Architecture-specific implementation to disable global interrupts */
    return 0;
}
```

### 2. Create Architecture-Specific Files

Create a new directory under `arch/` for your architecture. For example,
if the new architecture is `arch_new`, the directory structure should look like
this:

```
arch/
    arch_new/
        include/
            arch_interrupt.h
        src/
            arch_interrupt.c
```

### 3. Implement Required Functions

In the `arch_interrupt.c` file, implement the required functions for interrupt
management. These functions must match the declarations in `arch_interrupt.h`.

### 4. Update Build System

Update the `CMakeLists.txt` file in the `arch/` directory to include the new
architecture. For example:

```cmake
list(APPEND
    SCP_ARCHITECTURE_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/<new arch path>")
```

### 5. Implement Standard Library Hooks

For the new architecture, it is possible that a minimal implementation for
standard library hooks are required. These hooks for basic functionality should
be implemented in a file such as `arch_libc_hooks.c`.

#### Example:
```c
#include <stdbool.h>

void _exit(int rc)
{
    while (true) {
    }
}

int _write(int fd, const void *buf, int nbytes)
{
    return 0;
}
```

These hooks should be placed in the `src/` directory of your new architecture
folder.

### 6. Implement the `main` Function

The `main` function serves as the entry point for the architecture. It is
responsible for initializing the framework and handling early failures.
This function must be implemented in a file such as `arch_main.c`.

#### Example:
```c
#include <fwk_arch.h>
#include <fwk_status.h>
#include <fwk_noreturn.h>

#include <stdio.h>
#include <stdlib.h>

static noreturn void panic(void) {
    printf("Panic!\n");
    exit(1);
}

int main(void) {
    int status;

    status = fwk_arch_init();
    if (status != FWK_SUCCESS) {
        panic();
    }
}
```

This function initializes the framework using `fwk_arch_init()` and handles any
errors by invoking a panic handler. Place this implementation in the `src/`
directory of your new architecture folder.

> **Note:**
> - The `fwk_interrupt` module provides the interface for interrupt management.
>   Ensure that all required functions are implemented for the new architecture.
> - Refer to existing architectures (e.g., `arch/arm`) for examples of how to
>   structure and implement the required files.

## AArch64 Architecture Support

SCP-firmware has extended support for the AArch64 architecture, including both
Armv8-R64 and Armv8-A machines. This includes support for switching from EL3 to
EL2 for platforms that boot in EL3 mode, such as Cortex-A53 cores on certain
products.

The AArch64 architecture support follows the structure and interfaces defined on
the [Architecture Support](architecture_support.md) page.

Both the Armv8-R64 PMSA and Armv8-A MMU are supported. An initial memory map is
installed in `arch_crt0.S`, based on the firmware layout, which should be
customized using the `armv8r_mpu` module configuration for PMSA or the MMU
configuration for Armv8-A.

Note that AArch64 support now includes multiple exception levels. For Armv8-R64,
the firmware runs exclusively at secure EL2, while for Armv8-A, the firmware can
transition from EL3 to EL2 as needed. The EL1/0 environment is not initialized
by default.

Newlib is required to use certain SCP-firmware features on AArch64 that require
early initialization, such as notifications.

Both GCC and Clang are supported, but not Arm Clang.

### Interrupt handling

A minimal interrupt controller is provided with the following limitations:

 * All interrupts have the same priority.
 * NMIs (non-maskable interrupts) are not supported.
 * The extended SPI and PPI ranges are not supported.

AArch64 products must provide a `fmw_gic.h` header which defines:

 * `GICD_BASE` - the base address of the GIC distributor
 * `GICR_BASE` - the base address of the GIC redstributor corresponding to the
   core on which SCP-firmware is running

### Usage

To use the AArch64 architecture in a product, including the following in
Firmware.cmake:

    set(SCP_ARCHITECTURE "aarch64")

The fvp-baser-aemv8r product can be used as a reference for implementing an
AArch64 product.

## POSIX Architecture Support

SCP-firmware also provides a POSIX-hosted architecture located under
`arch/none/posix`. This target builds the framework as a regular userspace
process, which is useful for development and validating firmware
logic without Arm hardware. It follows the same architecture model
described above. Because execution happens in a POSIX process,
`arch_interrupt_global_enable()`, `arch_interrupt_global_disable()`, and
`arch_suspend()` are implemented as no-ops and should not be relied on for
functional behavior.

### Interrupt handling

Interrupt emulation is intentionally minimal and is implemented on top of POSIX
signals:

 * All interrupts have the same priority.
 * NMIs (non-maskable interrupts) are not supported.
 * Up to `IRQ_NUM` interrupt lines.
 * Delivery uses `SIGUSR1` with the IRQ line encoded in
   `siginfo_t.si_value.sival_int`.
 * Modules must also enable the relevant host signals (for example via POSIX
   `sigaction`/`mq_notify`) before they can be used for interrupt injection.

### Usage

To build a firmware for the POSIX architecture, request it in the product's
`Firmware.cmake`:

```
set(SCP_ARCHITECTURE "posix")
```

This selects the `arch-posix` support library automatically. The produced binary
runs directly on the host, making it ideal for fast iteration, sanitizers, and
tests that do not require the memory-mapped peripherals or exception-level
management that real Arm targets provide.
