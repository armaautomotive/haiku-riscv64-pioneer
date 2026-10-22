#!/bin/sh
# Same ACTUAL owner+frozen ROOT/ALL119 arithmetic and fault fixtures,with
# ONLY duplicate CPU work-manifest post-read elided. NO device access.
set -eu
[ "$#" = 3 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
sh "$directory/run_pioneer_q38_ffn_input_graph_images_tests.sh" "$1" "$2"
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-ffn-sealed-proof.XXXXXXXX")
for source in pioneer_q38_ffn_persistent_graph_fault_test pioneer_q38_ffn_persistent_graph_test;do
 ${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
  -DPIONEER_Q38_FFN_GRAPH_INPUT_ROOT=1 -DPIONEER_Q38_FFN_PERSISTENT_GRAPH=1 -DPIONEER_Q38_GDN_MUTABLE_STATE \
  -DPIONEER_Q38_FFN_HOST_PROFILE=1 -DPIONEER_Q38_FFN_SEALED_PROOF=1 \
  -DPIONEER_Q38_TEST_WORK_DOT_ZERO_ELISION -DPIONEER_Q38_TEST_AFFINE_REPLAY \
  -DPIONEER_Q38_TEST_BLOCK_SCALE_REUSE -DPIONEER_Q38_TEST_INT_ACCUMULATOR -DPIONEER_Q38_TEST_PIPELINED_MATRIX \
  "$directory/$source.cpp" -o "$temporary/$source"
 ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/$source" "$@"
done
printf 'Sealed-proof full-FFN owner evidence: %s;NO native timing or device access\n' "$temporary"
