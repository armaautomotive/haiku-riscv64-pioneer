#!/bin/sh
set -eu
test_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-memory-tests.XXXXXXXX")
trap 'rm -f "$temporary/test" "$temporary/pioneer_memory_parser.inc" "$temporary/pioneer_memory_mapping.inc" "$temporary/pioneer_memory_reservation.inc"; rmdir "$temporary"' EXIT HUP INT TERM
awk '
 /PIONEER_MEMORY_PARSER_BEGIN/ {copying=1; next}
 /PIONEER_MEMORY_PARSER_END/ {found=1; exit}
 copying {print}
 END {if(!found) exit 1}
' "$test_dir/../../../src/system/boot/platform/riscv/fdt.cpp" > "$temporary/pioneer_memory_parser.inc"
awk '
 /^SetupPhysicalMemoryMap\(\)$/ {copying=1; next}
 copying && /^static void$/ {found=1; exit}
 copying {print}
 END {if(!found) exit 1}
' "$test_dir/../../../src/system/boot/platform/riscv/mmu.cpp" > "$temporary/pioneer_memory_mapping.inc"
awk '
 /PIONEER_LARGE_MAP_RESERVATION_BEGIN/ {copying=1; next}
 /PIONEER_LARGE_MAP_RESERVATION_END/ {found=1; exit}
 copying {print}
 END {if(!found) exit 1}
' "$test_dir/../../../src/system/kernel/arch/riscv64/arch_vm.cpp" > "$temporary/pioneer_memory_reservation.inc"
fdt_prefix=${PIONEER_LIBFDT_PREFIX:-/opt/homebrew/opt/dtc}
${CXX:-c++} -std=c++17 -O2 -Wall -Wextra -Werror -fsanitize=address,undefined \
 -I "$temporary" -I "$fdt_prefix/include" -L "$fdt_prefix/lib" \
 "$test_dir/pioneer_memory_test.cpp" -lfdt -o "$temporary/test"
"$temporary/test" "${1:-$test_dir/../../../generated.pioneer/haiku-kexec-loader-20261013/smoke-1/live-fdt.dtb}"
