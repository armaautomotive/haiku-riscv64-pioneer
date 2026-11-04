#!/bin/sh
# Private full-source auth + actual RV32 observer,NEVER opens a card.
set -eu
[ "$#" = 2 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q4-fpu-block-model.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -ffp-contract=off \
 -fsanitize=address,undefined "$directory/pioneer_q38_q4_fpu_block_model_test.cpp" \
 "$directory/../PioneerModelWeightCache.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" "$1" "$2"
printf 'PRIVATE read-only actual Q4 pipeline observer retained: %s\n' "$temporary"
