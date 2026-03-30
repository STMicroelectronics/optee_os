# Build System

## Overview

The SCP/MCP software build system uses CMake and is organized with a main
__CMakeLists.txt__ file at the top level, complemented by separate
__CMakeLists.txt__ files in each module and target for individual products. This
structure allows you to define platforms, add specific modules to them, and
configure them to generate the final firmware binaries. The build system also
includes a helper top-level __Makefile.cmake__ file wrapper to generate all
required images for a target.

This documentation covers the use of the build system when creating products,
firmware, and modules.

For details on how to build an existing product, please refer to the
documentation using the build system's "help" parameter:

    $> make -f Makefile.cmake help

## Product and Firmware Hierarchy

A product is a collection of firmware. All existing products are located under
the __product__ directory and must adhere to the following hierarchy:

    <root>
     └─ product
        └── <product>
            ├── product.mk
            ├── include
            │   └── <product level header files...>
            │── src
            │   └── <product level source files...>
            ├── module
            ├── <firmware_1>
            │   └── <firmware 1 level configuration files...>
            │   └── Firmware.cmake
            │   └── CMakeLists.txt
            └── <firmware_2>
                └── <firmware 2 level configuration files...>
                └── Firmware.cmake
                └── CMakeLists.txt

> **NOTE**: There is functionality for building a product located outside of
> this root directory. Read below for instructions.

Different products that share similar files can be grouped into
product_group. Shared files are located under __common__ directory, while
all products' specific code are kept under product directory. The following
hierarchy shows the grouped products under __product__ directory:

    <root>
     └─ product
            └──<product_group>
                ├── common
                │   ├── include
                │   │   └── <common header files for products...>
                │   │── src
                │   │   └── <common source files for products...>
                │   └── module
                └── <product>
                    ├── product.mk
                    │── include
                    │   └── <product level header files...>
                    │── src
                    │   └── <product level source files...>
                    │── module
                    ├── <firmware_1>
                    │   └── <firmware 1 level configuration files...>
                    └── <firmware_2>
                        └── <firmware 2 level configuration files...>

__Note:__ The names of the \<product\> and \<firmware\> directories must not
contain spaces.

The product.mk is described in the following sections.

### The product.mk File

The product.mk file defines a product for the build system
listing all build images.

The following parameters are mandatory:

* __BS_PRODUCT_NAME__ - Human-friendly name for the product. The content of this
  variable is exposed to the compilation units.
* __BS_FIRMWARE_LIST__ - List of firmware directories under the current
  product.

### The Firmware.cmake File

Each firmware target is configured using a dedicated Firmware.cmake file,
which defines the settings, parameters, and dependencies specific to it.

The Firmware.cmake file sets various build options such as toolchain,
architecture, and features to be enabled or disabled. It also specifies the
list of modules and their paths, ensuring that the necessary components are
included and initialized in the correct order during the firmware build.

# Module

Modules are the building blocks of a product firmware. Modules can be
implemented under the modules/ directory at the root of the project, or they
can be product-specific and implemented under the product/\<product\>/modules
directory. In either case, modules have the following directory structure:

    <module root>
     └── <module>
          ├── include
          │   └── <header files...>
          ├── src
          │    ├── Makefile
          │    └── <source files...>
          ├── lib
          │    └── mod_<module>.a
          ├── test
          │    └── <unit test files...>
          ├── doc
          │    └── <documentation files...>
          ├── CMakeLists.txt
          └── Module.cmake

Only one of the 'src' or 'lib' directories is required. When building a
firmware, if the 'src' directory is present then the module library is built
from the module source code and the 'lib' directory, if present, is ignored.
When only the 'lib' directory is supplied, the module's pre-built static library
is used when building a firmware.

__Note:__ The name of the \<module\> directory must not contain spaces.

The name of the \<module\> directory is used in __SCP_MODULES__ by `cmake`

The __doc__ directory is optional and may contain markdown (.md) based
documentation.

## Module Code Generation

When a firmware is built there are two prerequisite files that will be generated
by the build system.
* fwk_module_idx.h: Contains an enumeration of the indices of the modules that
    make up the firmware. The ordering of the module indices in the enumeration
    within fwk_module_idx.h is guaranteed to follow the order of the module
    names in the SCP_MODULES list within Firmware.cmake file. This same
    ordering is used by the framework at runtime when performing operations
    that involve iterating over all the modules that are present in the
    firmware, such as the fwk_module_init_modules() function in fwk_module.c.
* fwk_module_list.c: Contains a table of pointers to module descriptors, one
    for each module that is being built as part of the firmware. This file and
    its contents are used internally by the framework and should not normally
    be used by other units such as modules.

# Interface

Interfaces provide a common set of abstractions and ensure consistent
implementation across different modules. They define the APIs and data
structures used by high-level modules, which do not need to be aware of the
low-level module implementation details. This implies that low-level modules can
be replaced without affecting the high-level ones.

Each interface has its own directory within the interface folder, which contains
header files in the main directory for including the interface's API.
Additionally, the `doc` directory provides detailed documentation on how to use
the interface effectively.

## Folder Structure

The `interface` folder follows a structure similar to the following:

```
interface/
├── <interface name 1>
│   ├── interface_<interface name 1>.h
│   └── doc
│       └── <documentation files...>
├── <interface name 2>
│   ├── interface_<interface name 2>.h
│   └── doc
│       └── <documentation files...>
└── ...
```

- Each interface is organized in its own subdirectory, following the naming
convention `interface_<interface name>.h`.
- The subdirectory within each interface contains the header files that
define the interface's API.
- The `doc` directory within each interface contains optional documentation
specific to that interface.

## Usage

To use an interface it requires to add the `include` path in the respective
module `CMakeLists.txt` file.

```cmake
target_include_directories(${SCP_MODULE_TARGET} PUBLIC
                  "${CMAKE_SOURCE_DIR}/interface/<interface name>")
```
Then simply include `interface_<interface name>.h` file to use the interface API
definitions.

It is important to refer to the interface-specific documentation provided in the
`doc` directory of each interface for detailed information on how to use the
interface and any specific considerations or limitations.

# Building SCP Firmware: A Step-by-Step Guide Using CMake

The SCP-firmware project uses CMake, a build system generator for C-like
languages, to configure and generate its build system. This section is
dedicated to getting you familiarized with the basics of our CMake build system.

For basic usage instructions on CMake, see [Running CMake].

[Running CMake]: https://cmake.org/runningcmake

## Prerequisites

Please follow the prerequisites from [user_guide.md](user_guide.md)

## Building
Make sure you have updated submodule in the repo
```sh
$ git submodule update --init --recursive
```

CMake restricts the build process to a single firmware target
at a time, for example:

```sh
cmake -B /tmp/build  -DSCP_FIRMWARE_SOURCE_DIR:PATH=juno/scp_romfw
```

This will configure cmake to build firmware for Juno platform scp_romfw
firmware, where:
- `/tmp/build`: is the directory where the build will be generated.
- `juno/scp_romfw`: is the firmware to build for the platform.

```sh
cmake --build /tmp/build    # will build the configured cmake
```

In some case like running SCP-firmware with OP-TEE, the firmware is a
static library that is then linked to a larger binary. Instead of
providing all the static libraries that have been built by cmake and
which can change with the configuration of your product, you can
gather all of them in a single one.

> ```sh
> cmake --build /tmp/build --target ${SCP_FIRMWARE_TARGET}-all
> will build the configured cmake and append all static librairies in
> lib${SCP_FIRMWARE_TARGET}-all.a
> ```

For ease of building the product, the make wrapper can be used with the below
command to build all firmwares for a product

```sh
$ make -f Makefile.cmake PRODUCT=juno
```

By default all the binaries will be located at `./build/<platform>` directory.
e.g. in above case it will be under `./build/juno directory`.

## Build and execute framework, module, and product unit tests
See below to build and execute tests

```sh
$ make -f Makefile.cmake test
```

Alternatively, execute just framework, module, or product tests

```sh
$ make -f Makefile.cmake fwk_test
```

```sh
$ make -f Makefile.cmake mod_test
```

```sh
$ make -f Makefile.cmake prod_test
```

See unit_test/user_guide.md for more information on configuring
module tests.

> **LIMITATIONS** \
> ArmClang toolchain is supported but not all platforms are working.

> **NOTE**: Read below documentation for advanced use of development environment
> and CMake build options.

## Build configuration

### Firmware

- Using cmake command line option

  ```sh
  cmake -B /tmp/build  -DSCP_FIRMWARE_SOURCE_DIR:PATH=juno/scp_ramfw\
  -DSCP_ENABLE_DEBUG_UNIT=TRUE
  ```

  It will configure cmake to build firmware for Juno platform `scp_ramfw`
  firmware. with debug unit enabled
  where:
  - `/tmp/build`:  is the directory where the build will be generated.
  - `juno/scp_romfw`: is the firmware to build for the platform.

  ```sh
  cmake --build /tmp/build    # will build the configured cmake
  ```

- Using ccmake or cmake-gui

  ```sh
  $ ccmake -B /tmp/build  -DSCP_FIRMWARE_SOURCE_DIR:PATH=juno/scp_ramfw
  ```

  It will configure cmake to build firmware for Juno platform scp_ramfw
  firmware with debug unit enabled
  where:
  * `/tmp/build`:  is the directory where the build will be generated.
  * `juno/scp_romfw`: is the firmware to build for the platform.

  It opens a curses based UI. If `/tmp/build` is not present i.e. if this
  is the first time the configuration is generated, select `'c'` (Configure)
  and then modify the options as desired. After selecting the options
  select `'c'` (Configure) and `'g'`(Generate) to generate the build system.

  ```sh
  $ cmake --build /tmp/build    # will build the configured cmake
  ```

  Build options can subsequently tuned using below command
  ```sh
  $ ccmake /tmp/build
  ```

- Using default value set for a specific option in `Firmware.cmake`.

  Every SCP firmware specific option(e.g. `SCP_ENABLE_XXX`) has a corresponding
  `_INIT` variable in respective `Firmware.cmake` and can be modified before
  build generation

  e.g.
  For Arm Juno platform `scp_ramfw/Firmware.cmake` following value can be edited
  manually before build configuration is generated.

  ```
  set(SCP_ENABLE_DEBUG_UNIT_INIT FALSE)
  ```

  **NOTE** In this method, if value needs to be re-modified then old build
  folder must be manually deleted. Subsequent re-run without deleting old
  configuration will not update the earlier configured value, See CACHE
  variables in CMake documentation.

  **NOTE**: Enabling/disabling option may result in inclusion or exclusion of
  a particular module in the firmware.
  e.g. See module/resource_perms/Module.cmake and note following
  ```cmake
  if(SCP_ENABLE_RESOURCE_PERMISSIONS)
    list(APPEND SCP_MODULES "resource-perms")
  endif()
  ```
  The above code will include `resource-perms` module in the firmware only if
  `SCP_ENABLE_RESOURCE_PERMISSIONS` is enabled. This also means, define
  `BUILD_HAS_MOD_RESOURCE_PERMS` will available only if this option is enabled.

### Variables

> **NOTE:** If you are using a GUI-based tool, how you configure these variables
> may differ from how this section describes. For instance, `cmake-gui` and
> `cmake` both present the options you may configure by default, and you do not
> need to do anything else. If you are using an IDE, you may need to modify
> the generated `CMakeCache.txt` file, or your IDE may offer an integrated way
> to modify these variables.

The various configuration variables can be listed on the command line with:

```sh
$ cmake "${SCP_SOURCE_DIR}" -LAH
```

This will give you a list of the options available to you as well as a helpful
description and their type.

You can override the default values for any of the settings you see with the
`-D` CMake option:

```sh
$ cmake "${SCP_SOURCE_DIR}" -B "${SCP_BUILD_DIR}" \
  -D${VARIABLE_NAME}:${VARIABLE_TYPE}="${VARIABLE_VALUE}"
  ...
```

> **NOTE:** If you do not see a variable that you expect to see, it is likely
> because the firmware has forcibly overridden it.

### Toolchain

If you wish to adjust the toolchain used to build the firmware, you may provide
the `SCP_TOOLCHAIN` cache variable. Toolchain support is on a per-firmware
basis, and the toolchains supported by the firmware are given by
`Toolchain-${SCP_TOOLCHAIN}.cmake` files found in the firmware.

When `SCP_TOOLCHAIN` is set as `Clang` `SCP_LLVM_SYSROOT_CC` must be defined.

For example, a firmware supporting both GCC and Arm Compiler 6 may offer a `GNU`
toolchain and an `ArmClang` toolchain (`Toolchain-GNU.cmake` and
`Toolchain-ArmClang.cmake`). In this situation, for GCC you might use:

```sh
$ cmake "${SCP_SOURCE_DIR}" -B "${SCP_BUILD_DIR}" \
  -DSCP_TOOLCHAIN:STRING="GNU"
  ...
```

Or for Arm Compiler 6:

```sh
$ cmake "${SCP_SOURCE_DIR}" -B "${SCP_BUILD_DIR}" \
  -DSCP_TOOLCHAIN:STRING="ArmClang"
  ...
```

Alternatively, if you wish to use a [custom toolchain file], you may provide
[`CMAKE_TOOLCHAIN_FILE`]:

```sh
$ cmake "${SCP_SOURCE_DIR}" -B "${SCP_BUILD_DIR}" \
  -DCMAKE_TOOLCHAIN_FILE:FILEPATH="${SCP_TOOLCHAIN_FILE}"
  ...
```

[custom toolchain file]:
        https://cmake.org/cmake/help/latest/manual/cmake-toolchains.7.html
[`CMAKE_TOOLCHAIN_FILE`]:
        https://cmake.org/cmake/help/latest/variable/CMAKE_TOOLCHAIN_FILE.html

> **NOTE:** An out-of-tree firmware may be configured per these instructions by
> providing a firmware directory outside of the project source directory.

## Build a Product Outside of the Root Directory
The following instructions are for building a product that is located outside
of the root directory.

### Change toolchain files

First, the `Toolchain-(GNU/Clang/ArmClang).cmake` file for the product needs to be
changed. The variable `${CMAKE_CURRENT_LIST_DIR}` used in the Toolchain file will
point to a directory in the external product directory as opposed to the
SCP-firmware root directory. So, the `Toolchain-(GNU/Clang/ArmClang).cmake` 
file should be changed to something similar to the following:

```
include_guard()

set(CMAKE_SYSTEM_PROCESSOR "<TBD>")
set(CMAKE_TOOLCHAIN_PREFIX "<TBD>")

set(CMAKE_ASM_COMPILER_TARGET "<TBD>")
set(CMAKE_C_COMPILER_TARGET "<TBD>")
set(CMAKE_CXX_COMPILER_TARGET "<TBD>")

if(NOT DEFINED ENV{CMAKE_TOP_DIR})
    set(ENV{CMAKE_TOP_DIR} "${CMAKE_SOURCE_DIR}")
endif()

set(CMAKE_TOP_DIR "$ENV{CMAKE_TOP_DIR}")
include("${CMAKE_TOP_DIR}/cmake/Toolchain/<TOOLCHAIN>-Baremetal.cmake")
```

An environment variable is needed here because there is an LTO try_compile
iteration of this file which does not include cached variables. 
It needs to be stored on the first iteration as each try_compile iteration
will be performed in different directories.

### Add PRODUCT_PATH parameter to the make command
The `PRODUCT_PATH` parameter should be assigned the directory in which the
product resides. For example, if `neoverse-rd/sgi575` is an external product
located in the directory: `~/foo/ExternalProducts`, then the command
should be as follows:
```
make -f Makefile.cmake PRODUCT_PATH=~/foo/ExternalProducts PRODUCT=neoverse-rd/sgi575 MODE=debug
```

# Development Environments

Along with core CMake build system, also provided is a option to generate
a build environment, please see details below.

## Visual Studio Code Development Containers

If you use Visual Studio Code, you may also work directly within a
pre-configured development container. See the [official tutorial] if you are
unfamiliar with the process of developing within containers.

[official tutorial]:
          https://code.visualstudio.com/docs/remote/containers-tutorial

## Docker

> **NOTE**: Using Docker in SCP build might show some errors.

The SCP-firmware project includes a [`Dockerfile`] which can be used to set up
an environment containing all of the necessary prerequisites required to build
and test a firmware.

This Dockerfile has four variants:

- `ci`: A continuous integration variant, which provides the tooling required
  for automating builds and quality assurance processes.
- `dev`: A development variant, which includes additional tools for developers
  accessing the container directly.

In case you want to manage your development environment directly through Docker,
you can build the development container with the following:

```sh
$ docker build -t scp-firmware --target=dev -f docker/Dockerfile .
```

You can then begin an interactive login shell with:

```sh
$ docker run -v $(pwd):/scp-firmware -v ~/.gitconfig:/home/user/.gitconfig \
    -e TERM -e ARMLMD_LICENSE_FILE -it scp-firmware /bin/bash
```

Alternatively, you can run commands directly from within the container with:

```sh
$ docker run -v $(pwd):/scp-firmware -v ~/.gitconfig:/home/user/.gitconfig \
      -e TERM -e ARMLMD_LICENSE_FILE -it scp-firmware pwd

```

[`Dockerfile`]: ./docker/Dockerfile

## Arm Compiler 6 support for Docker

To use Arm compiler 6 in the container it will be necessary to mount the
directory into `/opt/arm-compiler-6`.

## Quality Assurance

The SCP-firmware project has adopted a number of quality assurance tools in
order to programmatically measure and increase code quality, and most of these
tools can be invoked directly through the build system for convenience with the
following:

```sh
$ cmake --build <build directory> --target <target>
```

The following validation targets are supported:

- `check`: Runs all linting and formatting checks
  - `lint`: Runs all linting checks
  - `format`: Formats all code

### Sanity Checks

Also there are Sanity Checks available in the code source repository.
It contains sanity checks for the coding style. It ensures for instance
that no tabs are present in the code or that the copyright is present in
every files. These tests can be called individually by running the specific
scripts or as a whole by running __ci.py__.

The sanity checks can be found in tools/ directory
(e.g., check_style.py, check_tabs.py, check_spacing.py, check_copyright.py...).

The sanity check tests can be called individually, for instance by calling
__check_tabs.py__ from the project directory.

```sh
$ ./tools/check_tabs.py
```

The complete CI tests can be called in the following way:

  - Code quality checks (formatting, style, copyright, etc.)
  - Unit tests (framework, modules, products)
  - Build tests for all products

```sh
$ ./tools/ci.py
```

You can run a single CI stage using --stage:

```sh
# Run only code quality checks
$ ./tools/ci.py --stage code-quality

# Run only unit tests
$ ./tools/ci.py --stage unit-testing

# Run only product builds
$ ./tools/ci.py --stage build
```

Additional Options:

- --config-file    Path to the build config file
                  (default: tools/config/check_build/default_products_build.yml)
- --output-path    Where to store build logs (default: /tmp/scp/build)
- --ignore-errors  Continue running even if errors are detected

Definitions
===========

The build system sets the following definitions during the compilation of C
and assembly units:

* __BUILD_HOST__ - Set when the CPU target is "host".
* __BUILD_HAS_NOTIFICATION__ - Set when the build has notification support.
* __BUILD_STRING__ - A string containing build information (date, time and git
  commit). The string is assembled using the tool build_string.py.
* __BUILD_TESTS__ - Set when building the framework unit tests.
* __BUILD_MODE_DEBUG__ - Set when building in debug mode.
* __NDEBUG__ - Set when building in release mode. This definition is used by the
  standard C library to disable the assert() support.
* __BUILD_VERSION_MAJOR__ - Major version number.
* __BUILD_VERSION_MINOR__ - Minor version number.
* __BUILD_VERSION_PATCH__ - Patch version number.
* __BUILD_VERSION_STRING__ - String version using the format
  "v<major>.<minor>.<patch>". Example: "v2.3.1".
* __BUILD_VERSION_DESCRIBE_STRING__ - String containing version, date and git
  commit description. If the source code is not under a git repository, the
  string __unknown__ will be used instead.
* __BUILD_HAS_MOD_<MODULE NAME>__ - Set for each module being part of the build.

# Appendix

## Appendix A: Build Configurations

For build configurations please refer
to [build_configurations.md](doc/build_configurations.md)
