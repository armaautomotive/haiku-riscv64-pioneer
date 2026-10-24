#!/bin/sh
# OFFLINE QOP1 parent only. Does not build/install/select a native driver.
set -eu
python3 -c 'import os,sys;sys.exit(0 if all(not k.startswith("PIONEER_") or v=="0" for k,v in os.environ.items()) else 2)' || exit 2
[ "$#" = 2 ] || exit 2
here=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
cross=$(CDPATH= cd -- "$1/cross-tools-riscv64/bin" && pwd)/riscv64-unknown-haiku
output=$2
case "$output" in /*) ;; *) exit 2;; esac
[ ! -e "$output" ] && [ ! -L "$output" ] || exit 2
umask 077
mkdir -m 700 "$output"
flags='-std=c++11 -O2 -Wall -Wextra -Werror -ffreestanding -fno-exceptions -fno-rtti
 -fno-stack-protector -fno-pic -fno-pie -ffp-contract=off -ffunction-sections -fdata-sections
 -fno-unwind-tables -fno-asynchronous-unwind-tables -msmall-data-limit=0 -mno-relax
 -march=rv32imf -mabi=ilp32 -fstack-usage -fomit-frame-pointer'
"$cross-g++" $flags -DPIONEER_Q38_GDN_OUTPUT_GRAPH_ROOT=1 -DPIONEER_Q38_PROJECTION_GRAPH_ROOT=1 -DPIONEER_Q38_PROJECTION_MATRIX_ROOT=1 \
 -DPIONEER_Q38_FFN_RESIDENT_MATRIX -DPIONEER_Q38_COMPACT_ACTIVATIONS -DPIONEER_Q38_REUSE_ACTIVATIONS \
 -DPIONEER_Q38_WORK_COHORT_CACHE -DPIONEER_Q38_WORK_SKIP_DOT_CHECK -DPIONEER_Q38_WORK_SPARSE_EXPORT \
 -DPIONEER_Q38_WORK_COMPACT_OUTPUT -DPIONEER_Q38_ROOT_CYCLE_PROFILE -DPIONEER_Q38_WORK_SENTINEL_POLL \
 -c "$here/blackhole_qwen38_gdn_output_graph.cpp" -o "$output/program.o"
"$cross-g++" $flags -Dscalar_main=q38_projection_gather_once -Dscalar_fault_park=block_fault_park \
 -c "$here/blackhole_qwen38_gdn_output_gather.cpp" -o "$output/gather.o"
# One pre-existing whole-matrix frame is unchanged at4976B. Every other
# function retains the2048B gate;whole call stack must ALSO pass emulator.
awk -F '\t' 'NF==3 {if ($1 ~ /unsigned int q38_projection_matrix_once\(\)$/) {if ($2!=4976) bad=1; legacy++} else if ($2>2048) {print "STOP oversized new frame: " $0;bad=1}} END {exit bad || legacy!=1}' "$output/program.su" "$output/gather.su"
"$cross-as" -march=rv32imf -mabi=ilp32 -mno-relax --defsym TT_Q38_DISPATCH_ROOT=1 \
 --defsym TT_Q38_PROJECTION_GRAPH_ROOT=1 --defsym TT_Q38_GDN_OUTPUT_GRAPH_ROOT=1 \
 "$here/blackhole_qwen38_block_start.S" -o "$output/start.o"
"$cross-ld" -m elf32lriscv --no-relax --gc-sections -T "$here/blackhole_qwen38_projection_graph.ld" \
 "$output/start.o" "$output/program.o" "$output/gather.o" -Map "$output/program.map" -o "$output/program.elf"
for section in boot text prefix graph;do "$cross-objcopy" -O binary -j ".$section" "$output/program.elf" "$output/$section.bin";done
"$cross-nm" -n "$output/program.elf" > "$output/program.symbols"
"$cross-objdump" -d "$output/program.elf" > "$output/program.disassembly"
for section in boot text prefix graph;do shasum -a 256 "$output/$section.bin";done
printf 'OFFLINE distinct QOP1 complete ssm_out image ONLY;NO native or speed grant:%s\n' "$output"
