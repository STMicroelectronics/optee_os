\ingroup GroupModules Modules
\defgroup GroupMPMM_V2 Max Power Mitigation Mechanism Version 2(MPMM_V2)

Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.

MPMM_V2 Module Description
==========================

# Overview
This module provides the MPMM (Maximum Power Mitigation Mechanism) algorithm.
This mechanism detects and limits high activity events to assist in processor
power domain dynamic power budgeting and limit the triggering of whole-rail
responses to overcurrent conditions.

The algorithm will monitor MPMM Gear counters and controls the power limits
and gears. Metrics are provided for each domain to the metrics analyzer so
the distributor entity can use it to budget the power by limiting the number of
cores that can execute higher activity workloads and switching to a appropriate
DVFS level.

# MPMM design
Each set of cores supporting MPMM and supplied by a single rail is represented
by a domain. A core inside the domain is handled as sub-element. For each core,
there is a set of MPMM counters and gear registers. The algorithm is split
into two parts. The first part evaluates each core state individually. The
second part evaluates the performance requests for all cores within one domain.
The correct gear settings for each core are then applied and the new power limits
are requested.

# MPMM configuration
To use this module the platform code needs to provide the following
configuration options:

## Gear Weights
Gear weight are the weights of the dynamic power requirement for
each gear on a scale of 100. These weights must be normalised on maximum power
allowed for the domain.

Example table of gear and their expected workloads and corresponding expected
voltage and frequency pair.

| Workload    | Power Spectrum (x) | Expected V/F (normalised on max_pwr) |  MPMM-gear |
|:-----------:|:------------------:|:------------------------------------:|:----------:|
| Dhrystone   |         2x         |        1/1                           |    0       |
| Medium B/W  |         3x         |       0.85/0.9                       |    1       |
| High B/W    |         5x         |       0.7/0.8                        |    2       |
| Power Virus |         7x         |       0.7/0.8                        |    2       |

Using above table we can calculate the gear weights on power using
```math
P_{dyn} ∝ V^{2} \times F
```
```math
Gear_{wt} = V_{exp}^{2} \times F_{exp} \times 100
```
| MPMM-gear  |  Gear Weight  |
|:----------:|:-------------:|
|    0       |     100       |
|    1       |     65        |
|    2       |     40        |

## Core offline factor
Core offline factor is a scalar constant in the power model which compensates
for offline cores. If certain cores are offline, A higher frequency and voltage
would give the MPMM a chance to avoid becoming a bottleneck. The goal for this
scalar constant is to find a balance between the safe limit and the lowest limit.
This will help in re-distribution of different class of instructions optimially
for the next cycle.

## Base Throttling Count (BTC):
BTC represents the maximum steady state MPMM throttling count which a platform
wants to maintain for each core. The algorithm will try to find suitable gear
combinations which keeps the MPMM throttling around this level while trying to
maximize performance. It is a configuration parameter for the platform to tune.
The current policy selects the first gear with counter difference lower than the
BTC i.e If all the MPMM counters differences are below the BTC the lowest gear
is selected.
