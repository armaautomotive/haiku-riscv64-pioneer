#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-float16.XXXXXXXX")
trap 'rm -f "$temporary/test"; rmdir "$temporary"' EXIT HUP INT TERM
${CXX:-c++} -std=c++17 -O1 -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_float16_test.cpp" -o "$temporary/test"
"$temporary/test"
printf 'PASS: FP16 nearest-even, explicit subnormal flush, overflow/nonfinite and exhaustive normal-range round trips\n'
