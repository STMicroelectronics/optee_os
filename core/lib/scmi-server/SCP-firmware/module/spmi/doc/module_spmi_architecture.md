\ingroup GroupModules Modules
\defgroup GroupSPMI SPMI HAL

Module SPMI Architecture
========================

# Overview                                  {#module_spmi_architecture_overview}

System Power Management Interface (SPMI) is a two-wire interface that connects
the power management IC to the power management controller of the SoC.
This module implements a Hardware Abstraction Layer (HAL) API for SPMI
transactions.

# Architecture                          {#module_spmi_architecture_architecture}

The SPMI module provides an abstraction layer to send and receive SPMI commands
over the SoCs external SPMI bus. The SPMI module provides the interface to SPMI
controller driver, thus decoupling the SPMI controller driver platform implementation
from the client SPMI device driver.

The SPMI module defines a driver interface on which it relies to send and
receive SPMI transactions to/from the bus. The driver interface is also
used to indicate reception of SPMI transaction from a request capable completer.

A response event notifies the caller of the transaction completion or receipt
of a transmission from request capable completer.

The SPMI HAL module design is based on the I2C HAL module design.

The SPMI module provides support for concurrent accesses to an SPMI bus. If a
SPMI transaction is requested on an SPMI bus while the bus is busy processing
another transaction, the transaction request is queued. The queuing and
processing of SPMI transaction requests follow a FIFO logic. When a transaction
is completed, the processing of the transaction request at the head of the
queue, if any, is initiated.

# Flow                                          {#module_spmi_architecture_flow}

The following schematic describes the transaction flow for an SPMI controller
transmission. It is assumed that all the sequence occurs without any
concurrent access and that the driver handles the transmission asynchronously.
The flow for a reception is similar.

    Client            SPMI HAL        SPMI Driver     SPMI ISR (Driver)
      |                 |                 |               |
      |   spmi_read     |                 |               |
     +-+                |                 |               |
     | +-------------->+-+                |               |
     | |               | +- - +           |               |
     | +<--------------+-+    |process_   |               |
     +-+ FWK_PENDING    |     |event E1   |               |
      |                 |     |           |               |
      |                +-+<- -+           |               |
      |                | |                |               |
      |                | |                |               |
      |                | | send_command   |               |
      |                | +-------------->+-+              |
      |                | +<--------------+-+              |
      |                +-+  FWK_PENDING / |               |
      |                 |   FWK_SUCCESS   |               |
      |                 |                 |               |
      |                 |                 |  transaction +-+
      |                 |                 |  _completed  | |
      |                +-+<---------------+--------------+ |
      |            +- -+ |                |              | |
      |  process_  |   +-+                |              | |
      |  event E2  |    |                 |              | |
      |            +- >+-+                |              +-+
      |                | |                |               |
     +-+<- - - - - - - +-+                |               |
     +-+  process_      |                 |               |
      |   event R1      |                 |               |

    E1   : Request read event
    E2   : Request completed event
    R1   : Response to the request event E1
    ---> : Function call/return
    - -> : Asynchronous call via the event/notification interface

The client calls *spmi_read* API of the SPMI module.
This function creates and sends the SPMI device request event which
defines the selected target on the bus and the data to be transmitted.
It returns FWK_PENDING to the client.

When processing the request event, the SPMI module initiates the transfer by
programming the SPMI controller through the *send_command* API of the
SPMI driver. The driver module can process the request synchronously or
asynchronously.

In the case where the processing of the request completes
immediately (synchronous handling by the driver) and another transaction request
is pending, the processing of this last transaction request is not initiated
immediately to avoid multiple transactions being processed within the same event
processing. A reload event is then sent and the processing of the next pending
request is initiated as part of the processing of the reload event.

In case of asynchronous processing by the driver, the driver calls the
*transaction_completed* API of the SPMI module. The function creates and sends
the SPMI request completed event.

SPMI HAL will enter a PANIC state, if its process_event function is called with
an unsupported event type. Once the SPMI HAL is in panic state, it will not
accept further requests. PANIC state can only be reached by a software error
or corruption where an event was sent to SPMI HAL from code outside of SPMI HAL
It should never be in panic state in normal operation.

For both synchronous and asynchronous processing by driver, the response is
sent from HAL to client as per the
[deferred response architecture](doc/deferred_response_architecture.md)
