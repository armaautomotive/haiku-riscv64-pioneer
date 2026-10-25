#!/bin/sh
# Actual expanded ROOT's norm ROM;CPU byte endpoints only,NO device.
set -eu
[ "$#" = 1 ] || exit 2
here=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
build=$(mktemp -d /private/tmp/pioneer-q38-resident-layer-norm.XXXXXXXX)
"${CXX:-c++}" -std=c++17 -O1 -g -Wall -Wextra -Werror -ffp-contract=off -fsanitize=address,undefined \
 "$here/pioneer_q38_resident_layer_norm_image_test.cpp" -o "$build/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$build/test" "$1"
