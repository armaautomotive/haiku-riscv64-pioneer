#!/bin/sh
# Host metadata only. Optional legacy header is an immutable archived source.
set -eu
[ "$#" -le 1 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-ready-snapshot.XXXXXXXX")
extra=
if [ "$#" = 1 ]; then
 [ -f "$1" ] || exit 2
 extra="-DPIONEER_Q38_READY_SNAPSHOT_LEGACY -DPIONEER_Q38_READY_SNAPSHOT_HEADER=\"$1\""
fi
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined $extra \
 -DTT_PIONEER_EXPERIMENTAL_Q38_COHORT_SEALED -I"$directory/.." \
 "$directory/pioneer_q38_ready_snapshot_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test"
printf 'Offline host metadata evidence retained at %s\n' "$temporary"
