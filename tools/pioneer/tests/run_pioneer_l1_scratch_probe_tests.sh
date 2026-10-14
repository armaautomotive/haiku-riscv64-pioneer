#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-l1-probe.XXXXXXXX")
trap 'rm -rf "$temporary"' EXIT HUP INT TERM
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_l1_scratch_probe_test.cpp" -o "$temporary/test"
"$temporary/test"
printf 'PASS: stopped-core bounded L1 patterns, dynamic VC, data/TLB rollback and fault injection\n'
