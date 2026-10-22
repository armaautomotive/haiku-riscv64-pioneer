#!/bin/sh
# Separate OFFLINE QPE1 image;NO old builder role/native registry changes.
set -eu
# No inherited feature may silently promote this standalone arithmetic ROM.
python3 -c 'import os,sys; sys.exit(0 if all(not k.startswith("PIONEER_") or v=="0" for k,v in os.environ.items()) else 2)' || exit 2
[ "$#" = 2 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
cross=$(CDPATH= cd -- "$1/cross-tools-riscv64/bin" && pwd)/riscv64-unknown-haiku
output=$2
case "$output" in /*) ;; *) exit 2;; esac
[ ! -e "$output" ] && [ ! -L "$output" ] || exit 2
umask 077
mkdir -m 700 "$output"
"$cross-g++" -std=c++11 -O2 -Wall -Wextra -Werror -ffreestanding -fno-exceptions -fno-rtti \
 -fno-stack-protector -fno-pic -fno-pie -ffp-contract=off -ffunction-sections -fdata-sections \
 -fno-unwind-tables -fno-asynchronous-unwind-tables -msmall-data-limit=0 -mno-relax \
 -march=rv32imf -mabi=ilp32 -fstack-usage -c "$directory/blackhole_qwen38_embedding.cpp" -o "$output/program.o"
"$cross-as" -I"$directory" -march=rv32imf -mabi=ilp32 -mno-relax "$directory/blackhole_qwen38_embedding_start.S" -o "$output/start.o"
"$cross-ld" -m elf32lriscv --no-relax --gc-sections -T "$directory/blackhole_scalar.ld" \
 "$output/start.o" "$output/program.o" -Map "$output/program.map" -o "$output/program.elf"
"$cross-objcopy" -O binary -j .text "$output/program.elf" "$output/code.bin"
"$cross-objcopy" -O binary -j .boot "$output/program.elf" "$output/boot.bin"
"$cross-nm" -n "$output/program.elf" > "$output/program.symbols"
"$cross-objdump" -d "$output/program.elf" > "$output/program.disassembly"
shasum -a 256 "$output/code.bin" "$output/boot.bin"
printf 'OFFLINE QPE1 embedding arithmetic,NOT installed/driver-selected: %s\n' "$output"
