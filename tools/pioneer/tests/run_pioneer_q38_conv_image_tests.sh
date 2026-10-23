#!/bin/sh
# ACTUAL emitted QCV1 instructions,NO device/llama/network access.
set -eu
[ "$#" = 1 ] || { [ "$#" = 2 ] && { [ "$2" = --faults ] || [ "$2" = --sequence ]; }; } || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-conv-image.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_q38_conv_image_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" "$@"
printf 'Actual QCV1 instruction evidence:%s;NO device access\n' "$temporary"
