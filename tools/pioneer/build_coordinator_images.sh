#!/bin/sh
# Offline images ONLY: never builds/installs a driver or accesses a device.
set -eu
[ "$#" = 2 ] || { echo 'Usage: build_coordinator_images.sh CONFIGURED_RISCV64_BUILD FRESH_ABSOLUTE_OUTPUT' >&2; exit 2; }
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
cross=$(CDPATH= cd -- "$1/cross-tools-riscv64/bin" && pwd)/riscv64-unknown-haiku
output=$2
case "$output" in /*) ;; *) echo 'Absolute output required' >&2; exit 2;; esac
[ ! -e "$output" ] && [ ! -L "$output" ] || { echo 'Fresh output required' >&2; exit 2; }
umask 077
mkdir -m 700 "$output"
"$cross-as" -march=rv32i -mabi=ilp32 -mno-relax --defsym TT_FLEET_PERSISTENT=1 --defsym TT_FLEET_COORDINATED=1 \
 "$directory/blackhole_fleet_start.S" -o "$output/worker.o"
"$cross-ld" -m elf32lriscv --no-relax -T "$directory/blackhole_fleet_start.ld" "$output/worker.o" -o "$output/worker.elf"
"$cross-objcopy" -O binary -j .text "$output/worker.elf" "$output/worker.bin"
"$cross-nm" -n "$output/worker.elf" > "$output/worker.symbols"
"$cross-objdump" -d "$output/worker.elf" > "$output/worker.disassembly"
"$cross-g++" -std=c++11 -Os -Wall -Wextra -Werror -ffreestanding -fno-exceptions -fno-rtti \
 -fno-stack-protector -fno-pic -fno-pie -ffunction-sections -fdata-sections -fno-unwind-tables \
 -fno-asynchronous-unwind-tables -msmall-data-limit=0 -mno-relax -march=rv32im -mabi=ilp32 \
 -fstack-usage -c "$directory/blackhole_coordinator.cpp" -o "$output/program.o"
"$cross-as" -march=rv32im -mabi=ilp32 -mno-relax "$directory/blackhole_coordinator_start.S" -o "$output/start.o"
"$cross-ld" -m elf32lriscv --no-relax --gc-sections -T "$directory/blackhole_coordinator.ld" \
 "$output/start.o" "$output/program.o" -Map "$output/program.map" -o "$output/program.elf"
"$cross-objcopy" -O binary -j .text "$output/program.elf" "$output/code.bin"
"$cross-objcopy" -O binary -j .boot "$output/program.elf" "$output/boot.bin"
"$cross-nm" -n "$output/program.elf" > "$output/program.symbols"
"$cross-objdump" -d "$output/program.elf" > "$output/program.disassembly"
shasum -a 256 "$output/worker.bin" "$output/code.bin" "$output/boot.bin"
printf 'OFFLINE coordinator/worker candidates ONLY, no driver exposure: %s\n' "$output"
