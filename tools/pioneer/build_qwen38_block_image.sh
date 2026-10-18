#!/bin/sh
# OFFLINE one-core complete quantization-block image. No driver install/card.
set -eu
[ "$#" = 2 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
cross=$(CDPATH= cd -- "$1/cross-tools-riscv64/bin" && pwd)/riscv64-unknown-haiku
output=$2
case "${PIONEER_Q38_STREAM_IMAGE:-0}" in
 0) source=blackhole_qwen38_block.cpp; asmflag=;;
 1) source=blackhole_qwen38_stream.cpp; asmflag='--defsym TT_Q38_STREAM=1';;
 2) source=blackhole_qwen38_rows.cpp; asmflag='--defsym TT_Q38_ROWS=1';;
 3) source=blackhole_qwen38_shared_rows.cpp; asmflag='--defsym TT_Q38_SHARED_ROWS=1';;
 4) source=blackhole_qwen38_vector_rows.cpp; asmflag='--defsym TT_Q38_VECTOR_ROWS=1';;
 *) exit 2;;
esac
case "${PIONEER_Q38_STREAM_FAST:-0}" in
 0) cppflag=;;
 1) [ "${PIONEER_Q38_STREAM_IMAGE:-0}" != 0 ] || exit 2; cppflag=-DPIONEER_Q38_FAST_PANEL;;
 *) exit 2;;
esac
case "${PIONEER_Q38_FAST_SCALE:-0}" in
 0) ;;
 1) [ "${PIONEER_Q38_STREAM_FAST:-0}" = 1 ] || exit 2; cppflag="$cppflag -DPIONEER_Q38_FAST_SCALE";;
 *) exit 2;;
esac
case "$output" in /*) ;; *) exit 2;; esac
[ ! -e "$output" ] && [ ! -L "$output" ] || exit 1
umask 077; mkdir -m 700 "$output"
"$cross-g++" -std=c++11 -O2 -Wall -Wextra -Werror -ffreestanding -fno-exceptions -fno-rtti \
 -fno-stack-protector -fno-pic -fno-pie -ffp-contract=off -ffunction-sections -fdata-sections \
 -fno-unwind-tables -fno-asynchronous-unwind-tables -msmall-data-limit=0 -mno-relax \
 -march=rv32imf -mabi=ilp32 -fstack-usage $cppflag -c "$directory/$source" -o "$output/program.o"
"$cross-as" -march=rv32imf -mabi=ilp32 -mno-relax $asmflag "$directory/blackhole_qwen38_block_start.S" -o "$output/start.o"
"$cross-ld" -m elf32lriscv --no-relax --gc-sections -T "$directory/blackhole_qwen38_block.ld" \
 "$output/start.o" "$output/program.o" -Map "$output/program.map" -o "$output/program.elf"
for section in boot text; do "$cross-objcopy" -O binary -j ".$section" "$output/program.elf" "$output/$section.bin"; done
"$cross-nm" -n "$output/program.elf" > "$output/program.symbols"
"$cross-objdump" -d "$output/program.elf" > "$output/program.disassembly"
shasum -a 256 "$output/boot.bin" "$output/text.bin"
printf 'OFFLINE complete local GGUF block candidate,NOT fullK/resident/model or native: %s\n' "$output"
