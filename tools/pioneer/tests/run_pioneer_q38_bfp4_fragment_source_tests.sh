#!/bin/sh
# CPU-only in-memory format preparation; no device, filesystem cache or model.
set -eu
[ "$#" = 0 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-bfp4-fragment.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -ffp-contract=off -fsanitize=address,undefined \
 "$directory/pioneer_q38_bfp4_fragment_source_test.cpp" "$directory/../PioneerModelWeightCache.cpp" -o "$temporary/test"
"$temporary/test" "$temporary"
for selection in _KERNEL_MODE PIONEER_Q38_WORK_INFERENCE TT_PIONEER_Q38_WORK_MODE PIONEER_Q38_CPU_VALIDATION PIONEER_Q38_FULL_MODEL_CPU_REFERENCE;do
 if ${CXX:-c++} -std=c++17 -DPIONEER_Q38_BFP4_PREPARATION_DIAGNOSTIC=1 -D"$selection" -E "$directory/pioneer_q38_bfp4_fragment_source_test.cpp" > /dev/null 2> "$temporary/$selection.stderr";then exit 1;fi
 rg -q 'BFP4 fragment conversion is separate offline preparation' "$temporary/$selection.stderr"
done
printf 'PASS BFP4 fragment role guards: 5 refused; CPU-only evidence %s\n' "$temporary"
