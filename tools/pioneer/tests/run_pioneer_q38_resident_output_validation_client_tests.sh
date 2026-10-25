#!/bin/sh
# Actual production client lifecycle with TEST bytes/OS only;NO card.
set -eu
here=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
build=$(mktemp -d /private/tmp/pioneer-q38-output-debug68-client-tests.XXXXXXXX)
"${CXX:-c++}" -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$here/pioneer_q38_resident_output_validation_session_test.cpp" -o "$build/session"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$build/session"
printf 'CPU-only DEBUG68 client evidence retained at %s;NO device access\n' "$build"
