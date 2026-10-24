#!/bin/sh
# Actual QFG3 root;synthetic peer endpoints. NO device access.
set -eu
[ "$#" = 1 ] || exit 2
here=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
build=$(mktemp -d /private/tmp/pioneer-q38-ffn-residual-root-tests.XXXXXXXX)
"${CXX:-c++}" -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DPIONEER_Q38_FFN_GRAPH_RESIDUAL_ROOT=1 -DPIONEER_Q38_COMPACT_ACTIVATIONS -DPIONEER_Q38_REUSE_ACTIVATIONS \
 -DPIONEER_Q38_WORK_COHORT_CACHE -DPIONEER_Q38_WORK_SKIP_DOT_CHECK -DPIONEER_Q38_WORK_SPARSE_EXPORT -DPIONEER_Q38_WORK_COMPACT_OUTPUT \
 "$here/pioneer_q38_ffn_residual_graph_image_test.cpp" -o "$build/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$build/test" "$@"
printf 'Actual QFG3 root evidence:%s;NO device access\n' "$build"
