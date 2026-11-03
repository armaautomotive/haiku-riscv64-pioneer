#!/bin/sh
# Original + existing native cache read-only DATA diagnostic;NO card access.
set -eu
[ "$#" = 3 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-native-band-real-source.XXXXXXXX")
for phases in 4 1;do python3 "$directory/../build_native_bfp_band.py" "$1" "$temporary/p$phases" 5120 "$phases" --direct;done
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -ffp-contract=off -fsanitize=address,undefined \
 "$directory/pioneer_q38_native_bfp_band_real_source_test.cpp" "$directory/../PioneerModelWeightCache.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" "$2" "$3" "$temporary/p4" "$temporary/p1"
printf 'OFFBOARD real source/cache/band evidence: %s\n' "$temporary"
