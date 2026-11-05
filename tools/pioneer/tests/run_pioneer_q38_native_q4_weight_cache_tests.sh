#!/bin/sh
# Synthetic sparse851-tensor source only;NO real model or device access.
set -eu
[ "$#" = 0 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-native-q4-weight-cache.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -Wno-deprecated-declarations \
 -ffp-contract=off -fsanitize=address,undefined \
 "$directory/pioneer_q38_native_q4_weight_cache_test.cpp" "$directory/../PioneerModelWeightCache.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" "$temporary"
python3 "$directory/pioneer_q38_native_q4_cache_build_gate_test.py"
printf 'Local native-Q4 preparation fixture retained: %s\n' "$temporary"
