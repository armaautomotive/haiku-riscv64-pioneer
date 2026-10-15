#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-resident-read.XXXXXXXX")
trap 'rm -f "$temporary/test"; rmdir "$temporary"' EXIT HUP INT TERM
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_resident_read_probe_test.cpp" -o "$temporary/test"
"$temporary/test"
printf 'PASS: fixed GDDR-to-L1 read, posted-doorbell ordering, data verification/rollback, no retry or payload rewrite after uncertainty\n'
