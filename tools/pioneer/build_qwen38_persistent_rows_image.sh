#!/bin/sh
# OFFLINE ONLY persistent mixed-quant peer;NO driver install/card operations.
set -eu
[ "$#" = 2 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
cross=$(CDPATH= cd -- "$1/cross-tools-riscv64/bin" && pwd)/riscv64-unknown-haiku
output=$2
case "${PIONEER_Q38_VECTOR_PERSISTENT_IMAGE:-0}" in
 0) source=blackhole_qwen38_persistent.cpp;;
 1) source=blackhole_qwen38_vector_persistent.cpp;;
 2) source=blackhole_qwen38_vector_reuse_persistent.cpp;;
 3) source=blackhole_qwen38_vector_fixed_persistent.cpp;;
 4) source=blackhole_qwen38_vector_phase_persistent.cpp;;
 5) source=blackhole_qwen38_vector_resident_persistent.cpp;;
 *) exit 2;;
esac
case "$output" in /*) ;; *) exit 2;; esac
[ ! -e "$output" ] && [ ! -L "$output" ] || exit 1
umask 077; mkdir -m 700 "$output"
"$cross-g++" -std=c++11 -O2 -Wall -Wextra -Werror -ffreestanding -fno-exceptions -fno-rtti \
 -fno-stack-protector -fno-pic -fno-pie -ffp-contract=off -ffunction-sections -fdata-sections \
 -fno-unwind-tables -fno-asynchronous-unwind-tables -msmall-data-limit=0 -mno-relax \
 -march=rv32imf -mabi=ilp32 -fstack-usage -DPIONEER_Q38_FAST_PANEL -DPIONEER_Q38_FAST_SCALE \
 -c "$directory/$source" -o "$output/program.o"
"$cross-as" -march=rv32imf -mabi=ilp32 -mno-relax "$directory/blackhole_qwen38_persistent_start.S" -o "$output/start.o"
"$cross-ld" -m elf32lriscv --no-relax --gc-sections -T "$directory/blackhole_qwen38_block.ld" \
 "$output/start.o" "$output/program.o" -Map "$output/program.map" -o "$output/program.elf"
for section in boot text; do "$cross-objcopy" -O binary -j ".$section" "$output/program.elf" "$output/$section.bin"; done
"$cross-nm" -n "$output/program.elf" > "$output/program.symbols"
"$cross-objdump" -d "$output/program.elf" > "$output/program.disassembly"
shasum -a 256 "$output/boot.bin" "$output/text.bin"
printf 'OFFLINE persistent peer only;NO driver selects it/NO native or graph qualification: %s\n' "$output"
