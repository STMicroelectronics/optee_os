\ingroup GroupPLATFORMModule PLATFORM Product Modules
\addtogroup GroupPFDIMonitor Aspen PFDI Monitor

# RD-Aspen PFDI Monitor module

## Overview

The **PFDI Monitor** module provides runtime monitoring of per-core Platform Fault Detection Interface (PFDI) activity in safety-critical environments. It uses timer-based alarms to track PFDI timeouts and refresh intervals for each core within a subsystem. The module is protocol-agnostic and exposes an API for any external component (e.g., transport layer, vendor protocol) to request PFDI refreshes.

---

## Configuration


Each **element** (core) requires the following configuration:

```c
struct mod_pfdi_monitor_core_config {
    fwk_id_t alarm_id;
    unsigned int oor_pfdi_period_us;
    unsigned int boot_timeout_us;
    unsigned int onl_pfdi_period_us;
};
```

- `alarm_id`: Alarm used for monitoring this core.
- `oor_pfdi_period_us`: Timeout to receive the Out-of-Reset PFDI status.
- `boot_timeout_us`: Timeout after successful OoR before expecting Online PFDI status.
- `onl_pfdi_period_us`: Period between successive Online PFDI checks.

No module-level configuration is needed.

---

## Dependencies

The PFDI Monitor module binds to the following APIs:

| Module       | API Identifier             |
|--------------|----------------------------|
| `MOD_TIMER`  | `MOD_TIMER_API_ID_ALARM`   |

---

## API

The module provides an API to report PFDI status for individual cores:

```c
struct mod_pfdi_monitor_api {
    int (*oor_status)(fwk_id_t id, uint32_t status);
    int (*onl_status)(fwk_id_t id, uint32_t status);
};
```

- `oor_status(id, status)`: Report Out-of-Reset PFDI status.
- `onl_status(id, status)`: Report Online PFDI status.

Both APIs accept:

- `id`: Core element ID.
- `status`: Non-zero for failure, zero for success.

They return standard framework status codes.

---

## Event Handling

The module handles three internal event types:

| Event Index                         | Description                                |
| ----------------------------------- | ------------------------------------------ |
| `PFDI_MONITOR_EVENT_IDX_OOR_STATUS` | Received Out-of-Reset PFDI status.         |
| `PFDI_MONITOR_EVENT_IDX_ONL_STATUS` | Received Online PFDI status.               |
| `PFDI_MONITOR_EVENT_IDX_TIMEOUT`    | Timer expired waiting for expected status. |

Failure to receive valid status within configured intervals leads to logging errors.

---

## Notification Handling

The module handles power domain transition notifications to stop PFDI monitoring
when a core enters the OFF state.

---

## Function Summary

- `pfdi_monitor_init()`: Initialize core contexts.
- `pfdi_monitor_element_init()`: Setup configuration per core.
- `pfdi_monitor_start()`: Start OoR monitoring.
- `pfdi_monitor_bind()`: Bind to timer alarms.
- `pfdi_monitor_process_bind_request()`: Expose the PFDI Monitor API.
- `pfdi_monitor_process_event()`: Handle status or timeout events.
- `pfdi_monitor_process_notification()`: Handle power domain state transitions notifications.

---

## Example Usage

An external module (e.g., a transport layer) can bind to the PFDI Monitor API and report core PFDI statuses:

```c
const struct mod_pfdi_monitor_api *pfdi_api;

fwk_module_bind(
    FWK_ID_MODULE(FWK_MODULE_IDX_PFDI_MONITOR),
    FWK_ID_API(FWK_MODULE_IDX_PFDI_MONITOR, MOD_PFDI_MONITOR_API_IDX_PFDI_MONITOR),
    &pfdi_api);

// Report Out-of-Reset status
pfdi_api->oor_status(core_id, status);

// Later, report Online status
pfdi_api->onl_status(core_id, status);
```
