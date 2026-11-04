#!/bin/sh
# CPU instruction observer and exact rational checks;NO device/model access.
set -eu
[ "$#" = 0 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-mixed-fpu-decode.XXXXXXXX")
printf 'OFFBOARD mixed FPU fixture directory: %s\n' "$temporary"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -ffp-contract=off \
 -fsanitize=address,undefined "$directory/pioneer_q38_mixed_fpu_decode_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 python3 "$directory/pioneer_q38_mixed_fpu_fraction_test.py" "$temporary/test"
for role in _KERNEL_MODE PIONEER_Q38_WORK_INFERENCE PIONEER_Q38_CPU_VALIDATION __FAST_MATH__;do
 if ${CXX:-c++} -std=c++17 -D"$role" -fsyntax-only "$directory/pioneer_q38_mixed_fpu_decode_test.cpp" >"$temporary/$role.log" 2>&1;then
  printf 'ERROR:unqualified mixed role unexpectedly compiled:%s\n' "$role" >&2
  exit 1
 fi
 rg -q 'Mixed Q6/Q8 FPU operand producer is offboard only' "$temporary/$role.log"
done
printf 'OFFBOARD mixed FPU operand evidence: %s\n' "$temporary"
