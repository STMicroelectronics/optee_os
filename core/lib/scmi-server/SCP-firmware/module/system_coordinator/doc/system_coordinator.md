\ingroup GroupModules Modules
\defgroup GroupSystemCoordinator System Coordinator

# Coordinator

Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.

## Overview

System coordinator is a module that manages the power management system.
The system coordinator is responsible for managing each phase of the power
management.

Each phase is an API call to a module. The phase API and time are configurable
elements. Coordinator calls the next phase after the phase timer alarm elapses.
The order of the phases follows the element order.

The phase's API shall be in this format for system coordinator to call:

    int phase_handler(void);

A cycle timer alarm periodically notifies the coordinator to call all the phases
again. The cycle time is the sum of all the phase time.The cycle time is 
configurable and must be more than the sum of all phase time.

If a phase timer from the previous cycle elapses when a new cycle has begun,
the coordinator will discard that phase. Only phases from the current
cycle will be processed. For example, assuming there are three phases A, B and C
and two cycles 1 and 2, the sequence are A1->B1->A2->C1->B2->C2. C1 will be
discarded because A2 signifies cycle 2 had begun and only phases from cycle 2
will be processed.

## Phases

The timing diagram below illustrates the phases in a cycle. Although this
diagram depicts only three phases, the coordinator is capable of calling
multiple phases.

```
[Cycle time]  |<----------------x---------------->|<----------------x---------------->|
              |                                   |                                   |
[Phase time]  |<----a---->|<----b---->|<----c---->|<----a---->|<----b---->|<----c---->|
              |           |           |           |           |           |           |
              |           |           |           |           |           |           |
              +-------+   +-------+   +-------+   +-------+   +-------+   +-------+   |
[Module]      | Mod 1 |   | Mod 2 |   | Mod 3 |   | Mod 1 |   | Mod 2 |   | Mod 3 |   |
              +-------+---+-------+---+-------+---+-------+---+-------+---+-------+---+
```

Phase Time (*a* and *b*): Configurable values used by the phase timer to trigger
the next phase. If a phase time is set to 0, the phase timer will be skipped,
and an event will be immediately sent to initiate the next phase.

Last Phase Time (*c*): Configurable value representing the duration of the last
phase. Since it is the final phase in the cycle, it does not trigger any further
phase transitions. Its value is used to ensure that the sum of all phase times
does not exceed the configured cycle time

Cycle Time (*x*): configurable value representing the total duration of the
periodic cycle. The cycle time must be greater than the sum of all phase times:
$x>a+b+c$
