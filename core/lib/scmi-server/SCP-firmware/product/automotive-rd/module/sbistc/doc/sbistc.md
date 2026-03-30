\ingroup GroupModules Modules
\defgroup GroupSBISTC SBISTC Fault Handler

# SBISTC Fault Handler Module

The SBISTC module is responsible for handling SBISTC-related safety faults.
It subscribes to FMU fault notifications and reports when a fault associated
with the SBISTC is detected.

## Overview

- The SBISTC module does not interact directly with hardware registers.
- It is notified by the FMU module when a fault occurs on a specific FMU device
  and node, as configured per platform.
- When a configured FMU device/node fault is detected, the SBISTC module logs
  the event and can trigger further safety actions based on handler configured.
- The module supports multiple SBISTC faults, each mapped to a specific
  FMU device and node.

## Example Topology

The SBISTC block is a hardware safety mechanism that can signal errors to the FMU.
The FMU collects these error signals (from SBISTC and other sources) and,
when a fault is detected, notifies the SBISTC software module.

### Hardware Fault Signal Path

```
+------------------+   error signals  +------------------+
|    SBISTC HW     |----------------->|      FMU HW      |
|                  |----------------->| (Fault Manager)  |
+------------------+                  +------------------+
```

- The SBISTC hardware asserts an error signal to a specific FMU device/node input.
- The FMU hardware latches this as a fault.

### Software Notification Path

```
+----------------------+   Subscribe    +------------------+
|   SBISTC Module SW   |--------------->|      FMU SW      |
| (Event Subscriber)   |      To        | (Fault Handler)  |
+----------------------+  Notification  +------------------+
```

- The SBISTC module subscribes to FMU fault notifications during initialization,
  enabling it to receive relevant fault events.

```
+------------------+       Event      +----------------------+
|      FMU SW      |----------------->|   SBISTC Module SW   |
|  (Fault Handler) |                  | (Event Subscriber)   |
+------------------+                  +----------------------+
```

- The FMU software module detects the latched fault and sends a notification event.
- The SBISTC software module, subscribed to FMU notifications,
  checks if the event matches any configured SBISTC FMU device/node
  and handles the SBISTC-specific fault.

---

**In summary:**
- **Hardware:** SBISTC HW → FMU HW (error signal)
- **Software:** FMU SW → SBISTC SW (notification event)

## Configuration

The SBISTC module is configured in the platform configuration with
an array of all SBISTC faults, each specifying its FMU device and node ID.

```c
#include <mod_sbistc.h>

enum {
    FLT_SBISTC_EQ_FAIL_CORE0,
    FLT_SBISTC_DEADLCK_CORE0,
    FLT_SBISTC_EQ_FAIL_CORE1,
    FLT_SBISTC_DEADLCK_CORE1,
    /*
    ... Series continues.....
    */
    FLT_SBISTC_DEADLCK_CORE15,
    FLT_SBISTC_CNT
};

/* Example: Platform-specific SBISTC fault configuration */
static struct sbistc_fault_config sbistc_faults[FLT_SBISTC_CNT + 1] = {
    [FLT_SBISTC_EQ_FAIL_CORE0]  = { "SBISTC_EQ_FAIL_CORE0",  1,  4,  NULL },
    [FLT_SBISTC_DEADLCK_CORE0]  = { "SBISTC_DEADLCK_CORE0",  1,  5,  NULL },
    [FLT_SBISTC_EQ_FAIL_CORE1]  = { "SBISTC_EQ_FAIL_CORE1",  1,  6,  NULL },
    [FLT_SBISTC_DEADLCK_CORE1]  = { "SBISTC_DEADLCK_CORE1",  1,  7,  NULL },
    /*
    ... Series continues ....
    */
    [FLT_SBISTC_DEADLCK_CORE15] = { "SBISTC_DEADLCK_CORE15", 1, 47,  NULL },
    [FLT_SBISTC_CNT] = { NULL, 0, 0, NULL }
};

static struct mod_sbistc_config sbistc_mod_config = {
    .count = FLT_SBISTC_CNT,
    .flt_cfgs = sbistc_faults,
};

const struct fwk_module_config config_sbistc = {
    .data = &sbistc_mod_config,
};
```

- Each entry in `sbistc_faults` specifies the fault name,
  FMU device ID (System FMU), and FMU input node ID, handler.
- The module is initialized with a pointer to this array and the count of faults.

## Operation

- On initialization, the SBISTC module subscribes to FMU fault notifications.
- When a fault notification is received, the module iterates through
  its configured faults and checks if the FMU device and node match any entry.
- If a match is found, the module logs the SBISTC fault and
  can trigger further safety diagnostics or actions.

## Summary

- The SBISTC module is a logical event handler for SBISTC-related FMU faults.
- It is configured per platform with an array of FMU device and node IDs to monitor.
- It does not require a hardware base address.
- All fault handling logic is triggered by FMU notifications.
- The design supports multiple SBISTC signals and
  is easily extensible for future platforms.
- The module provides APIs to
  1. enable or disable the fault,
  2. retrieve the fault occurrence count,
  3. register a handler to be invoked when the fault is triggered.
