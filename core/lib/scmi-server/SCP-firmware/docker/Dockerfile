#
# Arm SCP/MCP Software
# Copyright (c) 2021-2025, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

FROM ubuntu:22.04@sha256:d80997daaa3811b175119350d84305e1ec9129e1799bba0bd1e3120da3ff52c3 AS common

ARG ARM_NONE_EABI_VERSION="13.3.rel1"
ARG AARCH64_NONE_ELF_VERSION="13.3.rel1"
ARG ARM_NONE_LINUX_GNUEABI_VERSION="10.3-2021.07"
ARG LCOV_VERSION="v1.16"
ARG CMAKE_VERSION="3.25.2"
ARG LLVM_VERSION="19.1.5"
ARG CPPCHECK_VERSION="2.8"
ARG DOXYGEN_VERSION="1.13.2"

ENV DEBIAN_FRONTEND=noninteractive

RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    apt-get update && \
    apt-get dist-upgrade -y && \
    apt-get install -y --no-install-recommends \
        bzip2 \
        cpanminus \
        gcc \
        flex \
        bison \
        g++ \
        git \
        make \
        ninja-build \
        perl \
        python-is-python3 \
        python3 \
        python3-pip \
        software-properties-common \
        wget \
        xz-utils && \
    python3 -m pip install --no-cache-dir --upgrade pip

ENV DEBIAN_FRONTEND=

# Install GCC arm-none-eabi
FROM common AS arm-none-eabi
COPY docker/installer-scripts/install-gcc-arm-none-eabi.sh \
    /tmp
RUN bash /tmp/install-gcc-arm-none-eabi.sh /opt/arm-none-eabi \
    "${ARM_NONE_EABI_VERSION}"

# Install GCC aarch64-none-elf
FROM common AS aarch64-none-elf
COPY docker/installer-scripts/install-gcc-aarch64-none-elf.sh \
    /tmp
RUN bash /tmp/install-gcc-aarch64-none-elf.sh /opt/aarch64-none-elf \
    "${AARCH64_NONE_ELF_VERSION}";

# Install GCC arm-none-linux-gnueabihf
FROM common AS arm-none-linux-gnueabihf
COPY container-files/installer-scripts/install-gcc-arm-none-linux-gnueabihf.sh \
    /tmp
RUN bash /tmp/install-gcc-arm-none-linux-gnueabihf.sh \
    /opt/arm-none-linux-gnueabihf \
    "${ARM_NONE_LINUX_GNUEABI_VERSION}";

# Install LCOV #
FROM common AS lcov
COPY container-files/installer-scripts/install-lcov.sh /tmp
RUN bash /tmp/install-lcov.sh /opt/lcov "${LCOV_VERSION}";
ENV PATH="${PATH}:/opt/lcov/bin"

# Install CMake, it needs to be installed before LLVM and cppcheck
FROM common AS cmake
COPY docker/installer-scripts/install-cmake.sh /tmp
RUN bash /tmp/install-cmake.sh /opt/cmake ${CMAKE_VERSION};
ENV PATH="${PATH}:/opt/cmake/bin"


# Install LLVM compiler and build libclang_rt.builtins
FROM common AS llvm
COPY --from=arm-none-eabi /opt/arm-none-eabi /opt/arm-none-eabi
COPY --from=cmake /opt/cmake /opt/cmake
ENV PATH="${PATH}:/opt/arm-none-eabi/bin"
ENV PATH="${PATH}:/opt/cmake/bin"
COPY docker/installer-scripts/install-llvm.sh /tmp
RUN bash /tmp/install-llvm.sh "${LLVM_VERSION}"


# Build and install cppcheck tool
FROM cmake AS cppcheck
COPY docker/installer-scripts/install-cppcheck.sh /tmp
RUN bash /tmp/install-cppcheck.sh /opt/cppcheck "${CPPCHECK_VERSION}"


# Install Doxygen tool
FROM cmake AS doxygen
COPY docker/installer-scripts/install-doxygen.sh /tmp
RUN bash /tmp/install-doxygen.sh /opt/doxygen "${DOXYGEN_VERSION}"


FROM llvm AS ci
ENV ARMLMD_LICENSE_FILE=
ENV LM_LICENSE_FILE=

VOLUME "/opt/arm-compiler-6"
ENV PATH="${PATH}:/opt/arm-compiler-6/bin"

COPY --from=arm-none-eabi /opt/arm-none-eabi /opt/arm-none-eabi
COPY --from=aarch64-none-elf /opt/aarch64-none-elf /opt/aarch64-none-elf
COPY --from=cmake /opt/cmake /opt/cmake
COPY --from=cppcheck /opt/cppcheck /opt/cppcheck
COPY --from=doxygen /opt/doxygen/source/build/bin/ /opt/doxygen/bin
ENV PATH="${PATH}:/opt/arm-none-eabi/bin"
ENV PATH="${PATH}:/opt/aarch64-none-elf/bin"
ENV PATH="${PATH}:/opt/cmake/bin"
ENV PATH="${PATH}:/opt/cppcheck/bin"
ENV PATH="${PATH}:/opt/doxygen/bin"

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        bundler \
        bzip2 \
        gpg-agent \
        gosu \
        lsb-release \
        lcov && \
    gem install bundler -v 2.4.22

ENV DEBIAN_FRONTEND=

# Install SCP-Firmware requirements
COPY requirements.txt .
RUN PIP_ROOT_USER_ACTION=ignore \
    python3 -m pip install --ignore-installed  -r requirements.txt

ENTRYPOINT ["sh"]


FROM ci AS user

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get dist-upgrade -y && \
    apt-get install -y --no-install-recommends nano sudo

ENV DEBIAN_FRONTEND=

COPY "docker/rootfs/usr/local/bin/init" "/usr/local/bin/init"
RUN chmod +x /usr/local/bin/init


FROM user AS dev

VOLUME /scp-firmware
WORKDIR /scp-firmware

ARG USER_UID=1000
ARG USER_GID=1000
ARG USERNAME=user

RUN groupadd -g ${USER_GID} ${USERNAME} || true && \
    useradd -l -u ${USER_UID} -g ${USERNAME} -m ${USERNAME} || true && \
    usermod -aG sudo ${USERNAME} && \
    echo "$USERNAME ALL=(root) NOPASSWD:ALL" > /etc/sudoers.d/$USERNAME && \
    chmod 0400 /etc/sudoers.d/$USERNAME

ENV NOTVISIBLE="in users profile"
RUN echo "export VISIBLE=now" >> "/etc/profile"

RUN echo "\nexport USER=${USERNAME}" >> "/home/${USERNAME}/.bashrc"
ENV PATH="/home/${USERNAME}/.local/bin:${PATH}"

ENTRYPOINT [ "bash", "/usr/local/bin/init" ]
