#!/bin/sh
# Offline images ONLY: never builds/installs a driver or accesses a device.
set -eu
[ "$#" = 2 ] || { echo 'Usage: build_coordinator_images.sh CONFIGURED_RISCV64_BUILD FRESH_ABSOLUTE_OUTPUT' >&2; exit 2; }
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
cross=$(CDPATH= cd -- "$1/cross-tools-riscv64/bin" && pwd)/riscv64-unknown-haiku
output=$2
case ${PIONEER_COORDINATOR_PACKET_IDLE:-0} in 0|1) ;; *) echo 'Packet-idle image flag must be0 or1' >&2; exit 2;; esac
case ${PIONEER_COORDINATOR_OUTPUT_COMPARE:-0} in 0|1) ;; *) echo 'Output-compare image flag must be0 or1' >&2; exit 2;; esac
case ${PIONEER_COORDINATOR_FUSED_OUTPUT:-0} in 0|1) ;; *) echo 'Fused-output image flag must be0 or1' >&2; exit 2;; esac
case ${PIONEER_COORDINATOR_COMPACT_INPUT:-0} in 0|1) ;; *) echo 'Compact-input image flag must be0 or1' >&2; exit 2;; esac
case ${PIONEER_COORDINATOR_LAYER:-0} in 0|1) ;; *) echo 'Layer-image flag must be0 or1' >&2; exit 2;; esac
case ${PIONEER_COORDINATOR_WIDE_OUTPUT:-0} in 0|1) ;; *) echo 'Wide-output flag must be0 or1' >&2; exit 2;; esac
case ${PIONEER_COORDINATOR_SPAN_UPLOAD:-0} in 0|1) ;; *) echo 'Span-upload flag must be0 or1' >&2; exit 2;; esac
case ${PIONEER_COORDINATOR_DECODER:-0} in 0|1) ;; *) echo 'Decoder flag must be0 or1' >&2; exit 2;; esac
case ${PIONEER_COORDINATOR_ATTENTION_WORKER:-0} in 0|1) ;; *) exit 2;; esac
[ "${PIONEER_COORDINATOR_ATTENTION_WORKER:-0}" = 0 ] || [ "${PIONEER_COORDINATOR_DECODER:-0}" = 1 ] || exit 2
case ${PIONEER_COORDINATOR_PARALLEL_ATTENTION:-0} in 0|1) ;; *) exit 2;; esac
[ "${PIONEER_COORDINATOR_PARALLEL_ATTENTION:-0}" = 0 ] || [ "${PIONEER_COORDINATOR_ATTENTION_WORKER:-0}" = 1 ] || exit 2
case ${PIONEER_COORDINATOR_STAGE_PROFILE:-0} in 0|1) ;; *) exit 2;; esac
[ "${PIONEER_COORDINATOR_STAGE_PROFILE:-0}" = 0 ] || [ "${PIONEER_COORDINATOR_PARALLEL_ATTENTION:-0}" = 1 ] || exit 2
case ${PIONEER_GRAPH_ROPE_FAST_FMA:-0} in 0|1) ;; *) exit 2;; esac
[ "${PIONEER_GRAPH_ROPE_FAST_FMA:-0}" = 0 ] || [ "${PIONEER_COORDINATOR_DECODER:-0}" = 1 ] || exit 2
case ${PIONEER_GRAPH_FAST_DIVIDE:-0} in 0|1) ;; *) exit 2;; esac
[ "${PIONEER_GRAPH_FAST_DIVIDE:-0}" = 0 ] || [ "${PIONEER_COORDINATOR_DECODER:-0}" = 1 ] || exit 2
case ${PIONEER_COORDINATOR_PARALLEL_GLU:-0} in 0|1) ;; *) exit 2;; esac
[ "${PIONEER_COORDINATOR_PARALLEL_GLU:-0}" = 0 ] || [ "${PIONEER_COORDINATOR_PARALLEL_ATTENTION:-0}" = 1 ] || exit 2
case ${PIONEER_COORDINATOR_GREEDY:-0} in 0|1) ;; *) exit 2;; esac
[ "${PIONEER_COORDINATOR_GREEDY:-0}" = 0 ] || [ "${PIONEER_COORDINATOR_PARALLEL_GLU:-0}" = 1 ] || exit 2
case ${PIONEER_COORDINATOR_UNROLL_TRANSFER:-0} in 0|1) ;; *) exit 2;; esac
[ "${PIONEER_COORDINATOR_UNROLL_TRANSFER:-0}" = 0 ] || [ "${PIONEER_COORDINATOR_GREEDY:-0}" = 1 ] || exit 2
case ${PIONEER_GRAPH_FAST_RMS:-0} in 0|1) ;; *) exit 2;; esac
[ "${PIONEER_GRAPH_FAST_RMS:-0}" = 0 ] || [ "${PIONEER_COORDINATOR_DECODER:-0}" = 1 ] || exit 2
case ${PIONEER_COORDINATOR_CUMULATIVE_COUNTERS:-0} in 0|1) ;; *) exit 2;; esac
[ "${PIONEER_COORDINATOR_CUMULATIVE_COUNTERS:-0}" = 0 ] || [ "${PIONEER_COORDINATOR_UNROLL_TRANSFER:-0}" = 1 ] || exit 2
case ${PIONEER_COORDINATOR_PAIRED_FFN:-0} in 0|1) ;; *) exit 2;; esac
[ "${PIONEER_COORDINATOR_PAIRED_FFN:-0}" = 0 ] || [ "${PIONEER_COORDINATOR_GREEDY:-0}" = 1 ] || exit 2
[ "${PIONEER_COORDINATOR_OUTPUT_COMPARE:-0}" = 0 ] || [ "${PIONEER_COORDINATOR_PACKET_IDLE:-0}" = 1 ] || exit 2
[ "${PIONEER_COORDINATOR_FUSED_OUTPUT:-0}" = 0 ] || { [ "${PIONEER_COORDINATOR_PACKET_IDLE:-0}" = 1 ] && [ "${PIONEER_COORDINATOR_OUTPUT_COMPARE:-0}" = 0 ]; } || exit 2
[ "${PIONEER_COORDINATOR_COMPACT_INPUT:-0}" = 0 ] || [ "${PIONEER_COORDINATOR_FUSED_OUTPUT:-0}" = 1 ] || exit 2
[ "${PIONEER_COORDINATOR_LAYER:-0}" = 0 ] || [ "${PIONEER_COORDINATOR_COMPACT_INPUT:-0}" = 1 ] || exit 2
[ "${PIONEER_COORDINATOR_WIDE_OUTPUT:-0}" = 0 ] || [ "${PIONEER_COORDINATOR_COMPACT_INPUT:-0}" = 1 ] || exit 2
[ "${PIONEER_COORDINATOR_SPAN_UPLOAD:-0}" = 0 ] || { [ "${PIONEER_COORDINATOR_WIDE_OUTPUT:-0}" = 1 ] && [ "${PIONEER_COORDINATOR_LAYER:-0}" = 1 ]; } || exit 2
[ "${PIONEER_COORDINATOR_DECODER:-0}" = 0 ] || { [ "${PIONEER_COORDINATOR_WIDE_OUTPUT:-0}" = 1 ] && [ "${PIONEER_COORDINATOR_LAYER:-0}" = 1 ]; } || exit 2
case "$output" in /*) ;; *) echo 'Absolute output required' >&2; exit 2;; esac
[ ! -e "$output" ] && [ ! -L "$output" ] || { echo 'Fresh output required' >&2; exit 2; }
umask 077
mkdir -m 700 "$output"
set --
if [ "${PIONEER_COORDINATOR_ATTENTION_WORKER:-0}" = 1 ]; then set -- --defsym TT_FLEET_ATTENTION=1; fi
if [ "${PIONEER_COORDINATOR_PARALLEL_GLU:-0}" = 1 ]; then set -- "$@" --defsym TT_FLEET_GLU=1; fi
"$cross-as" -march=rv32i -mabi=ilp32 -mno-relax --defsym TT_FLEET_PERSISTENT=1 --defsym TT_FLEET_COORDINATED=1 "$@" \
 "$directory/blackhole_fleet_start.S" -o "$output/worker.o"
"$cross-ld" -m elf32lriscv --no-relax -T "$directory/blackhole_fleet_start.ld" "$output/worker.o" -o "$output/worker.elf"
"$cross-objcopy" -O binary -j .text "$output/worker.elf" "$output/worker.bin"
"$cross-nm" -n "$output/worker.elf" > "$output/worker.symbols"
"$cross-objdump" -d "$output/worker.elf" > "$output/worker.disassembly"
source_file=blackhole_coordinator.cpp
link_file=blackhole_coordinator.ld
isa=rv32im
set --
if [ "${PIONEER_COORDINATOR_LAYER:-0}" = 1 ]; then
 source_file=blackhole_layer_coordinator.cpp
 link_file=blackhole_layer_coordinator.ld
 isa=rv32imf
 set -- -ffp-contract=off
fi
if [ "${PIONEER_COORDINATOR_DECODER:-0}" = 1 ]; then source_file=blackhole_decoder.cpp; fi
"$cross-g++" -std=c++11 -Os -Wall -Wextra -Werror -ffreestanding -fno-exceptions -fno-rtti "$@" \
 -fno-stack-protector -fno-pic -fno-pie -ffunction-sections -fdata-sections -fno-unwind-tables \
 -fno-asynchronous-unwind-tables -msmall-data-limit=0 -mno-relax -march="$isa" -mabi=ilp32 \
 -fstack-usage -DPIONEER_COORDINATOR_PACKET_IDLE=${PIONEER_COORDINATOR_PACKET_IDLE:-0} \
 -DPIONEER_COORDINATOR_OUTPUT_COMPARE=${PIONEER_COORDINATOR_OUTPUT_COMPARE:-0} \
 -DPIONEER_COORDINATOR_FUSED_OUTPUT=${PIONEER_COORDINATOR_FUSED_OUTPUT:-0} \
 -DPIONEER_COORDINATOR_COMPACT_INPUT=${PIONEER_COORDINATOR_COMPACT_INPUT:-0} \
 -DPIONEER_COORDINATOR_TRUSTED_IDLE=${PIONEER_COORDINATOR_LAYER:-0} \
 -DPIONEER_COORDINATOR_WIDE_OUTPUT=${PIONEER_COORDINATOR_WIDE_OUTPUT:-0} \
 -DPIONEER_COORDINATOR_SPAN_UPLOAD=${PIONEER_COORDINATOR_SPAN_UPLOAD:-0} \
 -DPIONEER_COORDINATOR_PARALLEL_ATTENTION=${PIONEER_COORDINATOR_PARALLEL_ATTENTION:-0} \
 -DPIONEER_COORDINATOR_STAGE_PROFILE=${PIONEER_COORDINATOR_STAGE_PROFILE:-0} \
 -DPIONEER_GRAPH_ROPE_FAST_FMA=${PIONEER_GRAPH_ROPE_FAST_FMA:-0} \
 -DPIONEER_GRAPH_FAST_DIVIDE=${PIONEER_GRAPH_FAST_DIVIDE:-0} \
 -DPIONEER_COORDINATOR_PARALLEL_GLU=${PIONEER_COORDINATOR_PARALLEL_GLU:-0} \
 -DPIONEER_COORDINATOR_GREEDY=${PIONEER_COORDINATOR_GREEDY:-0} \
 -DPIONEER_COORDINATOR_UNROLL_TRANSFER=${PIONEER_COORDINATOR_UNROLL_TRANSFER:-0} \
 -DPIONEER_GRAPH_FAST_RMS=${PIONEER_GRAPH_FAST_RMS:-0} \
 -DPIONEER_COORDINATOR_CUMULATIVE_COUNTERS=${PIONEER_COORDINATOR_CUMULATIVE_COUNTERS:-0} \
 -DPIONEER_COORDINATOR_PAIRED_FFN=${PIONEER_COORDINATOR_PAIRED_FFN:-0} \
 -c "$directory/$source_file" -o "$output/program.o"
"$cross-as" -march="$isa" -mabi=ilp32 -mno-relax "$directory/blackhole_coordinator_start.S" -o "$output/start.o"
set --
if [ "${PIONEER_COORDINATOR_DECODER:-0}" = 1 ]; then
 "$cross-as" -march="$isa" -mabi=ilp32 -mno-relax --defsym TT_ATTENTION_DATA_ONLY=1 "$directory/blackhole_attention_start.S" -o "$output/attention-exact.o"
 set -- "$output/attention-exact.o"
fi
"$cross-ld" -m elf32lriscv --no-relax --gc-sections -T "$directory/$link_file" \
 "$output/start.o" "$output/program.o" "$@" -Map "$output/program.map" -o "$output/program.elf"
"$cross-objcopy" -O binary -j .text "$output/program.elf" "$output/code.bin"
"$cross-objcopy" -O binary -j .boot "$output/program.elf" "$output/boot.bin"
"$cross-nm" -n "$output/program.elf" > "$output/program.symbols"
"$cross-objdump" -d "$output/program.elf" > "$output/program.disassembly"
shasum -a 256 "$output/worker.bin" "$output/code.bin" "$output/boot.bin"
printf 'OFFLINE coordinator/worker candidates ONLY, no driver exposure: %s\n' "$output"
