#!/bin/sh
# Pure DATA only; no hardware/SDK/native grants.
set -eu
here=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
build=$(mktemp -d /private/tmp/pioneer-layer-physical-data.XXXXXXXX)
"${CXX:-c++}" -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$here/pioneer_q38_resident_layer_physical_data_test.cpp" -o "$build/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$build/test"
