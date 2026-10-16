#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-second-policy.XXXXXXXX")
trap 'rm -f "$temporary/test"; rmdir "$temporary"' EXIT HUP INT TERM
${CXX:-c++} -std=c++17 -O1 -Wall -Wextra -Werror -Wno-unused-function -fsanitize=address,undefined \
 "$directory/pioneer_runtime_second_core_test.cpp" -o "$temporary/test"
"$temporary/test"
