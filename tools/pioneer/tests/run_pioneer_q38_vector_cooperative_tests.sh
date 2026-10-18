#!/bin/sh
set -eu
[ "$#" -ge 2 ] && [ "$#" -le 3 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-vector-cooperative.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DPIONEER_Q38_VECTOR_COOPERATIVE "$directory/pioneer_qwen38_dispatch_cooperative_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" "$@"
printf 'Offline actual root+vector-peer evidence retained at %s\n' "$temporary"
