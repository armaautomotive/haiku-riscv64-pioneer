#!/bin/sh
# Actual compiled-code observer only;NO device/model access.
set -eu
[ "$#" = 1 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-mixed-fpu-code.XXXXXXXX")
printf 'OFFBOARD mixed FPU code fixture directory: %s\n' "$temporary"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -ffp-contract=off \
 -fsanitize=address,undefined "$directory/pioneer_q38_mixed_fpu_decode_code_test.cpp" -o "$temporary/test"
for mode in mmio inline;do
 if [ "$mode" = inline ];then
  python3 "$directory/../build_q38_mixed_fpu_decode.py" "$1" "$temporary/$mode" --inline
 else python3 "$directory/../build_q38_mixed_fpu_decode.py" "$1" "$temporary/$mode";fi
 ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" "$temporary/$mode/text.bin" "$temporary/$mode/symbols.txt" "$mode"
done
printf 'OFFBOARD actual mixed FPU RV32I code evidence: %s\n' "$temporary"
