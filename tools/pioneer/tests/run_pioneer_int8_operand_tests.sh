#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-int8-operand.XXXXXXXX")
trap 'rm -f "$temporary/test"; rmdir "$temporary"' EXIT HUP INT TERM
${CXX:-c++} -std=c++17 -O1 -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_int8_operand_test.cpp" -o "$temporary/test"
"$temporary/test"
printf 'PASS: bounded integer-8 overlay, exhaustive 16-bit validity/refusal and signed-byte round trips; NO hardware\n'
