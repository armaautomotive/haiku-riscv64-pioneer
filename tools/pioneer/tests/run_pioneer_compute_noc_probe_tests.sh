#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-compute-noc.XXXXXXXX")
trap 'rm -f "$temporary/test"; rmdir "$temporary"' EXIT HUP INT TERM
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_compute_noc_probe_test.cpp" -o "$temporary/test"
"$temporary/test"
printf 'PASS: fixed compute NoC read-only route/idle proof; dropped writes, active commands/reads, identity/source faults and TLB rollback\n'
