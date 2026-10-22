#!/bin/sh
# Actual held-only code ownership algorithm;NO native/device IO.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-ffn-code-owner.XXXXXXXX")
for role in basic qualified;do
 flags=;[ "$role" != qualified ] || flags=-DPIONEER_Q38_TEST_QUALIFIED_LEGACY
 ${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined $flags \
  "$directory/pioneer_q38_ffn_graph_code_owner_test.cpp" -o "$temporary/$role"
 ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/$role"
done
printf 'Complete FFN code-owner evidence retained at %s;NO native/device IO or launch qualification\n' "$temporary"
