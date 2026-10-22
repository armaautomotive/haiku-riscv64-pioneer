#!/bin/sh
# Host numerical verification ONLY;NOT a native inference backend.
set -eu
[ "$#" = 0 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-learned-rms.XXXXXXXX")
${CXX:-c++} -std=c++17 -O2 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_q38_learned_rms_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test"
printf 'Fixed learned RMS numerical evidence:%s;NO device access\n' "$temporary"
