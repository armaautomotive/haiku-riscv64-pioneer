#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-runtime-policy.XXXXXXXX")
trap 'rm -f "$temporary/test"; rmdir "$temporary"' EXIT HUP INT TERM
${CXX:-c++} -std=c++17 -O1 -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_runtime_matvec_policy_test.cpp" -o "$temporary/test"
"$temporary/test"
printf 'PASS: resident runtime prepare/reuse, staged vector/output, exact completion and no reset/payload rewrite after uncertainty\n'
