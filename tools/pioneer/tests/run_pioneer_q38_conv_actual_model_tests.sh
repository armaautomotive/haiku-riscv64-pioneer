#!/bin/sh
# Read-only original public GGUF + emitted RV32;NO device/llama/network.
set -eu
[ "$#" = 2 ] && [ -f "$2" ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-conv-original-model.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_q38_conv_actual_model_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" "$1" "$2"
printf 'Actual QCV1 original-model instruction evidence:%s;NO device access\n' "$temporary"
