#
# Arm SCP/MCP Software
# Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

set -e

tool_dir=$1
version=$2

url="https://github.com/linux-test-project/lcov.git"

echo -e "Installing LCOV tool version: ${version}\n"

# Create target folder
mkdir -p ${tool_dir}
pushd ${tool_dir}

# Download
git clone "${url}" --depth 1 --branch "${version}" source

# Install
pushd source

make PREFIX=${tool_dir} install

popd
