\ingroup GroupModules Modules
\defgroup GroupThermal Thermal Management

# Thermal Management Architecture

Copyright (c) 2024, Arm Limited. All rights reserved.


## Overview

The Thermal Power Estimator is a module designed to compute the maximum
allocatable power for a platform based on thermal conditions. It forms part of
the Metrics pipeline framework and interacts with other components to provide
accurate power estimation leveraging temperature data and PID control feedback.

## Features

- **Thermal-Aware Power Estimation**: Utilizes thermal input from sensors and
  PID feedback to estimate maximum permissible power.
- PID Integration: Interfaces with a separate PID controller module for
  closed-loop feedback.
- Metrics Analyzer Integration: Acts as a power limit provider for the Metrics
  Analyzer service.

## Architecture diagram

                                Thermal Design
    Current temp                 Power (TDP)
         |                            |
         |                            |
         |                            |
         v                            v
       +-+-+     +----------+       +-+-+
       |   +---->+ PID Ctrl +------>+   +-------> Maximum allocatable power
       +-+-+     +----------+       +---+
         ^
         |
         |
         |
    Control temp


## Algorithm

The Thermal Power Estimator is typically deployed as part of a broader power
management strategy. The following sequence describes its operation within
the Metrics Analyzer framework:

1. The module retrieves the current temperature from the sensor.
2. PID control feedback is fetched to determine power adjustments.
3. Computes and provides the maximum allocatable power based on the above
   inputs.
4. Supplies the computed power limit to the Metrics Analyzer.
