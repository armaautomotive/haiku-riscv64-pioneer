#!/bin/sh
# OFFLINE controller+actual zero/root;kernel scope/producer+peers simulated.
set -eu
[ "$#" -ge 2 ] && [ "$#" -le 3 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-ffn-matrix-controller.XXXXXXXX")
flags='-DPIONEER_Q38_GDN_MUTABLE_STATE -DPIONEER_Q38_COMPACT_ACTIVATIONS -DPIONEER_Q38_REUSE_ACTIVATIONS
 -DTT_PIONEER_Q38_WORK_MODE -DPIONEER_Q38_INT_SFPU -DPIONEER_Q38_PIPELINED_MATRIX -DPIONEER_Q38_STATIC_VECTOR
 -DPIONEER_Q38_CACHED_AFFINE -DPIONEER_Q38_REPLAY_OPERAND -DPIONEER_Q38_WORK_SKIP_DOT_CHECK -DPIONEER_Q38_WORK_COHORT_CACHE
 -DPIONEER_Q38_WORK_OUTPUT_DMA -DPIONEER_Q38_SPAN_FAST -DPIONEER_Q38_QUEUED_MATRIX_ENTRY -DPIONEER_Q38_WORK_INPUT_SUM_CACHE
 -DPIONEER_Q38_WORK_ZERO_MINIMUM -DPIONEER_Q38_NATIVE_OPERAND_STORE -DPIONEER_Q38_WORK_DEFER_AFFINE
 -DPIONEER_Q38_WORK_SPARSE_EXPORT -DPIONEER_Q38_WORK_COMPACT_OUTPUT -DPIONEER_Q38_ROOT_CYCLE_PROFILE -DPIONEER_Q38_INLINE_CONFIG
 -DPIONEER_Q38_SCALE_REUSE_QUALIFICATION -DPIONEER_Q38_BLOCK_AFFINE_DST -DPIONEER_Q38_BLOCK_AFFINE_QUALIFICATION
 -DPIONEER_Q38_AFFINE_RAWDOT_QUALIFICATION -DPIONEER_Q38_DIRECT_SRCB_QUALIFICATION -DPIONEER_Q38_SENTINEL_POLL_QUALIFICATION
 -DPIONEER_Q38_Q4_TWO_PHASE -DTT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_VECTOR'
refusals=0
for selector in -UPIONEER_Q38_DIRECT_SRCB_QUALIFICATION -UPIONEER_Q38_SENTINEL_POLL_QUALIFICATION \
 -DPIONEER_Q38_RAW_UNPACK_CACHE_QUALIFICATION -DPIONEER_Q38_LOCAL_Q4_QUALIFICATION \
 -DPIONEER_Q38_Q4_NIBBLE_QUALIFICATION -DPIONEER_Q38_Q4_DIRECT_QUALIFICATION \
 -DPIONEER_Q38_BATCH4_FETCH_QUALIFICATION -DPIONEER_Q38_BATCH_RAW_QUALIFICATION \
 -DPIONEER_Q38_SIGNED_MAG2_QUALIFICATION -DPIONEER_Q38_PEER_PHASE_QUALIFICATION \
 -DPIONEER_Q38_COMMAND_REUSE_QUALIFICATION -DPIONEER_Q38_TILE_PREFETCH_QUALIFICATION \
 -DPIONEER_Q38_FRONTEND_BARRIER_QUALIFICATION -DPIONEER_Q38_HOST_PHASE_PROFILE;do
 refusals=$((refusals+1))
 if ${CXX:-c++} -std=c++17 -E $flags "$selector" "$directory/pioneer_q38_ffn_matrix_controller_test.cpp" \
  > /dev/null 2> "$temporary/refusal-$refusals.log";then exit 1;fi
 rg -F 'FFN overlay requires the exact qualified original-layout direct/sentinel cohort' "$temporary/refusal-$refusals.log" > /dev/null
done
printf 'PASS %s missing/mixed peer/root qualification compile refusals\n' "$refusals"
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined $flags \
 "$directory/pioneer_q38_ffn_matrix_controller_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" "$1" "$2"
if [ "$#" = 3 ];then
 sdk=$3/objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop
 "$3/cross-tools-riscv64/bin/riscv64-unknown-haiku-g++" -std=gnu++17 -O2 -Wall -Wextra -Werror \
  -fno-exceptions -fno-rtti -ffreestanding -fstack-usage -mcmodel=medany -fpic -D_KERNEL_MODE $flags \
  -I"$sdk/headers" -idirafter "$sdk/headers/posix" -c "$directory/pioneer_q38_ffn_matrix_kernel_compile.cpp" -o "$temporary/kernel-controller.o"
 awk -F '\t' 'NF==3 {if($2+0>1024){print "Excess kernel frame:",$0;bad=1};if($2+0>maximum)maximum=$2+0;seen=1} END {if(!seen||bad)exit 1;print "MAX_KERNEL_WITNESS_FRAME_BYTES="maximum";COMPILE_ONLY_NOT_NATIVE_ADAPTER"}' "$temporary/kernel-controller.su"
fi
printf 'Offline FFN matrix controller evidence retained at %s;NO native access\n' "$temporary"
