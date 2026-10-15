#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-spin-idle.XXXXXXXX")
trap 'rm -f "$temporary/test-0" "$temporary/test-1" "$temporary/pioneer_idle.inc"; rmdir "$temporary"' EXIT HUP INT TERM
awk '
 /PIONEER_IDLE_PROFILE_BEGIN/ {copying=1; next}
 /PIONEER_IDLE_PROFILE_END/ {found=1; exit}
 copying {print}
 END {if(!found) exit 1}
' "$directory/../../../src/system/kernel/cpu.cpp" > "$temporary/pioneer_idle.inc"
for diagnostic in 0 1; do
 defines=
 [ "$diagnostic" = 0 ] || defines=-DPIONEER_DIAGNOSTIC_SPIN_IDLE
 ${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -Wno-unused-function \
  -D__riscv -DKDEBUG=0 $defines -fsanitize=address,undefined \
  -I "$temporary" "$directory/pioneer_spin_idle_test.cpp" -o "$temporary/test-$diagnostic"
 "$temporary/test-$diagnostic"
done
printf 'PASS: production idle comparison is opt-in/64-CPU-only; normal and other CPU counts preserve idle behavior\n'
