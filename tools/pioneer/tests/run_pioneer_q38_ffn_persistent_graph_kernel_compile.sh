#!/bin/sh
# Read-only SDK;OBJECT ONLY,not a native driver/adapter or device access.
set -eu
[ "$#" = 1 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
sdk=$1/objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-persistent-graph-kernel.XXXXXXXX")
"$1/cross-tools-riscv64/bin/riscv64-unknown-haiku-g++" -std=gnu++17 -O2 -Wall -Wextra -Werror \
 -fno-exceptions -fno-rtti -ffreestanding -fstack-usage -mcmodel=medany -fpic -D_KERNEL_MODE \
 -DPIONEER_Q38_FFN_GRAPH_INPUT_ROOT=1 -DPIONEER_Q38_FFN_PERSISTENT_GRAPH=1 -DPIONEER_Q38_GDN_MUTABLE_STATE \
 -I"$sdk/headers" -idirafter "$sdk/headers/posix" -c "$directory/pioneer_q38_ffn_persistent_graph_kernel_compile.cpp" -o "$temporary/kernel.o"
awk -F '\t' 'NF==3 {if($2+0>2048){print "Excess persistent graph frame:",$0;bad=1};if($2+0>maximum)maximum=$2+0;seen=1}
 END {if(!seen||bad)exit 1;print "MAX_PERSISTENT_GRAPH_KERNEL_FRAME_BYTES="maximum";OBJECT_ONLY_NOT_NATIVE_ADAPTER"}' "$temporary/kernel.su"
printf 'Persistent graph kernel compile evidence: %s\n' "$temporary"
