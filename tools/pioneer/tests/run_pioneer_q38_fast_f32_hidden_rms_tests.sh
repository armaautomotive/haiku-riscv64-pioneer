#!/bin/sh
# Host-only candidate screening. No accelerator or model access.
set -eu
[ "$#" = 0 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-fast-rms.XXXXXXXX")
${CXX:-c++} -std=c++17 -O2 -g -Wall -Wextra -Werror -ffp-contract=off -fsanitize=address,undefined \
 "$directory/pioneer_q38_fast_f32_hidden_rms_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test"
for role in _KERNEL_MODE PIONEER_Q38_WORK_INFERENCE TT_PIONEER_Q38_WORK_MODE PIONEER_Q38_TOKEN_NATIVE_CONTEXT PIONEER_Q38_FULL_OFFLOAD_NATIVE PIONEER_Q38_FULL_OFFLOAD_CLIENT __FAST_MATH__; do
 if ${CXX:-c++} -std=c++17 -D"$role" -E "$directory/pioneer_q38_fast_f32_hidden_rms_test.cpp" > /dev/null 2> "$temporary/$role.stderr"; then exit 1; fi
 rg -q 'Approximate RMS is an isolated diagnostic' "$temporary/$role.stderr"
done
printf 'PASS fast RMS role exclusions; host-only artifacts at %s\n' "$temporary"
