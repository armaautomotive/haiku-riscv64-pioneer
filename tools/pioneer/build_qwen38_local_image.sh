#!/bin/sh
# OFFLINE local arithmetic image ONLY; no driver build/install/device access.
set -eu
[ "$#" = 3 ] || exit 2
case "$1" in delta|delta_vector|delta_vector_probe|packed|activation|activation0) kind=$1;; *) exit 2;; esac
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
cross=$(CDPATH= cd -- "$2/cross-tools-riscv64/bin" && pwd)/riscv64-unknown-haiku
output=$3
case "$output" in /*) ;; *) exit 2;; esac
[ ! -e "$output" ] && [ ! -L "$output" ] || exit 2
umask 077
mkdir -m 700 "$output"
"$cross-g++" -std=c++11 -O2 -Wall -Wextra -Werror -ffreestanding -fno-exceptions -fno-rtti \
 -fno-stack-protector -fno-pic -fno-pie -ffp-contract=off -ffunction-sections -fdata-sections \
 -fno-unwind-tables -fno-asynchronous-unwind-tables -msmall-data-limit=0 -mno-relax \
 -march=rv32imf -mabi=ilp32 -fstack-usage -c "$directory/blackhole_qwen38_$kind.cpp" -o "$output/program.o"
"$cross-as" -I"$directory" -march=rv32imf -mabi=ilp32 -mno-relax "$directory/blackhole_qwen38_${kind}_start.S" -o "$output/start.o"
linker=$directory/blackhole_scalar.ld
[ "$kind" != delta_vector_probe ] || linker=$directory/blackhole_qwen38_delta_vector_probe.ld
"$cross-ld" -m elf32lriscv --no-relax --gc-sections -T "$linker" \
 "$output/start.o" "$output/program.o" -Map "$output/program.map" -o "$output/program.elf"
"$cross-objcopy" -O binary -j .text "$output/program.elf" "$output/code.bin"
"$cross-objcopy" -O binary -j .boot "$output/program.elf" "$output/boot.bin"
"$cross-nm" -n "$output/program.elf" > "$output/program.symbols"
"$cross-objdump" -d "$output/program.elf" > "$output/program.disassembly"
shasum -a 256 "$output/code.bin" "$output/boot.bin"
printf 'OFFLINE Qwen3.8 local %s candidate,NOT installed/driver-selected: %s\n' "$kind" "$output"
