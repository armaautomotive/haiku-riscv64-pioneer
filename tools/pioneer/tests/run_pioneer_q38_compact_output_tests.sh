#!/bin/sh
# OFFLINE ONLY:bounded layout/transport arithmetic,NO card access.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-compact-output.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DPIONEER_Q38_WORK_COMPACT_OUTPUT "$directory/pioneer_q38_compact_output_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test"
printf 'Offline compact-output evidence retained at %s\n' "$temporary"
