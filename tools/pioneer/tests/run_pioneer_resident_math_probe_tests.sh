#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-resident-math.XXXXXXXX")
trap 'rm -f "$temporary/test"; rmdir "$temporary"' EXIT HUP INT TERM
${CXX:-c++} -std=c++17 -O1 -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_resident_math_probe_test.cpp" -o "$temporary/test"
"$temporary/test"
printf 'PASS: resident-input compute policy; every dropped write, NoC completion, no reset/retry/payload rewrite after uncertainty, GDDR/L1/NIU/TLB rollback\n'
