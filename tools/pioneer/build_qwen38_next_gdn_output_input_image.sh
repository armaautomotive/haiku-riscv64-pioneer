#!/bin/sh
# Separate OFFLINE QOI2 persistent peer; no driver/card access.
set -eu
python3 -c 'import os,sys;sys.exit(0 if all(not k.startswith("PIONEER_") or v=="0" for k,v in os.environ.items()) else 2)' || exit 2
[ "$#" = 2 ] || exit 2
projection_flags='-DPIONEER_Q38_PROJECTION_PERSISTENT=1 -DPIONEER_Q38_NEXT_GDN_OUTPUT_INPUT_IMAGE=1'
activation_source=blackhole_qwen38_next_gdn_output_input.cpp
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
cross=$(CDPATH= cd -- "$1/cross-tools-riscv64/bin" && pwd)/riscv64-unknown-haiku
output=$2
case "$output" in /*) ;; *) exit 2;; esac
[ ! -e "$output" ] && [ ! -L "$output" ] || exit 2
umask 077
mkdir -m 700 "$output"
flags='-std=c++11 -O2 -Wall -Wextra -Werror -ffreestanding -fno-exceptions -fno-rtti
 -fno-stack-protector -fno-pic -fno-pie -ffp-contract=off -ffunction-sections -fdata-sections
 -fno-unwind-tables -fno-asynchronous-unwind-tables -msmall-data-limit=0 -mno-relax
 -march=rv32imf -mabi=ilp32 -fstack-usage'
peer='-DPIONEER_Q38_FFN_PERSISTENT -DPIONEER_Q38_FAST_PANEL -DPIONEER_Q38_FAST_SCALE
 -DPIONEER_Q38_Q4_TWO_PHASE -DPIONEER_Q38_PIPELINED_MATRIX -DPIONEER_Q38_STATIC_VECTOR
 -DPIONEER_Q38_CACHED_AFFINE -DPIONEER_Q38_REPLAY_OPERAND -DPIONEER_Q38_WORK_SKIP_DOT_CHECK
 -DPIONEER_Q38_QUEUED_MATRIX_ENTRY -DPIONEER_Q38_WORK_INPUT_SUM_CACHE -DPIONEER_Q38_WORK_ZERO_MINIMUM
 -DPIONEER_Q38_NATIVE_OPERAND_STORE -DPIONEER_Q38_WORK_DEFER_AFFINE -DPIONEER_Q38_INLINE_CONFIG
 -DPIONEER_Q38_BLOCK_AFFINE_DST -DPIONEER_Q38_BLOCK_SCALE_REUSE -DPIONEER_Q38_WORK_AFFINE_REPLAY
 -DPIONEER_Q38_WORK_DIRECT_SRCB -DPIONEER_Q38_WORK_DOT_ZERO_ELISION'
"$cross-g++" $flags $peer $projection_flags -c "$directory/blackhole_qwen38_next_gdn_output_peer.cpp" -o "$output/program.o"
"$cross-g++" $flags $projection_flags -c "$directory/$activation_source" -o "$output/activation.o"
awk -F '\t' 'NF==3 && $2>2048 {print "STOP oversized new peer frame: " $0;bad=1} END {exit bad}' "$output/program.su" "$output/activation.su"
"$cross-as" -march=rv32imf -mabi=ilp32 -mno-relax --defsym TT_Q38_FFN_PERSISTENT_PEER=1 \
 "$directory/blackhole_qwen38_persistent_start.S" -o "$output/start.o"
"$cross-ld" -m elf32lriscv --no-relax --gc-sections -T "$directory/blackhole_qwen38_ffn_persistent.ld" \
 "$output/start.o" "$output/program.o" "$output/activation.o" -Map "$output/program.map" -o "$output/program.elf"
for section in boot text affine ffn;do "$cross-objcopy" -O binary -j ".$section" "$output/program.elf" "$output/$section.bin";done
"$cross-nm" -n "$output/program.elf" > "$output/program.symbols"
"$cross-objdump" -d "$output/program.elf" > "$output/program.disassembly"
for section in boot text affine ffn;do shasum -a 256 "$output/$section.bin";done
printf 'OFFLINE QOI2 full peer+Q8_0/6144 matmul ONLY;no native owner or speed grant:%s\n' "$output"
