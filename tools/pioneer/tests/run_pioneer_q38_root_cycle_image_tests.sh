#!/bin/sh
# OFFLINE actual RV32 root/peer;fabricated CSR clock,NO hardware operations.
set -eu
[ "$#" = 3 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
hash_is() { actual=$(shasum -a 256 "$1"); [ "${actual%% *}" = "$2" ]; }
hash_is "$1/text.bin" b0b67a77f5341604b0be5f759b4d9228aa934067a390e5805ab3b7c1de782194
hash_is "$1/output.bin" 1385ab712bf8756704a5c99abc344683b58b664a4bfac06443c2c83f7ae10427
hash_is "$2/text.bin" aa27f4356c7cc6bfb14dabbf79473a6b8e519ff0386ca343b4e7eabd500de5f7
hash_is "$2/output.bin" 15643b20c5aafa9090c00e6be0dd4985653cae7d621c707c3f69e27ddde9d03d
hash_is "$3/text.bin" 9ca34637bde4497ac08bac46b6611a97de972daba0af6cfb61ab71027b4ccc25
hash_is "$3/affine.bin" 0c3458da669f7eaa2488a94a23b7b3f7dd24a49d14de7c61270e167df6b4cbb0
for image in "$1" "$2" "$3"; do hash_is "$image/boot.bin" e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b; done
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-root-cycle-image.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DPIONEER_Q38_TEST_DEFER_AFFINE -DPIONEER_Q38_WORK_DEFER_AFFINE \
 -DPIONEER_Q38_COMPACT_ACTIVATIONS -DPIONEER_Q38_REUSE_ACTIVATIONS -DPIONEER_Q38_WORK_COHORT_CACHE \
 -DPIONEER_Q38_WORK_SKIP_DOT_CHECK -DTT_PIONEER_Q38_WORK_MODE -DPIONEER_Q38_INT_SFPU \
 -DPIONEER_Q38_PIPELINED_MATRIX -DPIONEER_Q38_STATIC_VECTOR -DPIONEER_Q38_CACHED_AFFINE \
 -DPIONEER_Q38_REPLAY_OPERAND -DPIONEER_Q38_WORK_OUTPUT_DMA -DPIONEER_Q38_SPAN_FAST \
 -DPIONEER_Q38_QUEUED_MATRIX_ENTRY -DTT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_VECTOR \
 -DPIONEER_Q38_TEST_INT_ACCUMULATOR -DPIONEER_Q38_TEST_PIPELINED_MATRIX -DPIONEER_Q38_Q4_TWO_PHASE \
 -DPIONEER_Q38_WORK_INPUT_SUM_CACHE -DPIONEER_Q38_WORK_ZERO_MINIMUM -DPIONEER_Q38_NATIVE_OPERAND_STORE \
 -DPIONEER_Q38_WORK_SPARSE_EXPORT -DPIONEER_Q38_WORK_COMPACT_OUTPUT \
 "$directory/pioneer_q38_root_cycle_image_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" "$@"
printf 'Offline ROOT cycle image evidence retained at %s;NO native timing\n' "$temporary"
