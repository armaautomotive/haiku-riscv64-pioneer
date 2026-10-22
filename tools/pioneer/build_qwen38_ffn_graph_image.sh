#!/bin/sh
# NEW OFFLINE complete FFN root ONLY;NO native access/selection/ownership.
set -eu
[ "${PIONEER_Q38_FFN_GRAPH_ROOT:-0}" = 1 ] || exit 2
for key in PIONEER_Q38_FFN_RESIDENT_MATRIX PIONEER_Q38_FFN_PERSISTENT PIONEER_Q38_OUTPUT_TRANSFER_ROOT \
 PIONEER_Q38_WORK_ROOT_COMMAND_REUSE PIONEER_Q38_WORK_BATCH4_FETCH PIONEER_Q38_PEER_PHASE_ROOT \
 PIONEER_Q38_WORK_RAW_UNPACK_CACHE PIONEER_Q38_RAW_UNPACK_CACHE_QUALIFICATION PIONEER_Q38_WORK_LOCAL_Q4 PIONEER_Q38_LOCAL_Q4_QUALIFICATION;do
 value=$(printenv "$key" || :)
 [ "${value:-0}" = 0 ] || { printf 'STOP incompatible complete FFN selector %s\n' "$key" >&2;exit 2; }
done
[ "$#" = 2 ] || exit 2
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
 -march=rv32imf -mabi=ilp32 -fstack-usage -fomit-frame-pointer'
"$cross-g++" $flags -DPIONEER_Q38_FFN_GRAPH_ROOT -DPIONEER_Q38_FFN_RESIDENT_MATRIX \
 -DPIONEER_Q38_COMPACT_ACTIVATIONS -DPIONEER_Q38_REUSE_ACTIVATIONS -DPIONEER_Q38_WORK_COHORT_CACHE \
 -DPIONEER_Q38_WORK_SKIP_DOT_CHECK -DPIONEER_Q38_WORK_SPARSE_EXPORT -DPIONEER_Q38_WORK_COMPACT_OUTPUT \
 -DPIONEER_Q38_ROOT_CYCLE_PROFILE -DPIONEER_Q38_WORK_SENTINEL_POLL \
 -c "$directory/blackhole_qwen38_ffn_graph.cpp" -o "$output/program.o"
"$cross-g++" $flags -Dscalar_main=q38_ffn_graph_gather_once -Dscalar_fault_park=block_fault_park \
 -c "$directory/blackhole_qwen38_ffn_resident_gather2.cpp" -o "$output/gather.o"
"$cross-as" -march=rv32imf -mabi=ilp32 -mno-relax --defsym TT_Q38_DISPATCH_ROOT=1 --defsym TT_Q38_FFN_GRAPH_ROOT=1 \
 "$directory/blackhole_qwen38_block_start.S" -o "$output/start.o"
"$cross-ld" -m elf32lriscv --no-relax --gc-sections -T "$directory/blackhole_qwen38_ffn_graph.ld" \
 "$output/start.o" "$output/program.o" "$output/gather.o" -Map "$output/program.map" -o "$output/program.elf"
for section in boot text prefix graph;do "$cross-objcopy" -O binary -j ".$section" "$output/program.elf" "$output/$section.bin";done
"$cross-nm" -n "$output/program.elf" > "$output/program.symbols"
"$cross-objdump" -d "$output/program.elf" > "$output/program.disassembly"
for section in boot text prefix graph;do shasum -a 256 "$output/$section.bin";done
printf 'OFFLINE NEW QFG1 complete FFN root ONLY;NOT native/controller/model/speed qualified: %s\n' "$output"
