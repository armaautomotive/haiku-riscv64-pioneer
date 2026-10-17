#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-graph-fma-fast.XXXXXXXX")
for fixed in 0 1; do
 ${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
  -DPIONEER_GRAPH_FMA_FIXED_NORMALIZE=$fixed "$directory/pioneer_graph_fma_fast_test.cpp" -o "$temporary/test-$fixed"
 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test-$fixed"
done
printf 'Offline hybrid FMA test retained at %s\n' "$temporary"
