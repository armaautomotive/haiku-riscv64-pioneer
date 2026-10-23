#!/bin/sh
# Offline emitted instructions;no Pioneer access/installation.
set -eu
[ "$#" = 1 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
output=$(mktemp -d /private/tmp/pioneer-q38-gdn-input-test.XXXXXXXX)
"${CXX:-c++}" -std=c++17 -O1 -g -Wall -Wextra -Werror -ffp-contract=off \
 -fsanitize=address,undefined "$directory/pioneer_q38_gdn_input_image_test.cpp" -o "$output/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$output/test" "$1"
