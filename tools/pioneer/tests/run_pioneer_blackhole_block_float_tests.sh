#!/bin/sh
# CPU-memory codec only;never opens a card or changes an installed backend.
set -eu
[ "$#" = 0 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-blackhole-block-float.XXXXXXXX")
${CXX:-c++} -std=c++17 -O2 -g -Wall -Wextra -Werror -ffp-contract=off \
 -fsanitize=address,undefined "$directory/pioneer_blackhole_block_float_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test"
printf 'CPU-only experimental native block-float evidence: %s\n' "$temporary"
