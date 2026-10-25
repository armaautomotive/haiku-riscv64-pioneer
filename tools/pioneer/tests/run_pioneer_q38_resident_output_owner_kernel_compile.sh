#!/bin/sh
# Complete owner template compile ONLY; NOT an actual native factory/test.
set -eu
[ "$#" = 1 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
temporary=$(mktemp -d /private/tmp/pioneer-output-owner-kernel.XXXXXXXX)
sdk=$1/objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop
"$1/cross-tools-riscv64/bin/riscv64-unknown-haiku-g++" -std=gnu++17 -O2 -Wall -Wextra -Werror \
 -fno-exceptions -fno-rtti -ffreestanding -fstack-usage -mcmodel=medany -fpic -D_KERNEL_MODE \
 -I"$sdk/headers" -idirafter "$sdk/headers/posix" -c \
 "$directory/pioneer_q38_resident_output_owner_kernel_compile.cpp" -o "$temporary/owner.o"
awk -F '\t' 'NF==3 {if($2+0>maximum)maximum=$2+0;if($2+0>2048||$3!="static")bad=1;seen=1} END {if(!seen||bad)exit 1;print "MAX_OUTPUT_OWNER_KERNEL_FRAME_BYTES="maximum}' "$temporary/owner.su"
shasum -a 256 "$temporary/owner.o"
printf 'Resident output controller:%s;TEMPLATES ONLY,NO qualified native factory/driver/link/install/card access\n' "$temporary"
