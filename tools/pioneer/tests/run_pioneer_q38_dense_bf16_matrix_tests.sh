#!/bin/sh
# Offline numerical/instruction observer only; never accesses the card.
set -eu
[ "$#" = 0 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-dense-bf16.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror \
 -Wno-missing-field-initializers -ffp-contract=off -fsanitize=address,undefined \
 "$directory/pioneer_q38_dense_bf16_matrix_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test"
printf 'OFFLINE dense BF16 matrix test: %s\n' "$temporary"
