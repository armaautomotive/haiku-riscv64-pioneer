#!/bin/sh
# ACTUAL emitted QRR2;only NoC/GDDR endpoints are simulated.
set -eu
[ "$#" = 1 ] || { [ "$#" = 2 ] && [ "$2" = --faults ]; } || exit 2
here=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
build=$(mktemp -d /private/tmp/pioneer-q38-next-residual-rms-tests.XXXXXXXX)
"${CXX:-c++}" -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$here/pioneer_q38_next_residual_rms_image_test.cpp" -o "$build/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$build/test" "$@"
printf 'Actual QRR2 instruction evidence:%s;NO device access\n' "$build"
