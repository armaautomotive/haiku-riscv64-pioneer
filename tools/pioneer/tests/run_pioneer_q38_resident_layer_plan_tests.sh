#!/bin/sh
# Pure canonical DATA tests;NO native build,device IO or execution grant.
set -eu
here=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
build=$(mktemp -d /private/tmp/pioneer-q38-resident-layer-plan.XXXXXXXX)
"${CXX:-c++}" -std=c++17 -O1 -g -Wall -Wextra -Werror -ffp-contract=off -fsanitize=address,undefined \
 "$here/pioneer_q38_resident_layer_plan_test.cpp" -o "$build/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$build/test"
