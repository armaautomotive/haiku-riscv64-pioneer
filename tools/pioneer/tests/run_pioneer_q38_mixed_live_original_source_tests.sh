#!/bin/sh
# Source object/reference TU stays separate from its interface consumer.
set -eu
[ "$#" = 1 ] || exit 2
here=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
build=$(mktemp -d /private/tmp/pioneer-mixed-original-source.XXXXXXXX)
"${CXX:-c++}" -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$here/pioneer_q38_mixed_live_original_source_test.cpp" "$here/../pioneer_q38_mixed_live_original_source.cpp" -o "$build/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$build/test" "$1"
