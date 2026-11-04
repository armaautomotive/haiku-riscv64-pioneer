#!/bin/sh
# Private authenticated original READ ONLY,no accelerator or derived files.
set -eu
[ "$#" = 1 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-mixed-fpu-model.XXXXXXXX")
printf 'OFFBOARD private-model mixed FPU fixture directory: %s\n' "$temporary"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -ffp-contract=off \
 -fsanitize=address,undefined "$directory/pioneer_q38_mixed_fpu_model_test.cpp" \
 "$directory/../PioneerModelWeightCache.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" "$1"
printf 'OFFBOARD private-model mixed FPU evidence: %s\n' "$temporary"
