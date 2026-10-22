#!/bin/sh
# Actual nested controllers,simulated endpoints;NO native/card/SDK IO.
set -eu
[ "$#" = 0 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-root-parent-close.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DPIONEER_Q38_FFN_GRAPH_INPUT_ROOT=1 "$directory/pioneer_q38_ffn_root_lifetime_parent_close_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test"
printf 'Offline ROOT parent-close evidence retained at %s;NO device access\n' "$temporary"
