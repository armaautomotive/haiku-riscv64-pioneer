#!/bin/sh
# Diagnostic original-GGUF reference and strict source-bound host session.
set -eu
[ "$#" = 0 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-embedding-validation-session.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DPIONEER_Q38_EMBEDDING_VALIDATION_CLIENT=1 -DPIONEER_Q38_CPU_VALIDATION=1 \
 "$directory/pioneer_q38_embedding_validation_session_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test"
printf 'DEBUG embedding session evidence retained at %s;NO device access\n' "$temporary"
