#!/bin/sh
set -eu
[ "$#" = 0 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-ffn-graph-session.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DTT_PIONEER_EXPERIMENTAL_Q38_COHORT_SEALED -DTT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_VECTOR \
 -DPIONEER_Q38_WORK_INFERENCE -DPIONEER_Q38_SPAN_PRELOAD -DPIONEER_Q38_SPAN_FAST -DPIONEER_Q38_FFN_PRIME=1 \
 -DPIONEER_Q38_FFN_MODEL_INIT_CLIENT=1 -DPIONEER_Q38_FFN_NATIVE_GRAPH_CLIENT=1 \
 "$directory/pioneer_q38_ffn_graph_session_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test"
