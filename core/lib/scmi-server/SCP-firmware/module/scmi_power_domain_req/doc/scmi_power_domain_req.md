\ingroup GroupModules Modules
\defgroup GroupSCMIPowerDomainRequester SCMI Power Domain Requester

# SCMI Power Domain Requester architecture

Copyright (c) 2024, Arm Limited. All rights reserved.

## Overview

The SCMI Power Domain Requester module is used to send SCMI Power Domain
protocol commands, to set and get power domain state.

This is needed when we have a power domain tree distributed on two or more
SCPs, one of them will act as both agent and platform.

The main SCP will send the SCMI power domain request to secondary SCP(s) to
control external PDs (external in this context means that it is controlled by
other SCP/LCP in systems that has multiple SCPs/LCPs)

Also this can be used if management control processor (MCP) is acting as agent
and wants to send requests to change power domain status to SCP.

   +-------+     +------------------------------+ +----------------------+
   |       |     |  +----------+      COMSS     | |  +----------+  CPNSS |
   |       |     |  |   PD0    |                | |  |   PD3    |        |
   | Agent |     |  +----------+                | |  +----------+        |
   |(OSPM) |     |  +----------+   +----------+ | |  +----------+        |
   |       |     |  |    PD1   |   |   PD2    | | |  |    PD4   |        |
   |       |     |  +----------+   +----------+ | |  +----------+        |
   |       |     +------------------------------+ +----------------------+
   |       |                     |                           |
   |       |                     |                           |
   |       |-----------   +---------------+           +---------------+
   +-------+          |   |      SCP0     |           |      SCP1     |
                      --> |               |---------> |               |
                          +---------------+           +---------------+


## Architecture

The following sequence diagram demonstrates the communication between the
involved modules starting from a agent sending SCMI Power domain commands,
until it gets the response.

                        Sequence diagram
+-------+   +--------------+  +---------+    +------------------+   +------+
| Agent |   | SCMI Pow dom |  | Pow Dom |    | SCMI Pow Dom Req |   | scmi |
+-------+   +--------------+  +---------+    +------------------+   +------+
     | state_set()  |               |                    |                |
     |------------->| state_set()   |                    |                |
     |              |-------------->|                    |                |
     |              |               | set_state()        |                |
     |              |               |------------------->|                |
     |              |               |                    |                |
     |              |               |                    | scmi_send()    |
     |              |               |                    |--------------->|
     |              |               |                    | scmi_response()|
     |              |               |                    |<---------------|
     |              |               |                    |                |
     |              |               |<-------------------|                |
     |              |               | report_power_      |                |
     |              |               | state_transition() |                |


## Configuration

Power Domain module configuration will add elements for the external PDs, and
these elements will bind to SCMI Power Domain Requester.

SCMI Power Domain Requester will have element for each external PD also to be
binded with the Power Domain elements.

                            Configuration example
    +-----------------------------------------------------------------------+
    |                                SCP0                                    |
    |-----------------------------------------------------------------------|
    |                                                                       |
    | +-------------+ +--------------+ +----------------+   +-------------+ |
    | |Client module| | Power Domain | |SCMI Pow dom Req|   | SCMI        | |
    | |-------------| |--------------| |----------------|   |-------------+ |
    | | +---------+ | | +----------+ | | +------------+ |   | +---------+ | |
    | | |element_A|-+-+>|element_B |-+-+>| element_C  | |-+-+>|element_D| | |
    | | +---------+ | | +----------+ | | +------------+ | | | +---------+ | |
    | +-------------+ +--------------+ +----------------+ | +-------------+ |
    |                                                     |                 |
    +-----------------------------------------------------+-----------------+
                                                          |
                               +-----------------------+  |
                               |       SCP1            |  |
                               |-----------------------|  |
                               | +-------------------+ |  |
                               | | SCMI Power Domain |<+--+
                               | |-------------------| |
                               +-----------------------+

Configuration parameters are:
- start_state : initial state of the external PD
- domain_id : the power domain number in the secondary SCP
- service_id : scmi service number to send command and receive response.
- pd_element_id : the config element in the Power domain module (element_B in
above example)
- sync_flag : wether we need t send syncrhonus or asynchronus request for PD.


## Software APIs

APIs of SCMI System Power Requester module should follow the mod_pd_driver_api
defined in the mod_power_domain.h and will be provided to the power domain
module in the process_bind_request()

- set_state : to set the external pd with a requested state
- get_state : to get the external pd current state
- reset : to reset the external pd (Not implemented for now, as no usecase)
- shutdown : will do nothing, this is called when SCMI system power request for
reset is received.

## Limitations and Notes

get_state will return a cached value of the state, so it will not send SCMI
command, he cached values is stored when a response back received with
SCMI_SUCCESS.
