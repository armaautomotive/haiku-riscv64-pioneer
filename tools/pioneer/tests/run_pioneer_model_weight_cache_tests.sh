#!/bin/sh
# Host filesystem fixture only. No user model,driver or accelerator access.
set -eu
[ "$#" = 0 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-model-weight-cache.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -ffp-contract=off \
 -fsanitize=address,undefined "$directory/pioneer_model_weight_cache_test.cpp" \
 "$directory/../PioneerModelWeightCache.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" "$temporary"
printf 'Host-only disk cache fixture retained: %s\n' "$temporary"
