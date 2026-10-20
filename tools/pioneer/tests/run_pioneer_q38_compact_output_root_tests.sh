#!/bin/sh
# OFFLINE ONLY:actual RV32 root/peer/NoC instructions with delayed PCIe model.
set -eu
[ "$#" = 3 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
hash_is() { actual=$(shasum -a 256 "$1"); [ "${actual%% *}" = "$2" ]; }
hash_is "$1/text.bin" 73b7170e8daf0caa3db5e9a9f4a3306b6c64b648a254609ee77f0e95ec954900
hash_is "$1/output.bin" fc7337cc327bb186049ef40279e556b796d0cbb436850fefb77aa361d0224627
hash_is "$2/text.bin" b0b67a77f5341604b0be5f759b4d9228aa934067a390e5805ab3b7c1de782194
hash_is "$2/output.bin" 1385ab712bf8756704a5c99abc344683b58b664a4bfac06443c2c83f7ae10427
hash_is "$3/text.bin" 8fd633343fc76240cb36fe12a1c58080875550cf1719139f07df55f9915f8c8b
hash_is "$3/affine.bin" deb5d7c79bb9bdeb6a17284e4100635545471d484e8e509e559b97dde0bc1983
for image in "$1" "$2" "$3"; do hash_is "$image/boot.bin" e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b; done
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-compact-output-root.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DPIONEER_Q38_COMPACT_ACTIVATIONS -DPIONEER_Q38_REUSE_ACTIVATIONS -DPIONEER_Q38_WORK_COHORT_CACHE \
 -DPIONEER_Q38_WORK_SKIP_DOT_CHECK -DTT_PIONEER_Q38_WORK_MODE -DPIONEER_Q38_INT_SFPU \
 -DPIONEER_Q38_PIPELINED_MATRIX -DPIONEER_Q38_STATIC_VECTOR -DPIONEER_Q38_CACHED_AFFINE \
 -DPIONEER_Q38_REPLAY_OPERAND -DPIONEER_Q38_WORK_OUTPUT_DMA -DPIONEER_Q38_SPAN_FAST \
 -DPIONEER_Q38_QUEUED_MATRIX_ENTRY -DTT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_VECTOR \
 -DPIONEER_Q38_TEST_INT_ACCUMULATOR -DPIONEER_Q38_TEST_PIPELINED_MATRIX -DPIONEER_Q38_Q4_TWO_PHASE \
 -DPIONEER_Q38_WORK_INPUT_SUM_CACHE -DPIONEER_Q38_WORK_ZERO_MINIMUM -DPIONEER_Q38_NATIVE_OPERAND_STORE \
 -DPIONEER_Q38_WORK_SPARSE_EXPORT -DPIONEER_Q38_WORK_COMPACT_OUTPUT \
 "$directory/pioneer_q38_compact_output_root_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" "$@"
printf 'Offline actual compact-output root evidence retained at %s\n' "$temporary"
