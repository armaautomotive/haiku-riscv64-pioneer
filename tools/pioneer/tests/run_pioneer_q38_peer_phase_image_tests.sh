#!/bin/sh
set -eu
[ "$#" = 2 ] || { [ "$#" = 3 ] && [ "$3" = --work-pilot ]; } || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
hash_is(){ actual=$(shasum -a 256 "$1");[ "${actual%% *}" = "$2" ]; }
hash_is "$1/text.bin" 209fc6b8d40ed0ac7216c16ec9657ed0d83259e2ded53b35524c8cc6507bf0ef
hash_is "$1/affine.bin" 083330550580cc8ba821456c95b256ff8ea1d9742bb12230b06af469392f856a
hash_is "$2/text.bin" 37af1da2262d48f1fe61d717769f8a4450e76788660ff9a1dfcd443257c5d813
hash_is "$2/affine.bin" 68fa5e2164715f9030cb4dd2e05e2645d53e25ec0cdfa55bc4b8eef3b82fc4e1
hash_is "$2/fetch.bin" 439d2dae42516799a4dcc08e60ad929b52f47e1bf2bc09da7f1c7eca987c3773
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-peer-phase-image.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DPIONEER_Q38_TEST_WORK_DOT_ZERO_ELISION -DPIONEER_Q38_TEST_AFFINE_REPLAY -DPIONEER_Q38_BLOCK_AFFINE_DST -DPIONEER_Q38_TEST_BLOCK_SCALE_REUSE \
 -DPIONEER_Q38_WORK_DEFER_AFFINE -DPIONEER_Q38_WORK_INPUT_SUM_CACHE -DPIONEER_Q38_WORK_ZERO_MINIMUM \
 -DPIONEER_Q38_NATIVE_OPERAND_STORE -DPIONEER_Q38_WORK_SKIP_DOT_CHECK -DPIONEER_Q38_REPLAY_OPERAND \
 -DPIONEER_Q38_CACHED_AFFINE -DPIONEER_Q38_STATIC_VECTOR -DPIONEER_Q38_PIPELINED_MATRIX \
 -DPIONEER_Q38_INT_SFPU -DTT_PIONEER_Q38_WORK_MODE -DPIONEER_Q38_WORK_COHORT_CACHE \
 -DPIONEER_Q38_QUEUED_MATRIX_ENTRY -DPIONEER_Q38_WORK_OUTPUT_DMA -DPIONEER_Q38_SPAN_FAST \
 -DTT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_VECTOR -DPIONEER_Q38_TEST_INT_ACCUMULATOR \
 -DPIONEER_Q38_TEST_PIPELINED_MATRIX -DPIONEER_Q38_Q4_TWO_PHASE \
 "$directory/pioneer_q38_peer_phase_image_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" "$@"
printf 'Offline actual peer-phase image evidence retained at %s;NO native timing\n' "$temporary"
