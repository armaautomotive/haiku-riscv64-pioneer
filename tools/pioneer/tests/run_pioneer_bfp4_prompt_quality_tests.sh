#!/bin/sh
# CPU-only synthetic tests, no model or accelerator access.
set -eu
[ "$#" = 0 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-bfp4-prompt-quality.XXXXXXXX")
common='-std=c++17 -O1 -g -Wall -Wextra -Werror -ffp-contract=off -fsanitize=address,undefined'
${CXX:-c++} $common "$directory/pioneer_bfp4_prompt_quality_test.cpp" -o "$temporary/math"
"$temporary/math"
${CXX:-c++} $common "$directory/pioneer_bfp4_prompt_quality_source_test.cpp" "$directory/../PioneerModelWeightCache.cpp" -o "$temporary/source"
"$temporary/source" "$temporary"
${CXX:-c++} $common -DPIONEER_Q38_BFP4_QUALITY_DIAGNOSTIC=1 "$directory/pioneer_q38_bfp4_first_input_test.cpp" "$directory/pioneer_q38_cpu_model_reference.cpp" -o "$temporary/observer"
"$temporary/observer"
for selection in _KERNEL_MODE PIONEER_Q38_WORK_INFERENCE TT_PIONEER_Q38_WORK_MODE PIONEER_Q38_CPU_VALIDATION PIONEER_Q38_FULL_MODEL_CPU_REFERENCE;do
 if ${CXX:-c++} -std=c++17 -D"$selection" -E "$directory/../pioneer_bfp4_prompt_quality.cpp" > /dev/null 2> "$temporary/$selection.stderr";then exit 1;fi
 rg -q 'BFP4 prompt comparison is separate diagnostic-only CPU data' "$temporary/$selection.stderr"
done
for selection in _KERNEL_MODE PIONEER_Q38_WORK_INFERENCE TT_PIONEER_Q38_WORK_MODE;do
 if ${CXX:-c++} -std=c++17 -DPIONEER_Q38_BFP4_QUALITY_DIAGNOSTIC=1 -D"$selection" -E "$directory/pioneer_q38_bfp4_prompt_input.cpp" > /dev/null 2> "$temporary/producer-$selection.stderr";then exit 1;fi
 rg -q 'Exact BFP4 quality CPU diagnostic role required' "$temporary/producer-$selection.stderr"
done
printf 'PASS prompt-quality role separation: 5 observer and 3 producer refusals; CPU-only evidence %s\n' "$temporary"
