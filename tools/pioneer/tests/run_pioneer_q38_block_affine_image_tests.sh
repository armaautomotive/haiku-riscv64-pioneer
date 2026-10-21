#!/bin/sh
set -eu
[ "$#" = 2 ] || { [ "$#" = 3 ] && { [ "$3" = --q4-work-pilot ] || [ "$3" = --faults-only ]; }; } || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
hash_is() { actual=$(shasum -a 256 "$1"); [ "${actual%% *}" = "$2" ]; }
hash_is "$1/text.bin" b86c3bc1cdf645f60b22e233d7b7f23033791645b4ae5a2edc0a9b9c92653cc3
hash_is "$1/affine.bin" fcfb6af634be07c7abbd33ef45d328361d5c4769f3e73bb842bc9f68bcfda38d
hash_is "$2/text.bin" 366e25c3ac97ad75fa9dc1e2295a71cc239cbe8d0e12a8452d14574082745a9e
hash_is "$2/affine.bin" 189a185d35afe0aa22f4129adc708debada7a601736a09ffb8c6433b381512d6
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-block-affine-image.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DPIONEER_Q38_BLOCK_AFFINE_DST -DPIONEER_Q38_WORK_DEFER_AFFINE -DPIONEER_Q38_WORK_INPUT_SUM_CACHE \
 -DPIONEER_Q38_WORK_ZERO_MINIMUM -DPIONEER_Q38_NATIVE_OPERAND_STORE -DPIONEER_Q38_WORK_SKIP_DOT_CHECK \
 -DPIONEER_Q38_REPLAY_OPERAND -DPIONEER_Q38_CACHED_AFFINE -DPIONEER_Q38_STATIC_VECTOR \
 -DPIONEER_Q38_PIPELINED_MATRIX -DPIONEER_Q38_INT_SFPU -DTT_PIONEER_Q38_WORK_MODE \
 -DPIONEER_Q38_WORK_COHORT_CACHE -DPIONEER_Q38_QUEUED_MATRIX_ENTRY -DPIONEER_Q38_WORK_OUTPUT_DMA \
 -DPIONEER_Q38_SPAN_FAST -DTT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_VECTOR \
 -DPIONEER_Q38_TEST_INT_ACCUMULATOR -DPIONEER_Q38_TEST_PIPELINED_MATRIX -DPIONEER_Q38_Q4_TWO_PHASE \
 "$directory/pioneer_q38_block_affine_image_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" "$@"
printf 'Offline block-affine actual image evidence retained at %s;NO native/speed claim\n' "$temporary"
