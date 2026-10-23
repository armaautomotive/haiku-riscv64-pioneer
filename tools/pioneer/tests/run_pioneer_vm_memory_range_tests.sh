#!/bin/sh
# Compile the actual kernel predicates/callers, without touching physical RAM.
set -eu
test_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo=$(CDPATH= cd -- "$test_dir/../../.." && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-vm-range-tests.XXXXXXXX")
trap 'rm -f "$temporary/range.inc" "$temporary/layout.inc" "$temporary/classification.inc" "$temporary/copy.inc" "$temporary/set.inc" "$temporary/mapper.inc" "$temporary/clear.inc" "$temporary/test"; rmdir "$temporary"' EXIT HUP INT TERM
extract()
{
 awk -v name="$2" -v type="$3" '
  $0 ~ "^" name "\\(" {copying=1; print type}
  copying {print}
  copying && /^}/ {found=1; exit}
  END {if(!found) exit 1}
 ' "$1"
}
extract "$repo/src/system/kernel/vm/vm.cpp" validate_memory_range 'static inline bool' > "$temporary/range.inc"
extract "$repo/src/system/kernel/vm/vm.cpp" user_memcpy status_t > "$temporary/copy.inc"
extract "$repo/src/system/kernel/vm/vm.cpp" user_memset status_t > "$temporary/set.inc"
extract "$repo/src/system/kernel/arch/riscv64/RISCV64VMTranslationMap.cpp" 'RISCV64VMPhysicalPageMapper::MemsetPhysical' status_t > "$temporary/mapper.inc"
extract "$repo/src/system/kernel/vm/vm_page.cpp" clear_page 'static void' > "$temporary/clear.inc"
awk '/^#define USER_BASE / || /^#define USER_TOP / {print; found++}
 END {if(found != 2) exit 1}' \
 "$repo/headers/private/kernel/arch/riscv64/arch_kernel.h" > "$temporary/layout.inc"
awk '/^\/\/ Buffers passed in from user-space/ {copying=1; next}
 copying && /^#ifdef __cplusplus/ {found=1; exit}
 copying {print}
 END {if(!found) exit 1}' "$repo/headers/private/kernel/kernel.h" > "$temporary/classification.inc"
${CXX:-c++} -std=c++17 -O2 -Wall -Wextra -Werror \
 -fsanitize=address,undefined -I "$temporary" \
 "$test_dir/pioneer_vm_memory_range_test.cpp" -o "$temporary/test"
"$temporary/test"
