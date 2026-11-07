#!/bin/sh
# Offline mapping and role checks, never a hardware or ownership test.
set -eu
[ "$#" = 0 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-hybrid-footprint.XXXXXXXX")
${CXX:-c++} -std=c++17 -O2 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_q38_hybrid_token_footprint_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test"
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -DPIONEER_Q38_TOKEN_BF16_PROGRAMS=1 \
 -DPIONEER_Q38_HYBRID_JOURNAL_DIAGNOSTIC=1 "$directory/pioneer_q38_hybrid_token_selection_test.cpp" -o "$temporary/selection"
"$temporary/selection"
if ${CXX:-c++} -std=c++17 -DPIONEER_Q38_HYBRID_JOURNAL_DIAGNOSTIC=1 \
 -E "$directory/pioneer_q38_hybrid_token_selection_test.cpp" > /dev/null 2> "$temporary/no-qt4.stderr"; then exit 1; fi
rg -q 'Hybrid journal fixture requires QTP4 test DATA' "$temporary/no-qt4.stderr"
if ${CXX:-c++} -std=c++17 -DPIONEER_Q38_TOKEN_BF16_PROGRAMS=1 -DPIONEER_Q38_HYBRID_JOURNAL_DIAGNOSTIC=0 \
 -E "$directory/pioneer_q38_hybrid_token_selection_test.cpp" > /dev/null 2> "$temporary/zero-selector.stderr"; then exit 1; fi
rg -q 'Hybrid journal fixture requires QTP4 test DATA' "$temporary/zero-selector.stderr"
for role in _KERNEL_MODE PIONEER_Q38_TOKEN_NATIVE_CONTEXT PIONEER_Q38_WORK_INFERENCE TT_PIONEER_Q38_WORK_MODE PIONEER_Q38_FULL_OFFLOAD_NATIVE PIONEER_Q38_FULL_OFFLOAD_CLIENT PIONEER_Q38_BF16_FULL_NATIVE PIONEER_Q38_BF16_FULL_CLIENT PIONEER_DRIVER_BUILD_TAG PIONEER_Q38_CPU_VALIDATION PIONEER_Q38_FULL_MODEL_CPU_REFERENCE; do
 if ${CXX:-c++} -std=c++17 -D"$role" -E "$directory/pioneer_q38_hybrid_token_footprint_test.cpp" > /dev/null 2> "$temporary/$role.stderr"; then exit 1; fi
 rg -q 'Hybrid token journal geometry is experimental DATA only' "$temporary/$role.stderr"
 if ${CXX:-c++} -std=c++17 -DPIONEER_Q38_TOKEN_BF16_PROGRAMS=1 -DPIONEER_Q38_HYBRID_JOURNAL_DIAGNOSTIC=1 -D"$role" \
  -E "$directory/pioneer_q38_hybrid_token_selection_test.cpp" > /dev/null 2> "$temporary/selection-$role.stderr"; then exit 1; fi
 rg -q 'Hybrid journal fixture requires QTP4 test DATA and forbids native or Work roles' "$temporary/selection-$role.stderr"
done
printf 'PASS hybrid footprint role exclusions; offline test artifacts at %s\n' "$temporary"
