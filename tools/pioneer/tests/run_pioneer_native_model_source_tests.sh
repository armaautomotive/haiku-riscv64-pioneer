#!/bin/sh
# Full original read-only CPU proof;NO GPU/cache writes or model mutation.
set -eu
[ "$#" = 2 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-learned-model-source.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -ffp-contract=off -fsanitize=address,undefined \
 -I"$1" "$directory/pioneer_native_model_source_test.cpp" "$directory/../PioneerModelWeightCache.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" "$2" "$temporary"
for selection in _KERNEL_MODE PIONEER_Q38_WORK_INFERENCE PIONEER_Q38_CPU_VALIDATION;do
 if printf '#include "%s/PioneerNativeModelBandSource.h"\n' "$directory/.." | \
  ${CXX:-c++} -std=c++17 -D"$selection" -I"$1" -x c++ -c - -o "$temporary/$selection.o" 2> "$temporary/$selection.stderr";then exit 1;fi
 rg -q 'Learned-band CPU reference is a separate diagnostic' "$temporary/$selection.stderr"
done
printf 'PASS learned source:3 forbidden driver/Work/validation roles;CPU-only evidence %s\n' "$temporary"
