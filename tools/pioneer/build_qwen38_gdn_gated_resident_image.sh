#!/bin/sh
# OFFLINE fused image only. NO native selection,installation or card access.
set -eu
python3 -c 'import os,sys; sys.exit(0 if all(not k.startswith("PIONEER_") or v=="0" for k,v in os.environ.items()) else 2)' || exit 2
[ "$#" = 2 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
cross=$(CDPATH= cd -- "$1/cross-tools-riscv64/bin" && pwd)/riscv64-unknown-haiku
output=$2
case "$output" in /*) ;; *) exit 2;; esac
[ ! -e "$output" ] && [ ! -L "$output" ] || exit 2
umask 077
mkdir -m 700 "$output"
for unit in gdn_input resident_gdn gdn_recurrent gdn_gated_resident; do
 role=
 optimize=-O2
 case "$unit" in
  gdn_input) role=-DPIONEER_Q38_GDN_INPUT_COMBINED_STAGE=1;;
  resident_gdn) role=-DPIONEER_Q38_RGDN_COMBINED_STAGE=1;;
  gdn_recurrent) optimize=-Os;role=-Dscalar_main=q38_resident_gdn_recurrent_stage;;
  gdn_gated_resident) optimize=-Os;role=-DPIONEER_Q38_GDN_GATED_RESIDENT_IMAGE=1;;
 esac
 "$cross-g++" -std=c++11 "$optimize" -Wall -Wextra -Werror -ffreestanding -fno-exceptions -fno-rtti \
  -fno-stack-protector -fno-pic -fno-pie -ffp-contract=off -ffunction-sections -fdata-sections \
  -fno-unwind-tables -fno-asynchronous-unwind-tables -msmall-data-limit=0 -mno-relax \
  -march=rv32imf -mabi=ilp32 -fstack-usage $role -c "$directory/blackhole_qwen38_$unit.cpp" -o "$output/$unit.o"
done
"$cross-as" -I"$directory" -march=rv32imf -mabi=ilp32 -mno-relax "$directory/blackhole_qwen38_gdn_gated_resident_start.S" -o "$output/start.o"
"$cross-ld" -m elf32lriscv --no-relax --gc-sections -T "$directory/blackhole_scalar.ld" \
 "$output/start.o" "$output/gdn_gated_resident.o" "$output/gdn_recurrent.o" "$output/gdn_input.o" "$output/resident_gdn.o" -Map "$output/program.map" -o "$output/program.elf"
"$cross-objcopy" -O binary -j .text "$output/program.elf" "$output/code.bin"
"$cross-objcopy" -O binary -j .boot "$output/program.elf" "$output/boot.bin"
# Measured new OFFLINE footprint,NOT permission to use old16KiB native snapshots.
[ "$(wc -c < "$output/code.bin")" -le 32768 ]
"$cross-nm" -n "$output/program.elf" > "$output/program.symbols"
"$cross-objdump" -d "$output/program.elf" > "$output/program.disassembly"
awk '$3 ~ /^(div|divu|rem|remu|mulh|mulhu|mulhsu|fdiv[.]s|fsqrt[.]s|fmadd[.]s|fmsub[.]s|fnmadd[.]s|fnmsub[.]s|fcvt[.]w[.]s|fcvt[.]wu[.]s)$/ {bad=1;print} END {exit bad}' "$output/program.disassembly"
awk -F '\t' 'NF==3 && $2 ~ /^[0-9]+$/ {seen++;if($2>1024)bad=1} END {if(!seen || bad)exit 1}' "$output/"*.su
shasum -a 256 "$output/code.bin" "$output/boot.bin"
printf 'OFFLINE QGH1 fused recurrence+peer-ready/NoC+gated normalization;NOT native selected/installed:%s\n' "$output"
