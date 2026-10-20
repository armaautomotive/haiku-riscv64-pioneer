#!/bin/sh
# OFFLINE ONLY;no driver/card/hardware authority.
set -eu
[ "$#" = 0 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
output=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-output-transfer-test.XXXXXXXX")
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
 "$directory/pioneer_q38_output_transfer_test.cpp" -o "$output/test"
"$output/test"
printf 'Offline output-transfer evidence retained at %s\n' "$output"
