#!/bin/sh
# Read-only SDK,DATA plan compile ONLY;NO driver/native selection/device IO.
set -eu
[ "$#" = 1 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-learned-rms-kernel.XXXXXXXX")
sdk=$1/objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop
"$1/cross-tools-riscv64/bin/riscv64-unknown-haiku-g++" -std=gnu++17 -O2 -Wall -Wextra -Werror \
 -fno-exceptions -fno-rtti -ffreestanding -fstack-usage -mcmodel=medany -fpic -D_KERNEL_MODE \
 -I"$sdk/headers" -idirafter "$sdk/headers/posix" -c "$directory/pioneer_q38_learned_rms_kernel_compile.cpp" -o "$temporary/kernel.o"
awk -F '\t' 'NF==3 {if($2+0>2048)bad=1;if($2+0>maximum)maximum=$2+0;seen=1} END {if(!seen||bad)exit 1;print "MAX_LEARNED_RMS_DATA_KERNEL_FRAME_BYTES="maximum";NO_NATIVE_PRODUCER"}' "$temporary/kernel.su"
printf 'Canonical learned RMS DATA kernel compile:%s;NO device access\n' "$temporary"
