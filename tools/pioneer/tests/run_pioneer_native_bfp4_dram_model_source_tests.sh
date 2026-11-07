#!/bin/sh
# CPU-only negative identity/role checks; no model or device access.
set -eu
[ "$#" = 1 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-bfp4-source-refusal.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -ffp-contract=off -fsanitize=address,undefined \
 -I"$1" "$directory/pioneer_native_bfp4_dram_model_source_test.cpp" "$directory/../PioneerModelWeightCache.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" "$temporary"
for selection in _KERNEL_MODE PIONEER_Q38_WORK_INFERENCE TT_PIONEER_Q38_WORK_MODE PIONEER_Q38_CPU_VALIDATION PIONEER_Q38_FULL_MODEL_CPU_REFERENCE;do
 if printf '#include "%s/PioneerNativeBfp4DramModelSource.h"\n' "$directory/.." | \
  ${CXX:-c++} -std=c++17 -D"$selection" -I"$1" -x c++ -c - -o "$temporary/$selection.o" 2> "$temporary/$selection.stderr";then exit 1;fi
 rg -q 'BFP4 source reference is a separate CPU diagnostic' "$temporary/$selection.stderr"
done
printf 'PASS BFP4 source:5 forbidden driver/Work/validation roles;CPU-only evidence %s\n' "$temporary"
