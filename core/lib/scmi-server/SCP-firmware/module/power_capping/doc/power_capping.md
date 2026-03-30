\ingroup GroupModules Modules
\defgroup GroupPOWER_CAPPING Power Capping

# Power Capping

Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.

## Overview

The Power Capping HAL module is a service responsible for interacting with the
Power Measurements Driver to retrieve the current average power and
power averaging interval. It also provides APIs for SCMI Power Capping,
enabling the configuration of the system’s Power Cap.

Additionally, it applies PID feedback to the Power Cap requested by the agent,
preventing direct enforcement while allowing a margin for
temporary performance bursts.

The module continuously supplies the power limit to the Metrics Analyzer.

## Architecture overview

```mermaid
graph TD
    B["SCMI Power Capping"]
    node_1["Metrics Analyzer"]
    B -."Use\n\nrequest_cap\nget_applied_cap\n\nget_average_power
    set_averaging_interval\nget_averaging_interval
    get_averaging_interval_step\nget_averaging_interval_range"
    .-> C["Power Capping HAL\n\nElement: Domain{\n    requested_cap
    applied_cap\n}"]
    C -."Use\n\nget_average_power\nset_averaging_interval
    get_averaging_interval\nget_averaging_interval_step
    get_averaging_interval_range".-> D["Power Measurements Driver"]
    C -."Use\n\npid_controller_update\npid_controller_set_point"
    .-> E["PID Controller"]
    node_1 -."Use\n\nget_limit".-> C
```

## Power Capping HAL APIs

| Function  | Description |
|-----------|-------------|
| int request_cap(fwk_id_t domain_id, uint32_t requested_cap) | Set the requested cap|
| int get_applied_cap(fwk_id_t domain_id, uint32_t *cap) | Return applied cap|
| int get_limit(fwk_id_t domain_id, uint32_t *power_limit) | Return requested cap as power limit|
| int get_average_power(fwk_id_t id, uint32_t * power) | Get average power across an averaging interval.|
| int set_averaging_interval(fwk_id_t id, uint32_t pai)	| Set the power averaging interval.|
| int get_averaging_interval(fwk_id_t id, uint32_t *pai) | Get the power averaging interval.|
| int get_averaging_interval_step(fwk_id_t id, uint32_t *pai_step) | Get averaging interval step.|
| int get_averaging_interval_range(fwk_id_t id, uint32_t *min_pai, uint32_t *max_pai) | Get averaging interval range. |

## Operation

### PID controlling operation

```mermaid
sequenceDiagram
  actor line_1 as Agent
  participant Coordinator as SCMI Power Capping
  participant Provider_0 as Power Capping HAL
  participant Provider_1 as PID Controller
  participant Provider_m as Power Measurements Driver
  participant Metrics_Analyzer as Metrics Analyzer
  participant Consumer as Generic Module
  Provider_0 ->> Consumer: fwk_notification_subscribe(power_limit_set_notification_id,<br>power_limit_set_notifier_id,<br>id);
  line_1 ->> Coordinator: scmi_power_capping_cap_set(<br>domain_id, power_cap)
  Coordinator ->> Provider_0: request_cap(domain_id, power_cap)
  Provider_0 ->> Provider_0: Update <br>requested_cap buffer
  Provider_0 ->> Provider_1: pid_controller_set_point(id, <br>requested_cap)
  line_1 ->> Coordinator: scmi_power_capping_cap_get(<br>domain_id, *power_cap)
  Coordinator ->> Provider_0: get_applied_cap(<br>domain_id, *power_cap)
  Provider_0 --) Coordinator: return applied_cap buffer
  Metrics_Analyzer ->> Provider_0: get_limit(domain_id, *power_limit)
  Provider_0 ->> Provider_m: get_average_power(domain_id, *power)
  Provider_m --) Provider_0: return power
  Provider_0 ->> Provider_1: pid_controller_update(id,<br> power, *pid_output)
  Provider_1 --) Provider_0: return pid_output
  Provider_0 --) Metrics_Analyzer: return pid_output
  Consumer ->> Provider_0: notify(power_limit_set_notification_id)
  Provider_0 --) Provider_0: Assign requested_cap to<br>applied_cap buffer
```

### Measurements Get operation

```mermaid
sequenceDiagram
  actor line_1 as Agent
  participant Coordinator as SCMI Power Capping
  participant Provider_0 as Power Capping HAL
  participant Provider_1 as Power Measurements Driver
  line_1 ->> Coordinator: scmi_power_capping_measurements_get(<br>domain_id, *power, *pai)
  Coordinator ->> Provider_0: get_average_power(domain_id, *power)
  Provider_0 ->> Provider_1: get_average_power(domain_id, *power)
  Provider_1 --) Provider_0: return power
  Provider_0 --) Coordinator: return power
  Coordinator ->> Provider_0: get_averaging_interval(domain_id, *pai)
  Provider_0 ->> Provider_1: get_averaging_interval(domain_id, *pai)
  Provider_1 --) Provider_0: return pai
  Provider_0 --) Coordinator: return pai
  Coordinator --) line_1: return power & pai
```

### PAI Get operation

```mermaid
sequenceDiagram
  actor line_1 as Agent
  participant Coordinator as SCMI Power Capping
  participant Provider_0 as Power Capping HAL
  participant Provider_1 as Power Measurements Driver
  line_1 ->> Coordinator: scmi_power_capping_pai_get(domain_id, *pai)
  Coordinator ->> Provider_0: get_averaging_interval(domain_id, *pai)
  Provider_0 ->> Provider_1: get_averaging_interval(domain_id, *pai)
  Provider_1 --) Provider_0: return pai
  Provider_0 --) Coordinator: return pai
  Coordinator --) line_1: return pai
```

### PAI Set operation

```mermaid
sequenceDiagram
  actor line_1 as Agent
  participant Coordinator as SCMI Power Capping
  participant Provider_0 as Power Capping HAL
  participant Provider_1 as Power Measurements Driver
  line_1 ->> Coordinator: scmi_power_capping_pai_set(domain_id, pai)
  Coordinator ->> Provider_0: set_averaging_interval(domain_id, pai)
  Provider_0 ->> Provider_1: set_averaging_interval(domain_id, pai)
```

### Domain Attributes operation

```mermaid
sequenceDiagram
  actor line_1 as Agent
  participant Coordinator as SCMI Power Capping
  participant Provider_0 as Power Capping HAL
  participant Provider_1 as Power Measurements Driver
  line_1 ->> Coordinator: scmi_power_capping_domain_attributes(<br>domain_id, *min_pai, *max_pai, *pai_step)
  Coordinator ->> Provider_0: get_averaging_interval_step(domain_id, *pai_step)
  Provider_0 ->> Provider_1: get_averaging_interval_step(domain_id, *pai_step)
  Provider_1 --) Provider_0: return pai_step
  Provider_0 --) Coordinator: return pai_step
  Coordinator ->> Provider_0: get_averaging_interval_range(<br>domain_id, *min_pai, *max_pai)
  Provider_0 ->> Provider_1: get_averaging_interval_range(<br>domain_id, *min_pai, *max_pai)
  Provider_1 --) Provider_0: return min_pai & max_pai
  Provider_0 --) Coordinator: return min_pai & max_pai
  Coordinator --) line_1: return pai_step & min_pai & max_pai
```
