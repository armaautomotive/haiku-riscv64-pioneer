#!/bin/sh
# Actual native binding fragments/types ONLY,not a linked driver or grant.
set -eu
[ "$#" = 1 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
temporary=$(mktemp -d /private/tmp/pioneer-output-native-binding-sdk.XXXXXXXX)
sdk=$1/objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop
"$1/cross-tools-riscv64/bin/riscv64-unknown-haiku-g++" -std=gnu++17 -O2 -Wall -Wextra -Werror \
 -fno-exceptions -fno-rtti -ffreestanding -fstack-usage -mcmodel=medany -fpic -D_KERNEL_MODE \
 -I"$sdk/headers" -I"$sdk/headers/os" -I"$sdk/headers/os/drivers" -I"$sdk/headers/os/kernel" -I"$sdk/headers/os/support" \
 -I"$sdk/headers/os/storage" -idirafter "$sdk/headers/posix" -c \
 "$directory/pioneer_q38_resident_output_native_binding_kernel_compile.cpp" -o "$temporary/binding.o"
awk -F '\t' 'NF==3 {if($2+0>maximum)maximum=$2+0;if($2+0>2048||$3!="static")bad=1;seen=1} END {if(!seen||bad)exit 1;print "MAX_OUTPUT_NATIVE_BINDING_KERNEL_FRAME_BYTES="maximum}' "$temporary/binding.su"
shasum -a 256 "$temporary/binding.o"
printf 'Actual binding fragments REAL SDK ABI compile ONLY:%s;SDK read-only,NO full native context/driver/link/install/card/initialization/compute grant\n' "$temporary"
