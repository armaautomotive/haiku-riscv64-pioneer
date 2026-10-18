#!/bin/sh
# OFFLINE explicit mixed-quant16x16 matrix probe. No install/device access.
set -eu
[ "$#" = 2 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
cross=$(CDPATH= cd -- "$1/cross-tools-riscv64/bin" && pwd)/riscv64-unknown-haiku
output=$2
case "${PIONEER_Q38_PANEL_AFFINE_IMAGE:-0}" in
 0) cppflag=; asmflag=; architecture=rv32im;;
 1) cppflag=-DPIONEER_Q38_PANEL_AFFINE; asmflag='--defsym TT_Q38_LOCAL_AFFINE=1'; architecture=rv32imf;;
 *) exit 2;;
esac
case "$output" in /*) ;; *) exit 2;; esac
[ ! -e "$output" ] && [ ! -L "$output" ] || exit 1
umask 077
mkdir -m 700 "$output"
"$cross-g++" -std=c++11 -O2 -Wall -Wextra -Werror -ffreestanding -fno-exceptions -fno-rtti \
 -fno-stack-protector -fno-pic -fno-pie -ffunction-sections -fdata-sections \
 -fno-unwind-tables -fno-asynchronous-unwind-tables -msmall-data-limit=0 -mno-relax \
 -ffp-contract=off -march="$architecture" -mabi=ilp32 -fstack-usage $cppflag -c "$directory/blackhole_qwen38_panel.cpp" -o "$output/unpack.o"
"$cross-as" -march="$architecture" -mabi=ilp32 -mno-relax $asmflag \
 --defsym TT_UNPACK_NONUNIFORM=1 --defsym TT_UNPACK_FULL_BF16=1 --defsym TT_UNPACK_RESIDENT=1 \
 --defsym TT_UNPACK_GEMV=1 --defsym TT_RUNTIME_MATVEC=1 --defsym TT_RUNTIME_INT8=1 \
 --defsym TT_Q38_LOCAL_PANEL16=1 "$directory/blackhole_tensix_unpack.S" -o "$output/matrix.o"
(cd "$output" && "$cross-ld" -m elf32lriscv --no-relax --gc-sections -T "$directory/blackhole_qwen38_panel.ld" \
 matrix.o unpack.o -Map program.map -o program.elf)
for section in boot text bridge unpack; do
 "$cross-objcopy" -O binary -j ".$section" "$output/program.elf" "$output/$section.bin"
done
"$cross-nm" -n "$output/program.elf" > "$output/program.symbols"
"$cross-objdump" -d "$output/program.elf" > "$output/program.disassembly"
shasum -a 256 "$output/boot.bin" "$output/text.bin" "$output/bridge.bin" "$output/unpack.bin"
printf 'OFFLINE mixed-quant packed-panel integer Tensix probe ONLY,not model execution: %s\n' "$output"
