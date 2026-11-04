#!/bin/sh
# CPU instruction observer only; no model/device access.
set -eu
[ "$#" = 0 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q4-fpu-decode.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -ffp-contract=off \
 -fsanitize=address,undefined "$directory/pioneer_q38_q4_fpu_decode_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 python3 "$directory/pioneer_q38_q4_fpu_fraction_test.py" "$temporary/test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -ffp-contract=off \
 -fsanitize=address,undefined "$directory/pioneer_q38_q4_fpu_block_test.cpp" -o "$temporary/block-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/block-test"
for role in _KERNEL_MODE PIONEER_Q38_WORK_INFERENCE PIONEER_Q38_CPU_VALIDATION;do
 if ${CXX:-c++} -std=c++17 -D"$role" -fsyntax-only "$directory/pioneer_q38_q4_fpu_decode_test.cpp" >"$temporary/$role.log" 2>&1;then
  printf 'ERROR:unqualified role unexpectedly compiled:%s\n' "$role" >&2
  exit 1
 fi
 rg -q 'Compressed Q4 FPU operand producer is not qualified' "$temporary/$role.log"
done
printf 'OFFBOARD compressed Q4 FPU operand evidence: %s\n' "$temporary"
