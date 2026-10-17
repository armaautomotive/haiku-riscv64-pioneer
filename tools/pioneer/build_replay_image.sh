#!/bin/sh
# OFFLINE fixed32-slot exact arithmetic replay candidate;no device access.
set -eu
[ "$#" = 2 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
cross=$(CDPATH= cd -- "$1/cross-tools-riscv64/bin" && pwd)/riscv64-unknown-haiku
output=$2
case "$output" in /*) ;; *) exit 2;; esac
[ ! -e "$output" ] && [ ! -L "$output" ] || exit 1
umask 077
mkdir -m 700 "$output"
"$cross-as" -march=rv32i -mabi=ilp32 -mno-relax \
 --defsym TT_UNPACK_NONUNIFORM=1 --defsym TT_UNPACK_FULL_BF16=1 \
 --defsym TT_UNPACK_RESIDENT=1 --defsym TT_UNPACK_GEMV=1 \
 --defsym TT_RUNTIME_MATVEC=1 --defsym TT_RUNTIME_Q8_INLINE=1 \
 --defsym TT_RUNTIME_Q8_PHASE_PIPE=1 --defsym TT_RUNTIME_Q8_ACCUM_PIPE=1 --defsym TT_RUNTIME_Q8_REPLAY=1 \
 "$directory/blackhole_tensix_unpack.S" -o "$output/program.o"
"$cross-ld" -m elf32lriscv --no-relax -T "$directory/blackhole_runtime_matvec.ld" "$output/program.o" -o "$output/program.elf"
"$cross-objcopy" -O binary -j .text "$output/program.elf" "$output/code.bin"
"$cross-nm" -n "$output/program.elf" > "$output/program.symbols"
"$cross-objdump" -d "$output/program.elf" > "$output/program.disassembly"
test "$(wc -c < "$output/code.bin" | tr -d ' ')" = 4096
shasum -a 256 "$output/code.bin"
