\ingroup GroupPLATFORMModule PLATFORM Product Modules
\addtogroup GroupClusterControl Aspen Cluster Control driver

# RD-Aspen Cluster Control configuration module

This module configures the RVBAR in RD-Aspen's cluster control registers.

The module takes a configuration struct which contains the memory address for
each cluster control region and the RVBAR values to program. It programs the
same RVBAR for all cores for all clusters.
