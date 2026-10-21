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
case "${PIONEER_Q38_TEST_BLOCK_AFFINE:-0}" in
 0) peer_cpp=
    hash_is "$3/text.bin" 9ca34637bde4497ac08bac46b6611a97de972daba0af6cfb61ab71027b4ccc25
    hash_is "$3/affine.bin" 0c3458da669f7eaa2488a94a23b7b3f7dd24a49d14de7c61270e167df6b4cbb0;;
 1) peer_cpp=-DPIONEER_Q38_BLOCK_AFFINE_DST
    hash_is "$3/text.bin" 366e25c3ac97ad75fa9dc1e2295a71cc239cbe8d0e12a8452d14574082745a9e
    hash_is "$3/affine.bin" 189a185d35afe0aa22f4129adc708debada7a601736a09ffb8c6433b381512d6;;
 2) peer_cpp='-DPIONEER_Q38_BLOCK_AFFINE_DST -DPIONEER_Q38_TEST_BLOCK_SCALE_REUSE'
    hash_is "$3/text.bin" 9aa8f73b560311c4f19b0267bfa73247d49981d5f23395e50ab6347f0dbfba78
    hash_is "$3/affine.bin" e55c606956e5602cdc9818b87ff7aa8e72e97e368abe8c1b8a8cd5585903de4b;;
 3) peer_cpp='-DPIONEER_Q38_BLOCK_AFFINE_DST -DPIONEER_Q38_TEST_BLOCK_SCALE_REUSE -DPIONEER_Q38_TEST_GROUP32'
    hash_is "$3/text.bin" 6f0087f798dfb657eb137acad08e65541e121747403c2d04e42c2661fdd14948
    hash_is "$3/affine.bin" 2cb765229203947400a2cc3710443cfd6ce572d5129829e61fed08e4a8109097;;
 4) peer_cpp='-DPIONEER_Q38_BLOCK_AFFINE_DST -DPIONEER_Q38_TEST_BLOCK_SCALE_REUSE -DPIONEER_Q38_TEST_PREDECODED_Q4 -DPIONEER_Q38_TEST_NATIVE_Q8'
    hash_is "$3/text.bin" 02d36cf9d0ec055cdf1bc701c3ffa584c79f66ee5dd248c7bb1bc3ddeb2368e2
    hash_is "$3/affine.bin" 1b9cdc6dacd5a03e157087b875e904c95e384d3d9143590b89beed91f591592f;;
 *) exit 2;;
esac
for image in "$1" "$2" "$3"; do hash_is "$image/boot.bin" e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b; done
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-root-cycle-image.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined $peer_cpp \
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
