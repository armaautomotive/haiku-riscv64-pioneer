#!/bin/sh
set -eu
[ "$#" = 1 ] || { echo 'Usage: run_pioneer_tensix_image_tests.sh CONFIGURED_RISCV64_BUILD' >&2; exit 2; }
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
cross=$(CDPATH= cd -- "$1/cross-tools-riscv64/bin" && pwd)/riscv64-unknown-haiku
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-tensix-image.XXXXXXXX")
trap 'rm -f "$temporary/image.o" "$temporary/image.elf" "$temporary/config.o" "$temporary/config.elf" "$temporary/config.bin" "$temporary/boot.bin" "$temporary/code.bin" "$temporary/test"; rmdir "$temporary"' EXIT HUP INT TERM
"$cross-as" -march=rv32i -mabi=ilp32 -mno-relax "$directory/../blackhole_tensix_matmul.S" -o "$temporary/image.o"
"$cross-ld" -m elf32lriscv --no-relax -T "$directory/../blackhole_tensix_matmul.ld" "$temporary/image.o" -o "$temporary/image.elf"
"$cross-objcopy" -O binary -j .boot "$temporary/image.elf" "$temporary/boot.bin"
"$cross-objcopy" -O binary -j .text "$temporary/image.elf" "$temporary/code.bin"
"$cross-as" -march=rv32i -mabi=ilp32 -mno-relax --defsym TT_CONFIG_SMOKE=1 "$directory/../blackhole_tensix_matmul.S" -o "$temporary/config.o"
"$cross-ld" -m elf32lriscv --no-relax -T "$directory/../blackhole_tensix_matmul.ld" "$temporary/config.o" -o "$temporary/config.elf"
"$cross-objcopy" -O binary -j .text "$temporary/config.elf" "$temporary/config.bin"
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined "$directory/pioneer_tensix_image_test.cpp" -o "$temporary/test"
"$temporary/test" "$temporary/boot.bin" "$temporary/code.bin" "$temporary/config.bin"
printf 'PASS: fixed RV32I image, whitelisted MMIO and reference matrix fixture; no hardware operation\n'
