#!/bin/sh
# Compile the standalone 4B ROOT and checked Tensix projection code.
set -eu
[ "$#" = 2 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
cross=$(CDPATH= cd -- "$1/cross-tools-riscv64/bin" && pwd)/riscv64-unknown-haiku
output=$2
case "$output" in /*) ;; *) exit 2;; esac
umask 077
mkdir -m 700 "$output"
flags='-std=c++17 -Os -Wall -Wextra -Werror -ffreestanding -fno-exceptions -fno-rtti -fno-stack-protector -fno-pic -fno-pie -fno-builtin -ffp-contract=off -ffunction-sections -fdata-sections -fno-unwind-tables -fno-asynchronous-unwind-tables -msmall-data-limit=0 -mno-relax -march=rv32imf_zicsr -mabi=ilp32 -fstack-usage'
for source in blackhole_qwen35_work blackhole_qwen35_token_core blackhole_qwen35_device_io; do
 "$cross-g++" $flags -c "$directory/$source.cpp" -o "$output/$source.o"
done
"$cross-g++" $flags -DPIONEER_Q35_PROJECTION_IMAGE=1 -Dblock_main=q35_work_matrix_band \
 -c "$directory/blackhole_qwen38_rows.cpp" -o "$output/matrix.o"
"$cross-as" -march=rv32imf_zicsr -mabi=ilp32 -mno-relax "$directory/blackhole_qwen35_work_start.S" -o "$output/start.o"
"$cross-ld" -m elf32lriscv --no-relax --gc-sections -T "$directory/blackhole_qwen35_work.ld" \
 "$output/start.o" "$output/blackhole_qwen35_work.o" "$output/blackhole_qwen35_token_core.o" \
 "$output/blackhole_qwen35_device_io.o" "$output/matrix.o" -Map "$output/work.map" -o "$output/work.elf"
for section in boot text; do "$cross-objcopy" -O binary -j ".$section" "$output/work.elf" "$output/$section.bin"; done
"$cross-nm" -n "$output/work.elf" > "$output/work.symbols"
"$cross-objdump" -d "$output/work.elf" > "$output/work.disassembly"
"$cross-nm" -u "$output/work.elf" > "$output/work.imports"
test ! -s "$output/work.imports"
awk '$3 ~ /^(div|divu|rem|remu|mulh|mulhu|mulhsu|fdiv[.]s|fsqrt[.]s|fmadd[.]s|fmsub[.]s|fnmadd[.]s|fnmsub[.]s|fcvt[.]w[.]s|fcvt[.]wu[.]s)$/ {
 print; bad=1
} END {exit bad}' "$output/work.disassembly"
shasum -a 256 "$output/boot.bin" "$output/text.bin"
