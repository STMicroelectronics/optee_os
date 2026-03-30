\defgroup GroupNI_710AE NI_710AE
\ingroup GroupModules

# Module NI_710AE Architecture

Copyright (c) 2025, Arm Limited. All rights reserved.

# Overview
The Arm® CoreLink™ NI‑710AE Network‑on‑Chip Interconnect is a highly
configurable AMBA®‑compliant system-level interconnect that enables functional
safety for automotive and industrial applications. Using multiple routers and
various topology options, you can connect multiple upstream and downstream
devices that use different AMBA protocols to the NI‑710AE.

```ditaa {cmd=true args=["-E"]}
                 +--------------------------------------+
                 |             NI-710AE                 |
                 |                                      |
                 |   Completer Node      Requester Node |
                 |   for Requester       for Completer  |
 +------------+  |   +----------+        +----------+   |    +----------+
 |            |  |   |          |        |          |   |    |          |
 |  AXI/ACE   +->+-->|  ASNI    +--------+  AMNI    +-->+--->| AXI/ACE  |
 |  Completer |  |   |          |        |          |   |    | Requester|
 +------------+  |   +----------+        +----------+   |    +----------+
                 |                                      |
 +------------+  |   +----------+        +----------+   |    +----------+
 |            |  |   |          |        |          |   |    |          |
 |  AHB       +->+-->|  HSNI    +--------+  HMNI    +-->+--->|  AHB      |
 |  Completer |  |   |          |        |          |   |    | Requester|
 +------------+  |   +----------+        +----------+   |    +----------+
                 |                                      |
                 |                       +----------+   |    +----------+
                 |                       |          |   |    |          |
                 |                       |  PMNI    +-->+--->|  APB      |
                 |                       |          |   |    | Requester|
                 |                       +----------+   |    +----------+
                 |                                      |
                 +--------------------------------------+
```

# Module Design

This module implementation supports Node Discovery and programming Access
Protection Units.

# Node Discovery

To build the discovery tree, the discovery process starts at the base address
of the configuration space, PERIPHBASE. Then the discovery uses pointer values
to determine the number and type of each interconnect domain or component,
their attributes, and the location of the configuration registers. The software
can use this information to access these registers for configuration purposes.

The NI\-710AE has the following types of nodes:
- Global Configuration Node
- Voltage Domain
- Power Domain
- Clock Domain
- Component (ASNI, AMNI, PMNI, PMU etc.)
- Subfeature


## Example Discovery 
```ditaa {cmd=true args=["-E"]}
+------------------------------------+
| Global configuration node registers|
|                                    |
| Number of Voltage Domains (VDs)    |
| +------------------------------+   |
| | VD 0 pointer                 |   |
| | VD 1 pointer                 |   |
| | ...                          |   |
| | VD n pointer                 |   |
| +------------------------------+   |
| Global registers                   |
+------------------------------------+
        |
        v
+------------------------------------+
| VD registers                       |
| Number of Power Domains (PDs)      |
| +------------------------------+   |
| | PD 0 pointer                 |   |
| | PD 1 pointer                 |   |
| | ...                          |   |
| | PD n pointer                 |   |
| +------------------------------+   |
| VD-specific registers              |
+------------------------------------+
        |
        v
+------------------------------------+
| PD registers                       |
| Number of Clock Domains (CDs)      |
| +------------------------------+   |
| | CD 0 pointer                 |   |
| | CD 1 pointer                 |   |
| | ...                          |   |
| | CD n pointer                 |   |
| +------------------------------+   |
| PD-specific registers              |
+------------------------------------+
        |
        v
+------------------------------------+
| CD registers                       |
| Number of components in CD         |
| +------------------------------+   |
| | Component 0 pointer           |  |
| | Component 1 pointer           |  |
| | ...                           |  |
| | Component n pointer           |  |
| +------------------------------+   |
| CD-specific registers              |
+------------------------------------+
        |
        v
+------------------------------------+
| Component registers                |
| Component type                     |
| Interface ID                       |
| Number of subfeatures in component |
| +------------------------------+   |
| | Subfeature 0 type            |   |
| | Subfeature 0 pointer         |   |
| +------------------------------+   |
| Component-specific registers       |
+------------------------------------+
        |
        v
+------------------------------------+
| Subfeature registers               |
| Subfeature-specific registers      |
+------------------------------------+
```

# APU Programming

APUs provide access control on both the requester and completer sides of the
interconnect. Requester-side APUs are in xSNIs and completer-side APUs are in
xMNIs. On the requester side, you can use an APU to control requester access to
specific memory regions and provide isolation between requesters. On the
completer side, you can use the APU outputs in a downstream firewall to provide
access protection to and from completer peripherals.

