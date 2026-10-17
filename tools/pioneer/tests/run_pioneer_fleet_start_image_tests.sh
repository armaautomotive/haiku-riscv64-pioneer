#!/bin/sh
set -eu
[ "$#" = 1 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
cross=$(CDPATH= cd -- "$1/cross-tools-riscv64/bin" && pwd)/riscv64-unknown-haiku
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-fleet-start.XXXXXXXX")
trap 'rm -f "$temporary/program.o" "$temporary/program.elf" "$temporary/code.bin" "$temporary/test"; rmdir "$temporary"' EXIT HUP INT TERM
"$cross-as" -march=rv32i -mabi=ilp32 -mno-relax "$directory/../blackhole_fleet_start.S" -o "$temporary/program.o"
"$cross-ld" -m elf32lriscv --no-relax -T "$directory/../blackhole_fleet_start.ld" "$temporary/program.o" -o "$temporary/program.elf"
"$cross-objcopy" -O binary -j .text "$temporary/program.elf" "$temporary/code.bin"
${CXX:-c++} -std=c++17 -O1 -Wall -Wextra -Werror -fsanitize=address,undefined "$directory/pioneer_fleet_start_image_test.cpp" -o "$temporary/test"
"$temporary/test" "$temporary/code.bin"
