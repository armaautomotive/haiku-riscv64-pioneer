#!/bin/sh
set -eu
test_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-smp-tests.XXXXXXXX")
trap 'rm -f "$temporary/test" "$temporary/pioneer_smp_start.inc"; rmdir "$temporary"' EXIT HUP INT TERM
awk '
 /^smp_boot_other_cpus\(uint64 pageTable, uint64 kernel_entry\)$/ { copying = 1; next }
 copying && /^void$/ { found = 1; exit }
 copying { print }
 END { if (!found) exit 1 }
' "$test_dir/../../../src/system/boot/platform/riscv/smp.cpp" > "$temporary/pioneer_smp_start.inc"
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DPIONEER_KEXEC_PROBE -I "$temporary" "$test_dir/pioneer_smp_test.cpp" -o "$temporary/test"
"$temporary/test"
