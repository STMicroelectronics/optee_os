# Build Configurations

CMake provides various configurations, some for the framework itself and others
for specific modules. These settings are customized to generate the desired
build. The SCP firmware utilizes this capability to configure different build
options for specific firmware targets (e.g., scp_ramfw or scp_romfw).

## Framework Configurations

- `SCP_ENABLE_FWK_EVENT_WATERMARK_TRACING`: Enable/disable tracing for event
  queues.

- `SCP_ENABLE_MARKED_LIST`: Enable/disable calculations of list max size.

- `SCP_ENABLE_NOTIFICATIONS`: Enable/disable notifications within SCP firmware,
  When notification support is enabled, the following applies:
    -The `BUILD_HAS_NOTIFICATION` is defined
      for the units being built.
    -Notification specific APIs are made available to the modules via the
      framework components (see [framework.md](doc/framework.md)).

- `SCP_ENABLE_RESOURCE_PERMISSIONS`: Enable/disable resource
  permissions settings.

- `SCP_ENABLE_SUB_SYSTEM_MODE`: Enable the execution as a sub-system.

## Module Configurations

- `SCP_ENABLE_DEBUG_UNIT`: Enable/disable debug unit.

- `SCP_ENABLE_STATISTICS`: Enable/disable statistics.

### SCMI Configurations

- `SCP_ENABLE_SCMI_NOTIFICATIONS`: Enable/disable SCMI notifications.

- `SCP_ENABLE_FAST_CHANNELS`: Enable/disable Fast Channels support. This
  option should be enabled/disabled by the use of a platform specific setting
  like `SCP_ENABLE_SCMI_PERF_FAST_CHANNELS`.

- `SCP_TARGET_EXCLUDE_BASE_PROTOCOL`: Exclude Base Protocol functionality from
  the SCMI Module.

- `SCP_ENABLE_SCMI_RESET`: Enable/disable SCMI reset domain protocol.

- `SCP_ENABLE_AGENT_LOGICAL_DOMAIN`: Parameter controls whether SCMI agents
  can have their relative view on system resources exposed by SCMI protocols.

### Transport Configurations

- `SCP_ENABLE_INBAND_MSG_SUPPORT`: Enable/disable Inband message
  support for the transport.

- `SCP_ENABLE_OUTBAND_MSG_SUPPORT`: Enable/disable Outband message
  support for the transport.

### Power Capping Configurations

- `SCP_ENABLE_SCMI_POWER_CAPPING_FAST_CHANNELS_COMMANDS`: Enable/disable
  Fast Channels support for Power Capping.

- `SCP_EXCLUDE_SCMI_POWER_CAPPING_STD_COMMANDS`: Exclude
  standard commands support for Power Capping.

### Clock Configurations

- `SCP_ENABLE_CLOCK_TREE_MGMT`: Enable/disable clock tree management support.

### Sensor Configurations

- `SCP_ENABLE_SCMI_SENSOR_EVENTS`: Enable/disable SCMI sensor events.

- `SCP_ENABLE_SCMI_SENSOR_V2`: Enable/disable SCMI sensor V2 protocol support.

- `SCP_ENABLE_SENSOR_TIMESTAMP`: Enable/disable sensor timestamp support.

- `SCP_ENABLE_SENSOR_MULTI_AXIS`: Enable/disable sensor multi axis support.

- `SCP_ENABLE_SENSOR_EXT_ATTRIBS`: Enable/disable sensor extended attributes
  for multi axis information.
- `SCP_ENABLE_SENSOR_SIGNED_VALUE`: Enable/disable sensor signed values support.

### ATU Configurations

- `SCP_ENABLE_ATU_MANAGE`: Enable/disable ATU management support. This option
  should be enabled only if the MSCP has permissions to configure the ATU.

- `SCP_ENABLE_ATU_DELEGATE`: Enable/disable ATU delegation support. This option
  should be enabled when the ATU is not managed directly by the firmware.

### Product Specific Configurations

- `SCP_ENABLE_AE_EXTENSION`: Enable/disable AE extension. This
  should be enabled, if the processor has an AE extension.

### Performance Configurations

- `SCP_ENABLE_PLUGIN_HANDLER`: Enable the Performance Plugin handler extension.

- `SCP_TARGET_EXCLUDE_SCMI_PERF_PROTOCOL_OPS`: Allow conditional inclusion of
  SCMI Performance commands operations. This allows platforms to include only
  the core Perf and FastChannels without the commands ops (for ACPI-based
  systems).

It can also be used to provide some platform specific settings.
e.g. For ARM Juno platform. See below

- `SCP_ENABLE_SCMI_PERF_FAST_CHANNELS`: Enable/disable Juno SCMI-perf
  Fast channels.
