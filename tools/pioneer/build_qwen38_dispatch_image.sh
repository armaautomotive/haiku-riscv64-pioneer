#!/bin/sh
# OFFLINE private persistent root only;NO driver install/card operations.
set -eu
[ "$#" = 2 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
cross=$(CDPATH= cd -- "$1/cross-tools-riscv64/bin" && pwd)/riscv64-unknown-haiku
output=$2
case "${PIONEER_Q38_FAULT_RETURN:-0}" in 0) fault_cpp=;; 1) fault_cpp=-DPIONEER_Q38_FAULT_RETURN;; *) exit 2;; esac
case "${PIONEER_Q38_COMPACT_ACTIVATIONS:-0}" in
 0) ;;
 1) [ "${PIONEER_Q38_FAULT_RETURN:-0}" = 0 ] || exit 2; fault_cpp="$fault_cpp -DPIONEER_Q38_COMPACT_ACTIVATIONS";;
 *) exit 2;;
esac
case "${PIONEER_Q38_REUSE_ACTIVATIONS:-0}" in
 0) ;;
 1) [ "${PIONEER_Q38_COMPACT_ACTIVATIONS:-0}" = 1 ] || exit 2
    fault_cpp="$fault_cpp -DPIONEER_Q38_REUSE_ACTIVATIONS -fomit-frame-pointer";;
 *) exit 2;;
esac
case "${PIONEER_Q38_TRANSFER_DIAGNOSTIC:-0}" in 0) ;; 1) [ "${PIONEER_Q38_FAULT_RETURN:-0}" = 1 ] || exit 2; fault_cpp="$fault_cpp -DPIONEER_Q38_TRANSFER_DIAGNOSTIC";; *) exit 2;; esac
case "${PIONEER_Q38_PEER_PROGRESS:-0}" in 0) ;; 1) [ "${PIONEER_Q38_FAULT_RETURN:-0}:${PIONEER_Q38_TRANSFER_DIAGNOSTIC:-0}" = 1:1 ] || exit 2; fault_cpp="$fault_cpp -DPIONEER_Q38_PEER_PROGRESS";; *) exit 2;; esac
case "$output" in /*) ;; *) exit 2;; esac
[ ! -e "$output" ] && [ ! -L "$output" ] || exit 1
umask 077; mkdir -m 700 "$output"
"$cross-g++" -std=c++11 -O2 -Wall -Wextra -Werror -ffreestanding -fno-exceptions -fno-rtti \
 -fno-stack-protector -fno-pic -fno-pie -ffp-contract=off -ffunction-sections -fdata-sections \
 -fno-unwind-tables -fno-asynchronous-unwind-tables -msmall-data-limit=0 -mno-relax \
 -march=rv32imf -mabi=ilp32 -fstack-usage $fault_cpp -c "$directory/blackhole_qwen38_dispatch.cpp" -o "$output/program.o"
"$cross-as" -march=rv32imf -mabi=ilp32 -mno-relax --defsym TT_Q38_DISPATCH_ROOT=1 \
 "$directory/blackhole_qwen38_block_start.S" -o "$output/start.o"
"$cross-ld" -m elf32lriscv --no-relax --gc-sections -T "$directory/blackhole_qwen38_block.ld" \
 "$output/start.o" "$output/program.o" -Map "$output/program.map" -o "$output/program.elf"
for section in boot text; do "$cross-objcopy" -O binary -j ".$section" "$output/program.elf" "$output/$section.bin"; done
"$cross-nm" -n "$output/program.elf" > "$output/program.symbols"
"$cross-objdump" -d "$output/program.elf" > "$output/program.disassembly"
shasum -a 256 "$output/boot.bin" "$output/text.bin"
printf 'OFFLINE dispatcher only;NO driver/capability/native/graph/inference qualification: %s\n' "$output"
