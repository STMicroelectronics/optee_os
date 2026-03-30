SCP-firmware - version 2.16
===========================

Copyright (c) 2011-2026, Arm Limited. All rights reserved.

NOTICE on update on SCP Arm Public Repository Contribution Policy
----------

Dear Contributors,

There is a change regarding Arm System Control Processor (SCP) open-source
repository.

After careful consideration, Arm has decided to transition public SCP repository
to a read-only state. This means that we will no longer be accepting external
contributions, including merge requests, issues, or feature submissions.

Going forward, the repository will remain publicly accessible in its current form.
Partners and contributors who wish to continue development are welcome to create
and maintain their own downstream forks based on latest available version of the repository.

We understand that this change may impact your workflows and appreciate your understanding.

Thank you for your support and contributions.

References
----------

[1] [Power Control System Architecture - DEN0050C] (https://developer.arm.com/documentation/den0050/d/?lang=en)

[2] [System Control and Management Interface - DEN0056A] (https://developer.arm.com/documentation/den0056/d/)

Introduction
------------

There is a strong trend in the industry to provide microcontrollers in systems
to abstract various power, or other system management tasks, away from
Application Processors (APs). The Power Control System Architecture (PCSA) [1]
describes how systems can be built following this approach.

The PCSA defines the concept of the System Control Processor (SCP), a dedicated
processor that is used to abstract power and system management tasks away from
application processors.

Similar to the SCP, the Manageability Control Processor (MCP) follows the same
approach with the goal of providing a management entry-point to the System on
Chip (SoC) where manageability is required, such as on a SoC targeting servers.

SCP-firmware provides a software reference implementation for the System Control
Processor (SCP) and Manageability Control Processor (MCP) components found in
several Arm Compute Sub-Systems.

Functionality
-------------

- Initialization of the system to enable application core boot
- Runtime services:
    - Power domain management
    - System power management
    - Performance domain management (Dynamic Voltage and Frequency Scaling)
    - Clock management
    - Sensor management
    - Reset domain management
    - Voltage domain management
- System Control and Management Interface (SCMI, platform-side)
- Support for the GNU Arm Embedded, Arm Compiler 6 and LLVM toolchains
- Support for platforms with several control processors
- Interactive debugging (with Command Line Interface)

Platforms
---------

Various builds of the release have been tested on the following ARM Fixed
Virtual Platforms (FVPs or boards where possible):

- System Guidance for Mobile platform SGM-775 (Version 11.19 Build 30)
- System Guidance for Infrastruture SGI-575 (Version 11.19 Build 30)
- Neoverse N1 reference design (Version 11.6 Build 45)
- System Guidance for Mobile platform SGM-776 (Version 11.19 Build 30)
- Juno reference design (Please contact Arm directly to obtain the Juno FVP)
- RD-N1-Edge Dual-Chip reference design
  (Please contact Arm directly to obtain the RdN1EdgeX2 FVP)
- RD-V1 reference design  (Please contact Arm directly to obtain the RD_V1 FVP)
- RD-V1-MC reference design
  (Please contact Arm directly to obtain the RD_V1_Multichip FVP)
- Renesas R-Car platform
- Morello (Please see Arm's Ecosystem FVPs Developer page)
- Total Compute (tc2) platform
  (Please contact Arm directly to obtain the TC2 FVP)

License
-------

The software is provided under a [BSD-3-Clause license](https://spdx.org/licenses/BSD-3-Clause.html).

The license for CMock can be found in contrib/cmock/git/LICENSE.txt.

The license for unity framework can be found in contrib/cmock/git/vendor/unity/LICENSE.txt.

Getting started
---------------

See user_guide.md for instructions on how to get, install, build and use
SCP-firmware on supported Arm platforms.

CMake
-----

SCP-firmware now supports building with CMake. See doc/cmake_readme.md for
details.

Feedback and Support
--------------------

Arm licensees may contact Arm via their partner managers.

