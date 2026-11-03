#!/bin/sh
# Sparse851-tensor fixture and private synthetic cache ONLY;no model/card.
set -eu
[ "$#" = 0 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-native-band-data.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -ffp-contract=off -fsanitize=address,undefined \
 "$directory/pioneer_q38_native_bfp_band_data_test.cpp" "$directory/../PioneerModelWeightCache.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" "$temporary"
for header in PioneerQ38NativeBfpBandData.h PioneerQ38NativeBfpBandSource.h;do
 for selection in _KERNEL_MODE PIONEER_Q38_WORK_INFERENCE PIONEER_Q38_CPU_VALIDATION;do
  if printf '#include "%s/%s"\n' "$directory/.." "$header" | \
   ${CXX:-c++} -std=c++17 -D"$selection" -x c++ -c - -o "$temporary/forbidden-$header-$selection.o" 2> "$temporary/forbidden-$header-$selection.stderr";then exit 1;fi
  grep -q 'Native BFP band is an unqualified offboard prototype' "$temporary/forbidden-$header-$selection.stderr"
 done
done
printf 'PASS native band DATA/source build guards:6 driver/Work/validation selections refused\n'
printf 'CPU-only native band data evidence: %s\n' "$temporary"
