# IO Block Interface

Copyright (c) 2024, Arm Limited. All rights reserved.

## Overview
The IO Block interface provides a standard API to be implemented by multiple
modules (IO Block drivers).

This will allow multiple modules to have the same interface.
but having different internal implementations.
i.e. It abstracts the knowledge of the API user from the implementation,
    which makes it more flexible and platform-agnostic.

## Use
    To use the IO Block interface it requires to add the include path in the
    respective module `CMakeLists.txt` file.
    ``` CMAKE
    target_include_directories(${SCP_MODULE_TARGET} PUBLIC
            "${CMAKE_SOURCE_DIR}/product/automotive-rd/rd1ae/interface/io_block/")
    ```
    Then simply include `interface_io_block.h` file to use all IO Block
    interface definitions.

### Example
    ```C
        /* `api` holds the concrete implementation of the interface. */
       api->map_region(struct interface_io_block_setup_mmap *mmap);
       ```
