#!/bin/sh
# OFFLINE ONLY. Hash-pin both actually executed RV32/Tensix images.
set -eu
[ "$#" = 2 ] || { [ "$#" = 3 ] && [ "$3" = --faults-only ]; } || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
for entry in "$1:text.bin:9ca34637bde4497ac08bac46b6611a97de972daba0af6cfb61ab71027b4ccc25" \
 "$1:affine.bin:0c3458da669f7eaa2488a94a23b7b3f7dd24a49d14de7c61270e167df6b4cbb0" \
 "$2:text.bin:b86c3bc1cdf645f60b22e233d7b7f23033791645b4ae5a2edc0a9b9c92653cc3" \
 "$2:affine.bin:fcfb6af634be07c7abbd33ef45d328361d5c4769f3e73bb842bc9f68bcfda38d"; do
 path=${entry%:*}; expected=${entry##*:}; base=${path%:*}; section=${path##*:}
 actual=$(shasum -a 256 "$base/$section"); [ "${actual%% *}" = "$expected" ]
done
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-inline-config-image.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DPIONEER_Q38_TEST_INT_ACCUMULATOR -DPIONEER_Q38_TEST_PIPELINED_MATRIX -DPIONEER_Q38_Q4_TWO_PHASE \
 -DPIONEER_Q38_COMPACT_ACTIVATIONS -DPIONEER_Q38_REUSE_ACTIVATIONS -DPIONEER_Q38_WORK_COHORT_CACHE \
 -DPIONEER_Q38_WORK_SKIP_DOT_CHECK -DTT_PIONEER_Q38_WORK_MODE -DPIONEER_Q38_INT_SFPU \
 -DPIONEER_Q38_PIPELINED_MATRIX -DPIONEER_Q38_STATIC_VECTOR -DPIONEER_Q38_CACHED_AFFINE \
 -DPIONEER_Q38_REPLAY_OPERAND -DPIONEER_Q38_WORK_OUTPUT_DMA -DPIONEER_Q38_SPAN_FAST \
 -DPIONEER_Q38_QUEUED_MATRIX_ENTRY -DTT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_VECTOR \
 -DPIONEER_Q38_WORK_INPUT_SUM_CACHE -DPIONEER_Q38_WORK_ZERO_MINIMUM \
 -DPIONEER_Q38_NATIVE_OPERAND_STORE -DPIONEER_Q38_REFERENCE_ROUNDING_BOUND \
 "$directory/pioneer_q38_inline_config_image_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" "$@"
printf 'Offline actual inline-config evidence retained at %s\n' "$temporary"
