#!/bin/sh
# Exact complete bytes/full original footprint DATA, not native admission.
set -eu
[ "$#" = 2 ] || exit 2
here=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
build=$(mktemp -d /private/tmp/pioneer-q38-next-gdn-output-catalog.XXXXXXXX)
"${CXX:-c++}" -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DPIONEER_Q38_GDN_GATED_NATIVE=1 -DPIONEER_Q38_GDN_RECURRENT_NATIVE=1 \
 "$here/pioneer_q38_next_gdn_output_catalog_test.cpp" -o "$build/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$build/test" "$1" "$2"
