##################################
Authenticated Debug Access Control
##################################

************
Introduction
************

Introducing security in debug is about making sure that only authorized people
have access to select parts of firmware and hardware. The Trusted Firmware-M
software implementation contained in this project is designed to be a
reference implementation.

ADAC aims at making sure that debug capabilities do not become attack vectors.
Debug security cannot be an afterthought when designing an SoC and the kind of
debug solution needed is driven by the threat models for the device use case.

The ADAC architecture is designed to be flexible to meet varying vendor needs,
adaptable to work with many different hardware and software components, and
scalable from small embedded or IoT systems to complex server environments.
At the same time, it strives to be simple and resilient against attack.

`Authenticated Debug Access Control`_ (ADAC).

**********
Components
**********

The repository contains software componenets, of the ADAC protocol, towards
the target side. The following components are included in this repo:

* Secure Debug Authenticator (SDA)
* ADAC protocol core
* Supported target platforms
    * trusted-firmware-m: dipdha platform
* Transport layer
    * `SDC-600 Secure Debug Channel`_

********************
Directory structures
********************

- psa-adac
    - core
    - sda
- target
    - target name
        - files implementing platform interface of psa-adac/core and psa-adac/sda
        - psa_adac_platform.h: place where platform specific incoming and outgoing calls can be declared
- transport
    - files implementing transport interface of psa-adac/core and psa-adac/sda
    - various implementations of transport layer for communication with the host can be hosted here
- template_hal_files
    - template files for hal api definitions


************************
Integration instructions
************************

Build options
=============

Build from the adac repository
----------------------------------

Configure:

.. code-block::

    cmake -B <build_dir> -S . -DCMAKE_BUILD_TYPE=Debug -DPSA_ADAC_TARGET=<target_name> -DPSA_ADAC_MBEDTLS_INCLUDE=<path to mbedtls include>

Build and install:

.. code-block::

    cmake --build <build_dir> -- install

Static library will be installation at:

.. code-block::

    <build_dir>/install/lib/lib_<target_name>.a

Build as cmake target
---------------------

.. code-block::

    add_subdirectory(${PLATFORM_PSA_ADAC_SOURCE_PATH} ${PLATFORM_PSA_ADAC_BUILD_PATH})
    target_link_libraries(<target_name>-psa-adac
        PRIVATE
            <caller of adac entry point>
    )

Configuration variables
=======================

PSA_ADAC_TOOLCHAIN
------------------
When OFF, the build will not include toolchain files from the
adac repository. User can decide to choose the toolchain configuration from the
adac repository or can also provide its own toolchain configuration files.

PSA_ADAC_TARGET
---------------
Name of the target. Support for the target should exist inside
the ./target/ directory.

PSA_ADAC_MBEDTLS_INCLUDE
------------------------
Path to mbedtls include directory (`MBEDTLS Repository`_)

.. code-block::

    <mbedtls>/include


HAL integration
===============

psa-adac/sda and psa-adac/core depends on the following inteface:

platform.h
----------
Defines the interface to the platform. A template file for the inteface
can be found inside template directory.

psa_adac_crypto_api.h
---------------------
Defines the interface to the cryptographic supported required
by the adac protocol implementation.

A target should provide the implementation of these HAL APIs. An example for platform.h
api implementation can be find inside corstone1000 target directory. And example for
crypto api implementaion can be find inside `Trusted-Firmware-M`_ repository. Further
such integration of crypto apis, based on software (ex: mbedtls) as well as based on
hardware accelerattion, can also be hosted as part of this repository.

msg_interface.h
---------------
Defines the interface to the transport layer. The transport layer supports the communication
between host and the target. The file, msg_interface.h, only contains the interface used
by the target, i.e. psa-adac/core and psa-adac/sda. Various implementation of transport
layer can be hosted inside ./trasport/ directory. For ex: corstone1000 uses transport based
on SDC600 COMPORT.

Integration to the secure debug workflow
========================================

The entry function definition to start the secure debug flow and any other dependency,
a target is free to declare such apis inside the file: psa_adac_platform.h

Corstone1000 psa_adac_platform.h is one such example.


Target examples
===============

Build instructions for Corstone1000 platform inside trusted-firmware-m
----------------------------------------------------------------------

Configure:

.. code-block::

    cmake -B <build_dir> -S . -DCMAKE_BUILD_TYPE=Debug -DPSA_ADAC_TARGET=trusted-firmware-m -DTFM_PLATFORM=arm/corstone1000 -DPSA_ADAC_MBEDTLS_INCLUDE=<mbedtls>/include

Build and install:

.. code-block::

    cmake --build <build_dir> -- install

Build library:

.. code-block::

    build/install/lib/libtrusted-firmware-m-psa-adac.a

The library generated contains secure debug support for Corstone1000 platform
which can be linked to Corstone1000's trusted-firmware-m build.

.. _Authenticated Debug Access Control: https://developer.arm.com/documentation/den0101/latest
.. _SDC-600 Secure Debug Channel: https://www.arm.com/products/silicon-ip-system/coresight-debug-trace/sdc-600
.. _MBEDTLS Repository: https://github.com/ARMmbed/mbedtls.git
.. _Trusted-Firmware-M : https://git.trustedfirmware.org/TF-M/trusted-firmware-m.git/

*Copyright (c) 2021, Arm Limited. All rights reserved.*
