#!/bin/sh
set -eu
[ "$#" = 1 ] || { echo 'Usage: run_pioneer_brisc_image_tests.sh CONFIGURED_RISCV64_BUILD' >&2; exit 2; }
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
cross=$(CDPATH= cd -- "$1/cross-tools-riscv64/bin" && pwd)/riscv64-unknown-haiku
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-brisc-image.XXXXXXXX")
trap 'rm -f "$temporary/image.o" "$temporary/image.elf" "$temporary/boot.bin" "$temporary/code.bin" "$temporary/test"; rmdir "$temporary"' EXIT HUP INT TERM
"$cross-as" -march=rv32i -mabi=ilp32 -mno-relax \
 "$directory/../blackhole_brisc_smoke.S" -o "$temporary/image.o"
"$cross-ld" -m elf32lriscv --no-relax -T "$directory/../blackhole_brisc_smoke.ld" \
 "$temporary/image.o" -o "$temporary/image.elf"
"$cross-objcopy" -O binary -j .boot "$temporary/image.elf" "$temporary/boot.bin"
"$cross-objcopy" -O binary -j .text "$temporary/image.elf" "$temporary/code.bin"
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_brisc_image_test.cpp" -o "$temporary/test"
"$temporary/test" "$temporary/boot.bin" "$temporary/code.bin"
printf 'PASS: offline RV32I boot/checksum/park image, fixed instruction/data bounds; no hardware execution\n'
