\ingroup GroupModules Modules
\defgroup GroupSafetyIslandPlatform SafetyIslandPlatform

# Module Safety Island Platform Architecture

Copyright (c) 2024, Arm Limited. All rights reserved.

## Overview

The Safety Island Platform provides means of receiving inter-processor messages
in order to boot a Safety Island Cluster.

## Module Design

```
        +--------+                      +---------+               +------------------------+               +--------------+
        | DRIVER |                      |TRANSPORT|               | SAFETY ISLAND PLATFORM |               | POWER DOMAIN |
        +---+----+                      +----+----+               +-----------+------------+               +------+-------+
            |                                |                                |                                   |
  Interrupt |                                |                                |                                   |
----------->+---+                            |                                |                                   |
            |   | isr()                      |                                |                                   |
            |   |                            |                                |                                   |
            |<--+                            |                                |                                   |
            |          signal_message()      |                                |                                   |
            +------------------------------->|                                |                                   |
            |                                |      signal_message()          |                                   |
            |                                +------------------------------->|                                   |
            |                                |                                |        For each cluster core      |
            |                                |                                |             set_state()           |
            |                                |                                +---------------------------------->|
            |                                |                                |                                   +------+
            |                                |                                |                                   |      | Set the power domain state of
            |                                |                                |                                   |      | the Safety Island Cluster core
            |                                |                                |            return status          |<-----+
            |                                |                                |<----------------------------------+
            |                                |                                |                                   |
            |                                |transport_release_channel_lock()|                                   |
            |                                |<-------------------------------+                                   |
            |                                |                                |                                   |
            |                                |        return status           |                                   |
            |                                +------------------------------->|                                   |
            |                                |                                |                                   |
            |                                |        return status           |                                   |
            |         return status          |<-------------------------------+                                   |
            |<-------------------------------+                                |                                   |
            |                                |                                |                                   |
            |                                |                                |                                   |
```

## Configuration Example

The following examples demonstrate how to populate the configuration data for
the Safety Island Platform module.

```config_safety_island_platform.c```

```C
static const struct fwk_element
    safety_island_platform_element_table[SI_CL_COUNT + 1] = {
    [SI_CL0_IDX] = {
        .name = "Safety Island Cluster 0",
        .data = &((struct safety_island_cluster_config) {
            .transport_id = FWK_ID_ELEMENT_INIT(
                FWK_MODULE_IDX_TRANSPORT,
                SCP_CFGD_MOD_TRANSPORT_EIDX_BOOT_SI_CLUS0),
            .cluster_layout = {SI_CL0_ID, SI_CL0_CORE_NUM, SI_CL0_CORE_OFS},
        }),
    },
    [SI_CL1_IDX] = {
        .name = "Safety Island Cluster 1",
        .data = &((struct safety_island_cluster_config) {
            .transport_id = FWK_ID_ELEMENT_INIT(
                FWK_MODULE_IDX_TRANSPORT,
                SCP_CFGD_MOD_TRANSPORT_EIDX_BOOT_SI_CLUS1),
            .cluster_layout = {SI_CL1_ID, SI_CL1_CORE_NUM, SI_CL1_CORE_OFS},
        }),
    },
    [SI_CL2_IDX] = {
        .name = "Safety Island Cluster 2",
        .data = &((struct safety_island_cluster_config) {
            .transport_id = FWK_ID_ELEMENT_INIT(
                FWK_MODULE_IDX_TRANSPORT,
                SCP_CFGD_MOD_TRANSPORT_EIDX_BOOT_SI_CLUS2),
            .cluster_layout = {SI_CL2_ID, SI_CL2_CORE_NUM, SI_CL2_CORE_OFS},
        }),
    },
    [SI_CL_COUNT] = { 0 },
};

static const struct fwk_element *get_safety_island_platform_element_table(
    fwk_id_t module_id)
{
    return safety_island_platform_element_table;
}

const struct fwk_module_config config_safety_island_platform = {
    .elements =
        FWK_MODULE_DYNAMIC_ELEMENTS(get_safety_island_platform_element_table),
};
```

Configuration data must also be provided for each Safety Island Cluster in
each of the following modules:
1. Power Domain module
2. PPU module
3. Tansport module
4. Communication Driver such as MHU
