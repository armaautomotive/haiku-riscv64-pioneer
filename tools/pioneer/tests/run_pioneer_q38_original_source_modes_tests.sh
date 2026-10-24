#!/bin/sh
set -eu
[ "$#" = 1 ] || exit 2
here=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
output=$(mktemp -d /private/tmp/pioneer-q38-original-modes.XXXXXXXX)
"${CXX:-c++}" -std=c++17 -O1 -g -Wall -Wextra -Werror -ffp-contract=off -fsanitize=address,undefined \
 "$here/pioneer_q38_original_source_modes_test.cpp" "$here/pioneer_q38_decoder_original_source.cpp" -o "$output/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$output/test" "$1"
