#!/bin/sh
set -eu
[ "$#" = 0 ] || exit 2
here=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
build=$(mktemp -d /private/tmp/pioneer-q38-gdn-layer-progress.XXXXXXXX)
"${CXX:-c++}" -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$here/pioneer_q38_gdn_layer_progress_test.cpp" -o "$build/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$build/test"
printf 'Metadata-only evidence:%s;NO native/device access\n' "$build"
