#!/bin/sh
# No device/llama/network access. Exact DEBUG session over synthetic GGUF.
set -eu
[ "$#" = 0 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-rms-validation-session.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DPIONEER_Q38_LEARNED_RMS_VALIDATION_CLIENT=1 -DPIONEER_Q38_CPU_VALIDATION=1 \
 -DPIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE=1 \
 "$directory/pioneer_q38_learned_rms_session_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test"
printf 'DEBUG learned RMS session evidence retained at %s;NO device access\n' "$temporary"
