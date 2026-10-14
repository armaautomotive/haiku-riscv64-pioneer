#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-p100a-root.XXXXXXXX")
trap 'rm -rf "$temporary"' EXIT HUP INT TERM
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_p100a_root_policy_test.cpp" -o "$temporary/test"
"$temporary/test"
printf 'PASS: P100A root opt-in requires exact tag, bus range and register layout\n'
