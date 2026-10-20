#!/bin/sh
# OFFLINE actual RV32 observer ONLY,not a driver selection/native authority.
set -eu
[ "$#" = 2 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
for entry in "$1:text.bin:8d0a4339c676509ff45c8802f00cb2a875dd4644537df158a9087464fc234a5a" \
 "$1:affine.bin:521420fa2a52603dc4c77f95cfab3df78f36f59532de38d8a99d7e2244ef892d" \
 "$2:text.bin:c2f5dbf162261221c0cdfafe45e24f778aaae32b180dbaa01453f6c4881f87d0" \
 "$2:affine.bin:f5b36fa25338b9e63f8a10f0cf6bb60cc397e91261bfececcec603eab1b94250"; do
 path=${entry%:*}; expected=${entry##*:}; base=${path%:*}; section=${path##*:}
 actual=$(shasum -a 256 "$base/$section"); [ "${actual%% *}" = "$expected" ]
done
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-input-sum-cache.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DPIONEER_Q38_TEST_INT_ACCUMULATOR -DPIONEER_Q38_TEST_PIPELINED_MATRIX -DPIONEER_Q38_Q4_TWO_PHASE \
 -DPIONEER_Q38_COMPACT_ACTIVATIONS -DPIONEER_Q38_REUSE_ACTIVATIONS -DPIONEER_Q38_WORK_COHORT_CACHE \
 -DPIONEER_Q38_WORK_SKIP_DOT_CHECK -DTT_PIONEER_Q38_WORK_MODE -DPIONEER_Q38_INT_SFPU \
 -DPIONEER_Q38_PIPELINED_MATRIX -DPIONEER_Q38_STATIC_VECTOR -DPIONEER_Q38_CACHED_AFFINE \
 -DPIONEER_Q38_REPLAY_OPERAND -DPIONEER_Q38_WORK_OUTPUT_DMA -DPIONEER_Q38_SPAN_FAST \
 -DPIONEER_Q38_QUEUED_MATRIX_ENTRY -DTT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_VECTOR \
 -DPIONEER_Q38_REFERENCE_ROUNDING_BOUND \
 "$directory/pioneer_q38_input_sum_cache_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" "$@"
printf 'Offline actual input-sum cache evidence retained at %s\n' "$temporary"
