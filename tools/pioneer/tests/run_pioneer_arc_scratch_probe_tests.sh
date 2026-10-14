#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-arc-probe.XXXXXXXX")
trap 'rm -rf "$temporary"' EXIT HUP INT TERM
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_arc_scratch_probe_test.cpp" -o "$temporary/test"
"$temporary/test"
printf 'PASS: fixed read-only ARC window encoding, six bounded reads, and exact TLB restoration\n'
