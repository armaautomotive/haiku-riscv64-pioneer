#!/bin/sh
# Full synthetic packet stream and failure checks; no P100A access.
set -eu
[ "$#" = 0 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-hybrid-span.XXXXXXXX")
${CXX:-c++} -std=c++17 -O2 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_q38_hybrid_span_upload_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test"
${CXX:-c++} -std=c++17 -O2 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_q38_hybrid_prepared_upload_test.cpp" "$directory/../PioneerModelWeightCache.cpp" -o "$temporary/prepared-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/prepared-test"
for role in _KERNEL_MODE PIONEER_Q38_WORK_INFERENCE TT_PIONEER_Q38_WORK_MODE PIONEER_Q38_FULL_OFFLOAD_CLIENT PIONEER_Q38_FULL_OFFLOAD_NATIVE PIONEER_Q38_CPU_VALIDATION PIONEER_Q38_FULL_MODEL_CPU_REFERENCE; do
 if ${CXX:-c++} -std=c++17 -D"$role" -E "$directory/pioneer_q38_hybrid_span_upload_test.cpp" > /dev/null 2> "$temporary/$role.stderr"; then exit 1; fi
 rg -q 'Hybrid span upload is a separate experimental diagnostic' "$temporary/$role.stderr"
done
printf 'PASS hybrid upload role exclusions; offline test artifacts at %s\n' "$temporary"
