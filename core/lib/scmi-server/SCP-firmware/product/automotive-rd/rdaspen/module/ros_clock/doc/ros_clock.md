\ingroup GroupPLATFORMModule PLATFORM Product Modules
\addtogroup GroupRoSClock ROS Clock Driver

# Aspen RoS Clock configuration module

This module configures Aspen's rest of the system clock which sends
clocks out to the other parts of the system.

The clocks that are sent out are controlled by the registers and can either
be the REFCLK or one of the PLL clocks from the PLL control block.

The module takes a configuration struct which contains the configurations
for each clock device. That is the clock control register, clock rate,
source, divider, etc.
