#!/bin/sh
# CPU-ONLY independent actual host-loader comparison,not a native grant.
set -eu
[ "$#" -le 2 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-canonical-model.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_q38_canonical_model_plan_test.cpp" -o "$temporary/test"
if [ "$#" = 2 ]; then
 ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" "$2"
else
 ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test"
fi
if [ "$#" -ge 1 ]; then
 sdk=$1/objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop
 "$1/cross-tools-riscv64/bin/riscv64-unknown-haiku-g++" -std=gnu++17 -O2 -Wall -Wextra -Werror \
  -fno-exceptions -fno-rtti -ffreestanding -fstack-usage -mcmodel=medany -fpic -D_KERNEL_MODE -DPIONEER_Q38_GDN_MUTABLE_STATE \
  -I"$sdk/headers" -idirafter "$sdk/headers/posix" -c "$directory/pioneer_q38_canonical_model_plan_kernel_compile.cpp" -o "$temporary/kernel.o"
 awk -F '\t' 'NF==3 {if($2+0>2048)bad=1;if($2+0>maximum)maximum=$2+0;seen=1} END {if(!seen||bad)exit 1;print "MAX_CANONICAL_MODEL_KERNEL_FRAME_BYTES="maximum";HEAP_OBJECT_ONLY"}' "$temporary/kernel.su"
fi
printf 'Offline canonical model evidence retained at %s;NO device access\n' "$temporary"
