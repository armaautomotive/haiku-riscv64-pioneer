#!/bin/sh
set -eu
test_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-memory-tests.XXXXXXXX")
trap 'rm -f "$temporary/test" "$temporary/pioneer_memory_parser.inc" "$temporary/pioneer_memory_mapping.inc" "$temporary/pioneer_memory_reservation.inc" "$temporary/pioneer_poison_guard.inc"; rmdir "$temporary"' EXIT HUP INT TERM
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
' "$test_dir/../../../src/system/kernel/vm/vm.cpp" > "$temporary/pioneer_memory_reservation.inc"
awk '
 /PIONEER_POISON_MAP_GUARD_BEGIN/ {copying=1; next}
 /PIONEER_POISON_MAP_GUARD_END/ {found=1; exit}
 copying {print}
 END {if(!found) exit 1}
' "$test_dir/../../../src/system/kernel/vm/vm.cpp" > "$temporary/pioneer_poison_guard.inc"
# Catch the actual boot ordering, not just a correct isolated predicate.
awk '
 /vm_kernel_args_init_post_area\(args\);/ {converted=NR}
 /PIONEER_LARGE_MAP_RESERVATION_BEGIN/ {reservation=NR; if(!converted) exit 1}
 /vm_block_address_range\("overflow protection"/ {overflow=NR; if(!reservation) exit 1}
 /block_debug_address_range\(args, "/ {guards++; if(!reservation || !overflow) exit 1}
 END {if(!converted || !reservation || !overflow || guards != 4) exit 1}
' "$test_dir/../../../src/system/kernel/vm/vm.cpp"
fdt_prefix=${PIONEER_LIBFDT_PREFIX:-/opt/homebrew/opt/dtc}
${CXX:-c++} -std=c++17 -O2 -Wall -Wextra -Werror -fsanitize=address,undefined \
 -I "$temporary" -I "$fdt_prefix/include" -L "$fdt_prefix/lib" \
 "$test_dir/pioneer_memory_test.cpp" -lfdt -o "$temporary/test"
"$temporary/test" "${1:-$test_dir/../../../generated.pioneer/haiku-kexec-loader-20261013/smoke-1/live-fdt.dtb}"
