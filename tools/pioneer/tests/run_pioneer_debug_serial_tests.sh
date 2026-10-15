#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-debug-serial.XXXXXXXX")
trap 'rm -f "$temporary/test" "$temporary/pioneer_debug_serial.inc" "$temporary/pioneer_gdb_buffer.inc"; rmdir "$temporary"' EXIT HUP INT TERM
extract()
{
 awk -v begin="$1" -v end="$2" '
  index($0, begin) {copying=1; next}
  index($0, end) {found=1; exit}
  copying {print}
  END {if(!found) exit 1}
 ' "$3" > "$4"
}
extract PIONEER_DEBUG_SERIAL_INPUT_BEGIN PIONEER_DEBUG_SERIAL_INPUT_END \
 "$directory/../../../src/system/kernel/arch/riscv64/arch_debug_console.cpp" "$temporary/pioneer_debug_serial.inc"
extract KERNEL_GDB_COMMAND_BUFFER_BEGIN KERNEL_GDB_COMMAND_BUFFER_END \
 "$directory/../../../src/system/kernel/debug/gdb.cpp" "$temporary/pioneer_gdb_buffer.inc"
${CXX:-c++} -std=c++17 -funsigned-char -Wall -Wextra -Werror -fsanitize=address,undefined \
 -I "$temporary" "$directory/pioneer_debug_serial_test.cpp" -o "$temporary/test"
"$temporary/test"
printf 'PASS: production serial blocking/nonblocking input, unsigned-char sentinel, bounded GDB packets and restart\n'
