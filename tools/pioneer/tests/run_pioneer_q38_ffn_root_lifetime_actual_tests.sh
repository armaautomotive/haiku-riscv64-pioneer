#!/bin/sh
# SAME lifetime controller + actual ROOT/ALL119 instruction machines.
# Read-only binary artifacts;NO native/card/SDK access or timing claims.
set -eu
[ "$#" = 2 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-ffn-root-lifetime-actual.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DPIONEER_Q38_FFN_GRAPH_INPUT_ROOT=1 -DPIONEER_Q38_TEST_WORK_DOT_ZERO_ELISION \
 -DPIONEER_Q38_TEST_AFFINE_REPLAY -DPIONEER_Q38_TEST_BLOCK_SCALE_REUSE \
 -DPIONEER_Q38_TEST_INT_ACCUMULATOR -DPIONEER_Q38_TEST_PIPELINED_MATRIX \
 "$directory/pioneer_q38_ffn_root_lifetime_actual_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" "$1" "$2"
for batch in 257 8192;do
 ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" "$1" "$2" "$batch"
done
for packet in 1 5000 9000;do
 ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" "$1" "$2" 4096 "$packet"
done
printf 'Offline ROOT+ALL119 actual-instruction lifetime evidence retained at %s;NO device access\n' "$temporary"
