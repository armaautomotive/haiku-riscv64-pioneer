#!/bin/sh
# Original-model CPU/file validation ONLY. No P100A device access.
set -eu
[ "$#" = 2 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
test -f "$2"
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-native-bfp-real.XXXXXXXX")
python3 "$directory/../build_q38_native_bfp_matvec.py" "$1" "$temporary/bfp4" 4
python3 "$directory/../build_q38_native_bfp_matvec.py" "$1" "$temporary/bfp8" 8
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -ffp-contract=off \
 -fsanitize=address,undefined "$directory/pioneer_q38_native_bfp_real_test.cpp" \
 "$directory/../PioneerModelWeightCache.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" "$temporary/bfp4" "$temporary/bfp8" "$2" "$temporary/learned-tile-cache"
printf 'OFFBOARD learned-weight BFP evidence: %s\n' "$temporary"
