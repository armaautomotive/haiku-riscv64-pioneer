#!/bin/sh
set -eu
[ "$#" = 0 ] || exit 2
here=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
build=$(mktemp -d /private/tmp/pioneer-debug64-client-tests.XXXXXXXX)
"${CXX:-c++}" -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DPIONEER_Q38_CONV_SILU_PRECISION_VALIDATION_CLIENT=1 \
 "$here/pioneer_q38_gdn_rendezvous_validation_session_test.cpp" -o "$build/session"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$build/session"
printf 'DEBUG64 client evidence:%s;NO device access\n' "$build"
