#!/bin/sh
# ACTUAL QCB1 RV32 instructions,NO device/llama/network.
set -eu
[ "$#" = 1 ] || { [ "$#" = 2 ] && [ "$2" = --faults ]; } || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-conv-batch-image.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_q38_conv_batch_image_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" "$@"
printf 'Actual QCB1 instruction evidence:%s;NO device access\n' "$temporary"
