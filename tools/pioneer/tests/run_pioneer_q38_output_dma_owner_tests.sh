#!/bin/sh
# OFFLINE host-owner/BME policy;NO device/card access.
set -eu
[ "$#" = 0 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
output=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-output-owner-test.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer \
 "$directory/pioneer_q38_output_dma_owner_test.cpp" -o "$output/test"
"$output/test"
printf 'Offline output-owner evidence retained at %s\n' "$output"
