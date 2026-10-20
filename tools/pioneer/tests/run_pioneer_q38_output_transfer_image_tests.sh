#!/bin/sh
# OFFLINE actual compiled RV32;NO driver/card access.
set -eu
[ "$#" = 1 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
output=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-output-image-test.XXXXXXXX")
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
 "$directory/pioneer_q38_output_transfer_image_test.cpp" -o "$output/test"
"$output/test" "$1"
printf 'Offline actual output-image evidence retained at %s\n' "$output"
