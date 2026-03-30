\ingroup GroupModules
\addtogroup GroupMockTelemetrySource Mock Telemetry Source

# Mock Telemetry Source module

## Overview

The **Mock Telemetry Source** module implements `mod_telemetry_driver_api` and
generates deterministic telemetry samples without relying on hardware. It is
primarily intended for:

- Early bring-up when real telemetry producers are unavailable.
- Demonstrations that require predictable data.
- Continuous-integration tests that need to exercise the telemetry/SCMI stack.

Mock source exposes a configurable set of Data Events (DEs) with names,
optional FastChannel metadata, seeded values, and per-DE mutators that update
the mock data before every read.

---

## Configuration

The module is configured via `struct mod_mock_telemetry_source_config`
(`module/mock_telemetry_source/include/mod_mock_telemetry_source.h`):

```c
struct mod_mock_telemetry_source_config {
    size_t de_count;
    const struct mod_telemetry_de_desc *de_desc_table;
    const struct mod_telemetry_de_fch_attr *de_fch_attr_table;
    const char *const *de_names;
    const uint64_t *initial_values;
    const mod_mock_telemetry_source_mutator *value_mutators;
};
```

| Field | Description |
| --- | --- |
| `de_count` | Number of Data Events exposed by the mock source (must be > 0). |
| `de_desc_table` | DE descriptor table (`struct mod_telemetry_de_desc`) with `de_count` entries. |
| `de_fch_attr_table` | Optional FastChannel attributes (`NULL` disables FastChannel metadata). |
| `de_names` | Array of NULL-terminated strings naming each DE. |
| `initial_values` | Optional seed values per DE (defaults to zero when `NULL`). |
| `value_mutators` | Optional table of callbacks invoked to mutate the stored value after each read. |

All tables must remain valid for the lifetime of the module and contain
`de_count` entries.

---

## Dependencies

The module relies only on standard framework services (`fwk_time`,
`fwk_mm`, etc.) and does not bind to external modules.

---

## API

The module implements the standard telemetry driver API:

```c
struct mod_telemetry_driver_api {
    int (*get_de_list)(
        uint32_t *num_de,
        const struct mod_telemetry_de_desc **de_list);
    int (*get_de_fch_attr)(
        uint32_t de_index,
        struct mod_telemetry_de_fch_attr *de_fch_attr);
    int (*get_de_name)(uint32_t de_index, char *name);
    int (*disable_de)(uint32_t de_index);
    int (*disable_de_all)(void);
    int (*enable_de)(uint32_t de_index);
    int (*get_data)(
        struct mod_telemetry_de_data **data_array,
        uint32_t num_des);
};
```

- `get_de_list()` returns the immutable DE descriptor table and count.
- `get_de_fch_attr()` copies optional FastChannel metadata (zeroed when absent).
- `get_de_name()` provides the configured human-readable DE name.
- `enable_de()` / `disable_de()` toggle individual DE sampling; `disable_de_all()`
  clears every DE at once.
- `get_data()` fills `struct mod_telemetry_de_data` entries with timestamps and
  mock samples for the currently enabled DEs (mutators are applied after each
  read).

---

## Value generation flow

1. A telemetry consumer calls `enable_de()` for the desired DEs. The module
   caches their descriptors, timestamps them, and copies the current mock value
   into the response buffer.
2. Subsequent `get_data()` calls refresh timestamps, write the latest mock value
   into each enabled DE buffer, and invoke the configured mutator (if any) to
   adjust the stored value for the next sample.
3. `disable_de()` (or `disable_de_all()`) clears the enable flag and removes the
   DE from the active list.

This cycle allows simple scripts to emulate increasing counters, oscillating
metrics, or any custom behavior expressible via the mutator callbacks.

---

## Example configuration


A complete configuration with three DEs and three mutators:

```c
static const struct mod_telemetry_de_desc mock_de_desc_table[] = {
    {
        .de_id = 1,
        .group_id = UINT32_MAX,
        .de_data_size = sizeof(uint64_t),
        .attributes = { UINT32_C(1) << 31, 0, 0 },
    },
    {
        .de_id = 2,
        .group_id = UINT32_MAX,
        .de_data_size = 2 * sizeof(uint64_t),
        .attributes = { UINT32_C(1) << 31, 0, 0 },
    },
    {
        .de_id = 3,
        .group_id = UINT32_MAX,
        .de_data_size = 3 * sizeof(uint64_t),
        .attributes = { UINT32_C(1) << 31, 0, 0 },
    },
};

static const struct mod_telemetry_de_fch_attr mock_de_fch_attr_table[] = {
    { 0, 0, 0 },
    { 0, 0, 0 },
    { 0, 0, 0 },
};

static const char *const mock_de_names[] = { "DE1", "DE2", "DE3" };

static const uint64_t mock_de_initial_values[] = {
    UINT64_C(0xAAAAAAAAAAAA0000),
    UINT64_C(0xBBBBBBBBBBBBFFFF),
    UINT64_C(0xCCCCCCCC00000000),
};

static void mock_de_increment(uint64_t *value) { ++(*value); }
static void mock_de_decrement(uint64_t *value) { --(*value); }
static void mock_de_multiply(uint64_t *value) { *value *= UINT64_C(2); }

static const mod_mock_telemetry_source_mutator mock_de_mutators[] = {
    mock_de_increment,
    mock_de_decrement,
    mock_de_multiply,
};

const struct fwk_module_config config_mock_telemetry_source = {
    .data = &((struct mod_mock_telemetry_source_config){
        .de_count = FWK_ARRAY_SIZE(mock_de_desc_table),
        .de_desc_table = mock_de_desc_table,
        .de_fch_attr_table = mock_de_fch_attr_table,
        .de_names = mock_de_names,
        .initial_values = mock_de_initial_values,
        .value_mutators = mock_de_mutators,
    }),
};
```

---

## Integration tips

- Register the module with the product build system:
  - `list(APPEND SCP_MODULES "mock-telemetry-source")` in `Firmware.cmake`.
  - `target_sources(... config_mock_telemetry_source.c)` in the product
    `CMakeLists.txt`.
- Bind `mod_telemetry` (or any telemetry consumer) to this driver so that
  `enable_de` / `get_data` flows reach the mock source.

---

## Example usage

A consumer can bind to the mock telemetry source and exercise the driver API to
read deterministic samples:

```c
const struct mod_telemetry_driver_api *mock_api;
const struct mod_telemetry_de_desc *de_list;
struct mod_telemetry_de_data *data_array;
uint32_t num_de;

/* Bind to the driver API */
fwk_module_bind(
    FWK_ID_MODULE(FWK_MODULE_IDX_MOCK_TELEMETRY_SOURCE),
    FWK_ID_API(
        FWK_MODULE_IDX_MOCK_TELEMETRY_SOURCE,
        0 /* only API */),
    &mock_api);

/* Enumerate available DEs */
mock_api->get_de_list(&num_de, &de_list);

/* Enable first DE */
mock_api->enable_de(0);

/* Fetch data for enabled DEs */
mock_api->get_data(&data_array, 1);

/* Disable first DE */
mock_api->disable_de(0);
```

In practice `mod_telemetry` performs these calls on behalf of higher-level
protocols (e.g., SCMI Telemetry), but the same pattern can be used by any test
harness or diagnostic component that wants to drive the mock source directly.

---