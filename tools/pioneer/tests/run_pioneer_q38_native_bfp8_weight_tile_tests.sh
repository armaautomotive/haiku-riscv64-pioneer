#!/bin/sh
# Synthetic CPU-only precision alternative; no model/card/installation.
set -eu
[ "$#" = 0 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-native-bfp8-tiles.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -ffp-contract=off -fsanitize=address,undefined \
 "$directory/pioneer_q38_native_bfp8_weight_tiles_test.cpp" "$directory/../PioneerModelWeightCache.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" "$temporary"
for selection in _KERNEL_MODE PIONEER_Q38_WORK_INFERENCE PIONEER_Q38_CPU_VALIDATION;do
 if printf '#include "%s/PioneerQ38NativeBfp8WeightTiles.h"\n' "$directory/.." | \
  ${CXX:-c++} -std=c++17 -D"$selection" -x c++ -c - -o "$temporary/forbidden-$selection.o" 2> "$temporary/forbidden-$selection.stderr";then exit 1;fi
 grep -q 'Unqualified all-BFP8 conversion is CPU preparation only' "$temporary/forbidden-$selection.stderr"
done
printf 'PASS all-BFP8 build guards:3 forbidden driver/Work/validation selections rejected\nCPU-only evidence: %s\n' "$temporary"
