#!/bin/sh
# Read-only actual GGUF reference regression,no device/llama/network.
set -eu
[ "$#" = 1 ] && [ -f "$1" ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-projection-actual-reference.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_q38_projection_validation_reference_actual_model_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" "$1"
printf 'DEBUG actual projection reference evidence retained at %s;NO device access\n' "$temporary"
