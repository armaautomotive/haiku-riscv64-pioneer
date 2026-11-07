#!/bin/sh
set -eu
[ "$#" = 0 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-hybrid-bfp8-selection.XXXXXXXX")
common='-std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined'
${CXX:-c++} $common "$directory/pioneer_q38_hybrid_bfp8_selection_test.cpp" -o "$temporary/test"
"$temporary/test"
${CXX:-c++} $common "$directory/pioneer_q38_hybrid_bfp8_forward_test.cpp" "$directory/../PioneerModelWeightCache.cpp" -o "$temporary/forward"
"$temporary/forward"
for role in _KERNEL_MODE PIONEER_Q38_WORK_INFERENCE TT_PIONEER_Q38_WORK_MODE PIONEER_Q38_CPU_VALIDATION PIONEER_Q38_FULL_MODEL_CPU_REFERENCE;do
 if ${CXX:-c++} $common -D"$role" -fsyntax-only "$directory/pioneer_q38_hybrid_bfp8_selection_test.cpp" 2> "$temporary/$role.stderr";then exit 1;fi
 rg -q 'Hybrid BFP8 selection is CPU preparation only' "$temporary/$role.stderr"
done
