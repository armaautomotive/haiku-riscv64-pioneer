#!/bin/sh
set -eu
[ "$#" = 3 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
hash_is(){ actual=$(shasum -a 256 "$1");[ "${actual%% *}" = "$2" ]; }
hash_is "$1/text.bin" aa27f4356c7cc6bfb14dabbf79473a6b8e519ff0386ca343b4e7eabd500de5f7
hash_is "$1/output.bin" 15643b20c5aafa9090c00e6be0dd4985653cae7d621c707c3f69e27ddde9d03d
hash_is "$2/text.bin" 65c790cf68dc705a3af5458e2b89aafa28a5b316c0a7b9fad1c8bd51fd36ecd8
hash_is "$2/output.bin" d358b793b2c83def15e58fa52b62197e2f2241dd85e9bc1a5fc9a1733cf46308
hash_is "$3/text.bin" 209fc6b8d40ed0ac7216c16ec9657ed0d83259e2ded53b35524c8cc6507bf0ef
hash_is "$3/affine.bin" 083330550580cc8ba821456c95b256ff8ea1d9742bb12230b06af469392f856a
for root in "$1" "$2" "$3";do hash_is "$root/boot.bin" e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b;done
peer_cpp='-DPIONEER_Q38_BLOCK_AFFINE_DST -DPIONEER_Q38_TEST_BLOCK_SCALE_REUSE -DPIONEER_Q38_TEST_DIRECT_SRCB -DPIONEER_Q38_TEST_AFFINE_REPLAY -DPIONEER_Q38_TEST_WORK_DOT_ZERO_ELISION'
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-sentinel-root-image.XXXXXXXX")
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
 "$directory/pioneer_q38_sentinel_root_image_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" "$@"
printf 'Offline sentinel root image evidence retained at %s;NO native timing\n' "$temporary"
