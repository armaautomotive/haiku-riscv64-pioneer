#!/bin/sh
# CPU-only format/footprint experiment;NO device/file/model/driver access.
set -eu
[ "$#" = 0 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-prepared-q4.XXXXXXXX")
${CXX:-c++} -std=c++17 -O2 -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_q38_prepared_q4_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test"
printf 'Offline prepared-Q4 metadata evidence retained at %s\n' "$temporary"
