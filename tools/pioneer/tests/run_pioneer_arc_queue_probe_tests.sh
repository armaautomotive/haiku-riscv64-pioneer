#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-arc-queue.XXXXXXXX")
trap 'rm -f "$temporary/test"; rmdir "$temporary"' EXIT HUP INT TERM
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_arc_queue_probe_test.cpp" -o "$temporary/test"
"$temporary/test"
printf 'PASS: fixed read-only ARC clock query, queue ownership/wrap, dropped writes, timeout and rollback gates\n'
