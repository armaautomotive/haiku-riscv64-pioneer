#!/bin/sh
# CPU-only canonical-geometry arithmetic; does not allocate card memory.
set -eu
[ "$#" = 0 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-hybrid-capacity.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable \
 -fsanitize=address,undefined "$directory/pioneer_q38_hybrid_capacity_test.cpp" -o "$temporary/test"
"$temporary/test"
for role in _KERNEL_MODE PIONEER_Q38_WORK_INFERENCE TT_PIONEER_Q38_WORK_MODE PIONEER_Q38_CPU_VALIDATION PIONEER_Q38_FULL_MODEL_CPU_REFERENCE;do
 if ${CXX:-c++} -std=c++17 -D"$role" -E -x c++ "$directory/../PioneerQ38HybridModelPlan.h" > /dev/null 2> "$temporary/$role.stderr";then exit 1;fi
 rg -q 'Hybrid layout proposal is offline data' "$temporary/$role.stderr"
done
printf 'PASS hybrid repack role separation: five forbidden contexts; CPU_ONLY\n'
