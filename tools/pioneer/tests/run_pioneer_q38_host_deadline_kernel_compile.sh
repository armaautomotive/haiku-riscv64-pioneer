#!/bin/sh
# Read-only SDK,standalone CPU clock DATA ONLY;NO link/driver/card action.
set -eu
[ "$#" = 1 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d /private/tmp/pioneer-q38-host-deadline-sdk.XXXXXXXX)
sdk=$1/objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop
for variant in a b; do
 "$1/cross-tools-riscv64/bin/riscv64-unknown-haiku-g++" -std=gnu++17 -O2 -Wall -Wextra -Werror \
  -fno-exceptions -fno-rtti -ffreestanding -fstack-usage -mcmodel=medany -fpic -D_KERNEL_MODE \
  -I"$sdk/headers" -idirafter "$sdk/headers/posix" -c "$directory/pioneer_q38_host_deadline_kernel_compile.cpp" -o "$temporary/$variant.o"
 awk -F '\t' 'NF==3 {if($2+0>128)bad=1;if($2+0>maximum)maximum=$2+0;seen++} END {if(seen!=3||bad)exit 1;print "MAX_HOST_DEADLINE_KERNEL_FRAME_BYTES="maximum";3CPU_ONLY_WRAPPERS"}' "$temporary/$variant.su"
done
cmp "$temporary/a.o" "$temporary/b.o"
shasum -a 256 "$temporary/a.o" "$temporary/b.o"
printf 'Byte-identical CPU deadline SDK compile:%s;NO link/driver/card access\n' "$temporary"
