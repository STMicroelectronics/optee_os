\ingroup GroupPLATFORMModule PLATFORM Product Modules
\addtogroup GroupSCMIPFDIMonitor Aspen SCMI PFDI Monitor

# SCMI PFDI Monitor Protocol Module

## Overview

The **SCMI PFDI Monitor** module implements a vendor-defined SCMI protocol allowing external entities to report Platform Fault Detection Interface (PFDI) status events to the system. It acts as a translation layer between SCMI agents and the PFDI Monitor service.

---

## SCMI Protocol Details

- **Protocol ID**: `0x90`
- **Protocol Version**: `0x20000`

---

## Supported Commands

| Command ID | Command Name     | Payload (uint32_t[]) | Description                        |
|------------|------------------|----------------------|------------------------------------|
| `0x3`      | `OOR_STATUS`     | `[status]`           | Report Out-of-Reset PFDI status    |
| `0x4`      | `ONL_STATUS`     | `[status]`           | Report Online PFDI status          |

---

## Dependencies

The module depends on the following:

| Module             | API Identifier                            |
|--------------------|-------------------------------------------|
| `MOD_SCMI`         | `MOD_SCMI_API_IDX_PROTOCOL`               |
| `MOD_PFDI_MONITOR` | `MOD_PFDI_MONITOR_API_IDX_PFDI_MONITOR`   |

---

## Configuration

Each **element** must define the following:

```c
struct mod_scmi_pfdi_monitor_core_config {
    fwk_id_t scmi_service_id;
    fwk_id_t pfdi_monitor_id;
};
```

- `scmi_service_id`: Associated SCMI service.
- `pfdi_monitor_id`: Associated PFDI monitor element.

---

## Message Handling Flow

1. SCMI agent sends an Out-of-Reset or online status message with a `status` parameter.
2. SCMI PFDI Monitor module identifies the source element.
3. It forwards the status to the PFDI Monitor module using the appropriate API:
   - `oor_status()` or `onl_status()`.
4. Responds to the agent with the appropriate SCMI status code.

---

## Function Summary

- `scmi_pfdi_monitor_init()`: Initialize internal context.
- `scmi_pfdi_monitor_element_init()`: Configure each core.
- `scmi_pfdi_monitor_bind()`: Bind to SCMI and PFDI Monitor APIs.
- `scmi_pfdi_monitor_process_bind_request()`: Expose protocol API to SCMI module.
- `scmi_pfdi_monitor_message_handler()`: Dispatch received SCMI messages.
