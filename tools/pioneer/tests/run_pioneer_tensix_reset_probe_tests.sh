#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-reset-probe.XXXXXXXX")
trap 'rm -rf "$temporary"' EXIT HUP INT TERM
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_tensix_reset_probe_test.cpp" -o "$temporary/test"
"$temporary/test"
printf 'PASS: enabled physical Tensix tile, read-only reset state, exact TLB restoration\n'
