#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-affine-replay-words.XXXXXXXX")
for mode in canonical rawdot; do
 extra=
 [ "$mode" = canonical ] || extra=-DPIONEER_Q38_WORK_DOT_ZERO_ELISION
 ${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined $extra \
  "$directory/pioneer_q38_affine_replay_words_test.cpp" -o "$temporary/test-$mode"
 ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test-$mode"
done
printf 'Offline affine-replay word evidence retained at %s\n' "$temporary"
