#!/bin/sh
# OFFLINE ONLY;actual emitted RV32/NoC/Tensix/SFPU,NOT device access.
set -eu
[ "$#" = 1 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
output=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-work-dot-test.XXXXXXXX")
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -ffp-contract=off \
 "$directory/pioneer_q38_work_dot_modes_test.cpp" -o "$output/test"
"$output/test" "$1"
printf 'Offline Work dot-mode evidence retained at %s\n' "$output"
