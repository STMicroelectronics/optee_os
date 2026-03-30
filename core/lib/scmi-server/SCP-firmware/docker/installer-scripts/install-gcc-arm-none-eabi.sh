#
# Arm SCP/MCP Software
# Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

set -e

tool_dir=$1
version=$2
hostarch=$(uname -m)

# Supported versions
case $version in
"9-2019-q4-major")
	compact_vers=9-2019q4
	archive_ext=tar.bz2
	extract_flags="-xf"
	gnu_frag="gnu-rm"
	toolchain="gcc-arm-none-eabi-${version}-${hostarch}-linux"
  ;;
"9-2020-q2-update")
	compact_vers=9-2020q2
	archive_ext=tar.bz2
	extract_flags="-xf"
	gnu_frag="gnu-rm"
	toolchain="gcc-arm-none-eabi-${version}-${hostarch}-linux"
  ;;
"10-2020-q4-major")
	compact_vers=10-2020q4
	archive_ext=tar.bz2
	extract_flags="-xf"
	gnu_frag="gnu-rm"
	toolchain="gcc-arm-none-eabi-${version}-${hostarch}-linux"
  ;;
"10.3-2021.07"|"10.3-2021.10")
	compact_vers=$version
	archive_ext=tar.bz2
	extract_flags="-xf"
	gnu_frag="gnu-rm"
	toolchain="gcc-arm-none-eabi-${version}-${hostarch}-linux"
  ;;
"11.2-2022.02")
	compact_vers=$version"/binrel"
	archive_ext=tar.xz
	extract_flags="-xf"
	gnu_frag="gnu"
	toolchain="gcc-arm-${version}-${hostarch}-arm-none-eabi"
  ;;
*[rel1])
	compact_vers=$version"/binrel"
	archive_ext=tar.xz
	extract_flags="-xf"
	gnu_frag="gnu"
	toolchain="arm-gnu-toolchain-${version}-${hostarch}-arm-none-eabi"
  ;;
*)
	echo "Error: gcc version not listed"
	exit 1
  ;;
esac

url="https://developer.arm.com/-/media/Files/downloads/${gnu_frag}/${compact_vers}/${toolchain}.${archive_ext}"

echo -e "Installing ${toolchain}\n"

# Create target folder
mkdir -p ${tool_dir}

# Download
wget -nv ${url}

# Extract
tar ${extract_flags} ${toolchain}.${archive_ext} -C ${tool_dir} --strip-components=1

# Clean up
rm ${toolchain}.${archive_ext}
