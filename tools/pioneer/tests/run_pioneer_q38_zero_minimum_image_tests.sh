#!/bin/sh
# OFFLINE actual RV32/Tensix observer;NO driver selection/native authority.
set -eu
[ "$#" = 2 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
for entry in "$1:text.bin:c2f5dbf162261221c0cdfafe45e24f778aaae32b180dbaa01453f6c4881f87d0" \
 "$1:affine.bin:f5b36fa25338b9e63f8a10f0cf6bb60cc397e91261bfececcec603eab1b94250" \
 "$2:text.bin:9a3f8f13b3dc8d147bd07df92f8437bc0b681e7298a4fde36166bd1a2cde07a8" \
 "$2:affine.bin:9258c8b82bc0d419d4497f1ab816e5e0bc54e1fe6f34e67afa0c31a5088b0e3c"; do
 path=${entry%:*}; expected=${entry##*:}; base=${path%:*}; section=${path##*:}
 actual=$(shasum -a 256 "$base/$section"); [ "${actual%% *}" = "$expected" ]
done
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-zero-minimum-image.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DPIONEER_Q38_TEST_INT_ACCUMULATOR -DPIONEER_Q38_TEST_PIPELINED_MATRIX -DPIONEER_Q38_Q4_TWO_PHASE \
 -DPIONEER_Q38_COMPACT_ACTIVATIONS -DPIONEER_Q38_REUSE_ACTIVATIONS -DPIONEER_Q38_WORK_COHORT_CACHE \
 -DPIONEER_Q38_WORK_SKIP_DOT_CHECK -DTT_PIONEER_Q38_WORK_MODE -DPIONEER_Q38_INT_SFPU \
 -DPIONEER_Q38_PIPELINED_MATRIX -DPIONEER_Q38_STATIC_VECTOR -DPIONEER_Q38_CACHED_AFFINE \
 -DPIONEER_Q38_REPLAY_OPERAND -DPIONEER_Q38_WORK_OUTPUT_DMA -DPIONEER_Q38_SPAN_FAST \
 -DPIONEER_Q38_QUEUED_MATRIX_ENTRY -DTT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_VECTOR \
 -DPIONEER_Q38_REFERENCE_ROUNDING_BOUND \
 "$directory/pioneer_q38_zero_minimum_image_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" "$@"
printf 'Offline actual zero-minimum image evidence retained at %s\n' "$temporary"
