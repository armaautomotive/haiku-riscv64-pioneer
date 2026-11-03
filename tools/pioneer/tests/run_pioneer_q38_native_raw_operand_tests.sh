#!/bin/sh
# Actual SFPU emitter, independent CPU lane model; no real model/device.
set -eu
[ "$#" = 0 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-native-raw-operand.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -ffp-contract=off -fsanitize=address,undefined \
 "$directory/pioneer_q38_native_raw_operand_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test"
for forbidden in _KERNEL_MODE PIONEER_Q38_WORK_INFERENCE PIONEER_Q38_CPU_VALIDATION; do
 if ${CXX:-c++} -std=c++17 -fsyntax-only -x c++ -D"$forbidden" "$directory/../PioneerQ38NativeRawOperand.h" > "$temporary/$forbidden.stdout" 2> "$temporary/$forbidden.stderr";then
  echo 'STOP unqualified native-raw helper unexpectedly accepted a driver/Work/validation build' >&2;exit 1
 fi
 rg -q 'unqualified offboard prerequisite' "$temporary/$forbidden.stderr"
done
echo 'PASS native-raw build guards:3 driver/Work/validation selections rejected before use'
printf 'CPU-only native raw operand evidence: %s\n' "$temporary"
