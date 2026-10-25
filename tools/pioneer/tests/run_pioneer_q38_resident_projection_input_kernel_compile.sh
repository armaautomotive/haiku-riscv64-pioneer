#!/bin/sh
# Template/header compile ONLY, not the missing actual native adapter.
set -eu
[ "$#" = 1 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
temporary=$(mktemp -d /private/tmp/pioneer-resident-input-kernel.XXXXXXXX)
sdk=$1/objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop
"$1/cross-tools-riscv64/bin/riscv64-unknown-haiku-g++" -std=gnu++17 -O2 -Wall -Wextra -Werror \
 -fno-exceptions -fno-rtti -ffreestanding -fstack-usage -mcmodel=medany -fpic -D_KERNEL_MODE \
 -I"$sdk/headers" -idirafter "$sdk/headers/posix" -c \
 "$directory/pioneer_q38_resident_projection_input_kernel_compile.cpp" -o "$temporary/new-owner.o"
awk -F '\t' 'NF==3 {if($2+0>maximum)maximum=$2+0;if($2+0>2048||$3!="static")bad=1;seen=1} END {if(!seen||bad)exit 1;print "MAX_CLOSED_INPUT_KERNEL_FRAME_BYTES="maximum";HEAP_OWNERS_ONLY"}' "$temporary/new-owner.su"
shasum -a 256 "$temporary/new-owner.o"
printf 'Closed input template proof:%s;SDK read-only,NO native factory/driver/link/install/card access\n' "$temporary"
