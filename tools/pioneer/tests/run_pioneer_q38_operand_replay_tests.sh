#!/bin/sh
# OFFLINE ONLY; actual compiled RV32 plus independent replay/Tensix model.
set -eu
[ "$#" = 2 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
output=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-operand-replay-test.XXXXXXXX")
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -ffp-contract=off \
 "$directory/pioneer_q38_operand_replay_test.cpp" -o "$output/test"
"$output/test" "$1" "$2"
