#!/bin/sh
set -eu
[ "$#" -le 1 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-ordered-parts.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DPIONEER_Q38_ORDERED_EXTENT_FAST "$directory/pioneer_q38_ordered_parts_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test"
for mode in -1 2 invalid; do
 status=0
 PIONEER_Q38_ORDERED_EXTENT_FAST=$mode sh "$directory/../build_qwen38_banks_driver.sh" /nonexistent-sdk "$temporary/refused-$mode" > "$temporary/$mode.log" 2>&1 || status=$?
 [ "$status" = 2 ] && [ ! -e "$temporary/refused-$mode" ]
done
for missing in PIONEER_Q38_PANEL_DRIVER PIONEER_DRIVER_RELOAD PIONEER_Q38_WORK_MODE PIONEER_Q38_WORK_COMPACT_OUTPUT PIONEER_Q38_NORMAL_CLOCK PIONEER_Q38_DISPATCH_PROFILE PIONEER_Q38_WORK_DEFER_AFFINE; do
 status=0; value=0
 case "$missing" in PIONEER_Q38_DISPATCH_PROFILE|PIONEER_Q38_WORK_DEFER_AFFINE) value=1;; esac
 env PIONEER_Q38_ORDERED_EXTENT_FAST=1 PIONEER_Q38_PANEL_DRIVER=10 PIONEER_DRIVER_RELOAD=1 \
  PIONEER_Q38_WORK_MODE=1 PIONEER_Q38_WORK_COMPACT_OUTPUT=1 PIONEER_Q38_NORMAL_CLOCK=1 \
  PIONEER_Q38_DISPATCH_PROFILE=0 PIONEER_Q38_WORK_DEFER_AFFINE=0 "$missing=$value" \
  sh "$directory/../build_qwen38_banks_driver.sh" /nonexistent-sdk "$temporary/refused-$missing" > "$temporary/$missing.log" 2>&1 || status=$?
 [ "$status" = 2 ] && [ ! -e "$temporary/refused-$missing" ]
 rg -q 'STOP ordered extents require the standalone non-profile compact Work candidate' "$temporary/$missing.log"
done
if [ "$#" = 1 ]; then
 sdk=$1/objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop
 "$1/cross-tools-riscv64/bin/riscv64-unknown-haiku-g++" -std=gnu++17 -O2 -Wall -Wextra -Werror \
  -fno-exceptions -fno-rtti -ffreestanding -fstack-usage -mcmodel=medany -fpic -D_KERNEL_MODE \
  -I"$sdk/headers" -idirafter "$sdk/headers/posix" -c "$directory/pioneer_q38_ordered_parts_kernel_compile.cpp" -o "$temporary/kernel.o"
 awk -F '\t' 'NF==3 {if($2+0>1024)bad=1;if($2+0>maximum)maximum=$2+0;seen=1} END {if(!seen||bad)exit 1;print "MAX_ORDERED_KERNEL_FRAME_BYTES="maximum";COMPILE_ONLY"}' "$temporary/kernel.su"
fi
printf 'Offline interval proof evidence retained at %s\n' "$temporary"
