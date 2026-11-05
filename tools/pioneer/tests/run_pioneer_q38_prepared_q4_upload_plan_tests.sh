#!/bin/sh
# Synthetic source/cache endpoints;no device or actual model inference.
set -eu
[ "$#" = 0 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-prepared-q4-upload-tests.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -Wno-deprecated-declarations \
 -ffp-contract=off -fsanitize=address,undefined \
 "$directory/pioneer_q38_prepared_q4_upload_plan_test.cpp" "$directory/../PioneerModelWeightCache.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test"
printf 'Prepared Q4 upload/ROOT metadata fixture retained at %s;NO native/card/TPS grant\n' "$temporary"
