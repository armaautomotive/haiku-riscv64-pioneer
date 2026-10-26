#!/bin/sh
# Actual new link subroutine bytes, synthetic predecessor DATA only.
set -eu
[ "$#" = 1 ] || exit 2
here=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
python3 "$here/pioneer_q38_resident_layer_link_image_common.py" "$1"
build=$(mktemp -d /private/tmp/pioneer-q38-layer-link-instructions.XXXXXXXX)
"${CXX:-c++}" -std=c++17 -O1 -g -Wall -Wextra -Werror -ffp-contract=off -fsanitize=address,undefined \
 "$here/pioneer_q38_resident_layer_link_image_test.cpp" -o "$build/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$build/test" "$1"
