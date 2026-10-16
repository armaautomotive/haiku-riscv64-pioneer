#!/bin/sh
set -eu
[ "$#" = 1 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
cross=$(CDPATH= cd -- "$1/cross-tools-riscv64/bin" && pwd)/riscv64-unknown-haiku
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-runtime-readback.XXXXXXXX")
trap 'rm -f "$temporary/program.o" "$temporary/program.elf" "$temporary/boot.bin" "$temporary/code.bin" "$temporary/test"; rmdir "$temporary"' EXIT HUP INT TERM
"$cross-as" -march=rv32i -mabi=ilp32 -mno-relax --defsym TT_UNPACK_NONUNIFORM=1 --defsym TT_UNPACK_FULL_BF16=1 --defsym TT_UNPACK_RESIDENT=1 --defsym TT_UNPACK_GEMV=1 --defsym TT_RUNTIME_MATVEC=1 --defsym TT_RUNTIME_Q8_READBACK=1 "$directory/../blackhole_tensix_unpack.S" -o "$temporary/program.o"
"$cross-ld" -m elf32lriscv --no-relax -T "$directory/../blackhole_runtime_matvec.ld" "$temporary/program.o" -o "$temporary/program.elf"
"$cross-objcopy" -O binary -j .boot "$temporary/program.elf" "$temporary/boot.bin"
"$cross-objcopy" -O binary -j .text "$temporary/program.elf" "$temporary/code.bin"
${CXX:-c++} -std=c++17 -O1 -Wall -Wextra -Werror -fsanitize=address,undefined "$directory/pioneer_runtime_readback_image_test.cpp" -o "$temporary/test"
"$temporary/test" "$temporary/boot.bin" "$temporary/code.bin"
