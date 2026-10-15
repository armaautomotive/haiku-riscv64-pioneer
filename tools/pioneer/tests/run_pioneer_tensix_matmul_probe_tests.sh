#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-tensix-matmul.XXXXXXXX")
trap 'rm -f "$temporary/test"; rmdir "$temporary"' EXIT HUP INT TERM
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_tensix_matmul_probe_test.cpp" -o "$temporary/test"
"$temporary/test"
printf 'PASS: fixed TRISC0 compute ownership/start/drain/stop, L1 rollback and uncertain-state recovery gates\n'
