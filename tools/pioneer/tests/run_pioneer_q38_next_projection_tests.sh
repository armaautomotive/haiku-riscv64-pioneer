#!/bin/sh
# QPS2 independent DATA + emitted ROOT + ALL119 retained unchanged QFP3 peers.
# Synthetic weights/input staging,not native producer/full-model performance.
set -eu
[ "$#" = 2 ] || [ "$#" = 3 ] || exit 2
for key in $(env | sed -n 's/^\(PIONEER_[A-Z0-9_]*\)=.*/\1/p');do
 [ "$(printenv "$key" || :)" = 0 ] || exit 2
done
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-next-projection.XXXXXXXX")
python3 "$directory/pioneer_q38_next_projection_gates_test.py"
python3 "$directory/pioneer_q38_next_projection_generator_test.py" "$1" "$2"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_q38_next_projection_catalog_test.cpp" -o "$temporary/catalog"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/catalog" "$1" "$2"
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_q38_next_projection_plan_test.cpp" -o "$temporary/plan"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/plan"
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DPIONEER_Q38_PROJECTION_NEXT_ROOT=1 "$directory/pioneer_q38_next_projection_image_test.cpp" -o "$temporary/image"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/image" "$1"
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DPIONEER_Q38_PROJECTION_NEXT_ROOT=1 "$directory/pioneer_q38_next_projection_refusal_test.cpp" -o "$temporary/refusals"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/refusals" "$1"
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DPIONEER_Q38_PROJECTION_NEXT_ROOT=1 \
 -DPIONEER_Q38_TEST_WORK_DOT_ZERO_ELISION -DPIONEER_Q38_TEST_AFFINE_REPLAY -DPIONEER_Q38_BLOCK_AFFINE_DST -DPIONEER_Q38_TEST_BLOCK_SCALE_REUSE \
 -DPIONEER_Q38_WORK_DEFER_AFFINE -DPIONEER_Q38_WORK_INPUT_SUM_CACHE -DPIONEER_Q38_WORK_ZERO_MINIMUM \
 -DPIONEER_Q38_NATIVE_OPERAND_STORE -DPIONEER_Q38_WORK_SKIP_DOT_CHECK -DPIONEER_Q38_REPLAY_OPERAND \
 -DPIONEER_Q38_CACHED_AFFINE -DPIONEER_Q38_STATIC_VECTOR -DPIONEER_Q38_PIPELINED_MATRIX \
 -DPIONEER_Q38_INT_SFPU -DTT_PIONEER_Q38_WORK_MODE -DPIONEER_Q38_WORK_COHORT_CACHE \
 -DPIONEER_Q38_QUEUED_MATRIX_ENTRY -DPIONEER_Q38_WORK_OUTPUT_DMA -DPIONEER_Q38_SPAN_FAST \
 -DTT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_VECTOR -DPIONEER_Q38_TEST_INT_ACCUMULATOR \
 -DPIONEER_Q38_TEST_PIPELINED_MATRIX -DPIONEER_Q38_Q4_TWO_PHASE -DPIONEER_Q38_COMPACT_ACTIVATIONS \
 -DPIONEER_Q38_REUSE_ACTIVATIONS -DPIONEER_Q38_WORK_SPARSE_EXPORT -DPIONEER_Q38_WORK_COMPACT_OUTPUT \
 "$directory/pioneer_q38_next_projection_cooperative_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" "$1" "$2"
if [ "$#" = 3 ];then sh "$directory/run_pioneer_q38_next_projection_kernel_tests.sh" "$3";fi
printf 'Offline QPS2 DATA+ROOT+retained arithmetic evidence:%s;NO device access\n' "$temporary"
