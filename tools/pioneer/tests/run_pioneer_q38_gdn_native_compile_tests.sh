#!/bin/sh
# Compile the ACTUAL complete native translation unit with gated private
# lifecycle code. OBJECT ONLY;no linked driver/module/staging/card access.
# The ordinary driver builder MUST continue refusing mutable-state!=0.
set -eu
[ "$#" = 1 ] || exit 2
project=$(CDPATH= cd -- "$(dirname "$0")/../../.." && pwd)
build=$(CDPATH= cd -- "$1" && pwd)
sdk=$build/objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-gdn-native-compile.XXXXXXXX")
set -- -isystem "$project/headers/private/kernel" -isystem "$project/headers/private/kernel/arch/riscv64" \
 -isystem "$project/headers/private/kernel/arch/generic" -isystem "$project/headers/private" \
 -isystem "$project/headers/private/drivers" -isystem "$project/headers/private/system" \
 -isystem "$project/headers/private/system/arch/riscv64" -isystem "$project/build/config_headers" \
 -isystem "$project/headers/private/shared" -I"$build/objects/haiku/riscv64/release/system/kernel" \
 -I"$build/objects/haiku/riscv64/debug/system/kernel" -I"$sdk/headers" -idirafter "$sdk/headers/posix"
for include in "$sdk/headers/os" "$sdk"/headers/os/*; do [ ! -d "$include" ] || set -- "$@" -isystem "$include"; done
"$build/cross-tools-riscv64/bin/riscv64-unknown-haiku-g++" -std=gnu++17 -O2 -Wall -Wextra -Werror \
 -fno-exceptions -fno-rtti -fstack-usage -ffreestanding -finline -fno-semantic-interposition \
 -fno-tree-vectorize -fno-use-cxa-atexit -fno-strict-aliasing -fno-delete-null-pointer-checks -mcmodel=medany -fpic \
 -D_KERNEL_MODE -DPIONEER_Q38_GDN_MUTABLE_STATE -DTT_PIONEER_DRIVER_RELOAD -DPIONEER_DRIVER_BUILD_TAG=0x2026102000ff0001ULL \
 -DTT_PIONEER_EXPERIMENTAL_Q38_BANKS -DTT_PIONEER_EXPERIMENTAL_Q38_PANEL -DTT_PIONEER_EXPERIMENTAL_Q38_AFFINE \
 -DTT_PIONEER_EXPERIMENTAL_Q38_BLOCK -DTT_PIONEER_EXPERIMENTAL_Q38_STREAM -DTT_PIONEER_EXPERIMENTAL_Q38_ROWS \
 -DTT_PIONEER_EXPERIMENTAL_Q38_COHORT -DTT_PIONEER_EXPERIMENTAL_Q38_COHORT_SEALED -DTT_PIONEER_EXPERIMENTAL_Q38_COHORT_PAYLOAD \
 -DTT_PIONEER_EXPERIMENTAL_Q38_DISPATCH -DTT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_VECTOR \
 -DPIONEER_Q38_COMPACT_ACTIVATIONS -DPIONEER_Q38_REUSE_ACTIVATIONS -DPIONEER_Q38_REUSE_CROSS_QUANT \
 -DTT_PIONEER_Q38_WORK_MODE -DPIONEER_Q38_WORK_POSTED_STAGE -DPIONEER_Q38_WORK_COHORT_CACHE \
 -DPIONEER_Q38_Q4_TWO_PHASE -DPIONEER_Q38_SPAN_PRELOAD -DPIONEER_Q38_SPAN_FAST -DPIONEER_Q38_SPAN_REUSE_CODE \
 -DPIONEER_Q38_INT_SFPU -DPIONEER_Q38_PIPELINED_MATRIX -DPIONEER_Q38_STATIC_VECTOR -DPIONEER_Q38_CACHED_AFFINE \
 -DPIONEER_Q38_REPLAY_OPERAND -DPIONEER_Q38_WORK_SKIP_DOT_CHECK -DPIONEER_Q38_WORK_OUTPUT_DMA \
 -DPIONEER_Q38_QUEUED_MATRIX_ENTRY -DPIONEER_Q38_LOCAL_DELTA_PROBE -DPIONEER_Q38_WORK_SPARSE_EXPORT \
 -DPIONEER_Q38_NORMAL_CLOCK -DPIONEER_Q38_WORK_INPUT_SUM_CACHE -DPIONEER_Q38_WORK_ZERO_MINIMUM \
 -DPIONEER_Q38_NATIVE_OPERAND_STORE -DPIONEER_Q38_WORK_COMPACT_OUTPUT "$@" \
 -c "$project/tools/pioneer/tests/pioneer_q38_gdn_native_adapter_compile.cpp" -o "$temporary/native-private-lifecycle.o"
test -s "$temporary/native-private-lifecycle.o"
awk -F '\t' '$1 ~ /AcquireProbeLock|AcquireQ38GdnSessionLock|Q38MutableScopeOwned|RetainQ38DispatchOwner|EndQ38DispatchOwnership|ReleaseResidentCookie|StateZeroBeforeCohort|StateZeroOtherCoresHeld|NonRootHeld|MutableZero|BindQ38MutableOwner|InitializeQ38MutableState|InitializeOwnedState|PioneerQ38StateZero|PioneerQ38GdnCallScope|PioneerQ38GdnFreshPlan|pioneer_q38_gdn_.*_compile/ {
 if($2+0>1024)bad=1;if($2+0>maximum)maximum=$2+0;seen=1
} END {if(!seen||bad)exit 1;print "MAX_GATED_NATIVE_LIFECYCLE_FRAME_BYTES="maximum";OBJECT_ONLY_NO_MODULE"}' "$temporary/native-private-lifecycle.su"
printf 'PASS complete actual native driver+private zero adapters/controllers instantiated ONLY in an OBJECT witness;NO linked/usable driver,public call seal or qualification: %s\n' "$temporary"
