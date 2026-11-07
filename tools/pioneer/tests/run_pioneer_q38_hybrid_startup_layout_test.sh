#!/bin/sh
# CPU-only actual startup serialization against independent hybrid layout.
set -eu
[ "$#" = 0 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-hybrid-startup.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_q38_hybrid_startup_layout_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test"
for role in _KERNEL_MODE PIONEER_Q38_WORK_INFERENCE TT_PIONEER_Q38_WORK_MODE PIONEER_Q38_FULL_OFFLOAD_NATIVE PIONEER_Q38_FULL_OFFLOAD_CLIENT; do
 if ${CXX:-c++} -std=c++17 -D"$role" -E "$directory/pioneer_q38_hybrid_startup_layout_test.cpp" > /dev/null 2> "$temporary/$role.stderr"; then exit 1; fi
 rg -q 'Hybrid canonical repack is diagnostic DATA only' "$temporary/$role.stderr"
done
printf 'PASS role exclusions; offline test artifacts at %s\n' "$temporary"
