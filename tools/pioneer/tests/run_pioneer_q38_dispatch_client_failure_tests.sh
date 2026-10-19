#!/bin/sh
set -eu
[ "$#" = 0 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-dispatch-client-failure.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 -I"$directory/cli-stubs" "$directory/pioneer_q38_dispatch_client_failure_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test"
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DPIONEER_Q38_WORK_CLIENT_TEST -DTT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_VECTOR \
 -I"$directory/cli-stubs" "$directory/pioneer_q38_dispatch_client_failure_test.cpp" -o "$temporary/work"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/work"
printf 'Offline real-client failure diagnostics evidence retained at %s\n' "$temporary"
