#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-graph-norm.XXXXXXXX")
case ${PIONEER_GRAPH_FAST_RMS:-0} in 0|1) ;; *) exit 2;; esac
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DPIONEER_GRAPH_FAST_RMS=${PIONEER_GRAPH_FAST_RMS:-0} \
 "$directory/pioneer_graph_norm_test.cpp" -o "$temporary/test"
UBSAN_OPTIONS=halt_on_error=1 "$temporary/test"
printf 'Offline graph norm test retained at %s\n' "$temporary"
