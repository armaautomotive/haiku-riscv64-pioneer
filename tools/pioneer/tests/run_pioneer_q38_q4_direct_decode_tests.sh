#!/bin/sh
# OFFLINE pure instruction stream proof;no native execution.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-q4-direct-decode.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DPIONEER_Q38_WORK_Q4_DIRECT_DECODE -DPIONEER_Q38_WORK_Q4_NIBBLE \
 -DPIONEER_Q38_NATIVE_OPERAND_STORE -DPIONEER_Q38_WORK_DIRECT_SRCB \
 -DPIONEER_Q38_STATIC_VECTOR -DPIONEER_Q38_REPLAY_OPERAND \
 "$directory/pioneer_q38_q4_direct_decode_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test"
printf 'Offline direct Q4 stream evidence retained at %s\n' "$temporary"
