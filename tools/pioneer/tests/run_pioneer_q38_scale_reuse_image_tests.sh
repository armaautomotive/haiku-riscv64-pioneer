#!/bin/sh
set -eu
[ "$#" = 2 ] || { [ "$#" = 3 ] && [ "$3" = --q4-work-pilot ]; } || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
hash_is(){ actual=$(shasum -a 256 "$1");[ "${actual%% *}" = "$2" ]; }
hash_is "$1/text.bin" 366e25c3ac97ad75fa9dc1e2295a71cc239cbe8d0e12a8452d14574082745a9e
hash_is "$1/affine.bin" 189a185d35afe0aa22f4129adc708debada7a601736a09ffb8c6433b381512d6
hash_is "$2/text.bin" 9aa8f73b560311c4f19b0267bfa73247d49981d5f23395e50ab6347f0dbfba78
hash_is "$2/affine.bin" e55c606956e5602cdc9818b87ff7aa8e72e97e368abe8c1b8a8cd5585903de4b
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-scale-reuse-image.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DPIONEER_Q38_BLOCK_AFFINE_DST -DPIONEER_Q38_TEST_BLOCK_SCALE_REUSE -DPIONEER_Q38_WORK_DEFER_AFFINE -DPIONEER_Q38_WORK_INPUT_SUM_CACHE \
 -DPIONEER_Q38_WORK_ZERO_MINIMUM -DPIONEER_Q38_NATIVE_OPERAND_STORE -DPIONEER_Q38_WORK_SKIP_DOT_CHECK \
 -DPIONEER_Q38_REPLAY_OPERAND -DPIONEER_Q38_CACHED_AFFINE -DPIONEER_Q38_STATIC_VECTOR \
 -DPIONEER_Q38_PIPELINED_MATRIX -DPIONEER_Q38_INT_SFPU -DTT_PIONEER_Q38_WORK_MODE \
 -DPIONEER_Q38_WORK_COHORT_CACHE -DPIONEER_Q38_QUEUED_MATRIX_ENTRY -DPIONEER_Q38_WORK_OUTPUT_DMA \
 -DPIONEER_Q38_SPAN_FAST -DTT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_VECTOR \
 -DPIONEER_Q38_TEST_INT_ACCUMULATOR -DPIONEER_Q38_TEST_PIPELINED_MATRIX -DPIONEER_Q38_Q4_TWO_PHASE \
 "$directory/pioneer_q38_scale_reuse_image_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" "$@"
printf 'Offline ACTUAL scale reuse evidence retained at %s;NO native IO\n' "$temporary"
