#!/bin/sh
# Read-only authenticated original model;actual instructions/byte endpoints.
set -eu
[ "$#" = 2 ] || exit 2
here=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
build=$(mktemp -d /private/tmp/pioneer-q38-residual-rms-original-tests.XXXXXXXX)
"${CXX:-c++}" -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$here/pioneer_q38_residual_rms_actual_model_test.cpp" -o "$build/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$build/test" "$@"
printf 'Actual QRR1/original post-attention weights evidence:%s;NO device access\n' "$build"
