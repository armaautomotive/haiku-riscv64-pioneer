#!/bin/sh
# OFFLINE private persistent root only;NO driver install/card operations.
set -eu
[ "${PIONEER_Q38_WORK_TILE_PREFETCH:-0}" = 0 ] || { echo 'STOP one-tile prefetch has NO coordinator/controller qualification' >&2;exit 2; }
[ "${PIONEER_Q38_TILE_PREFETCH_QUALIFICATION:-0}" = 0 ] || exit 2
[ "${PIONEER_Q38_WORK_RAW_UNPACK_CACHE:-0}" = 0 ] || { echo 'STOP raw-Dst reuse changes peers ONLY;no coordinator/controller qualification exists' >&2;exit 2; }
[ "${PIONEER_Q38_RAW_UNPACK_CACHE_QUALIFICATION:-0}" = 0 ] || exit 2
[ "${PIONEER_Q38_WORK_FRONTEND_BARRIER:-0}" = 0 ] || exit 2
[ "${PIONEER_Q38_WORK_BATCH_RAW_INPUT:-0}" = 0 ] || exit 2
[ "${PIONEER_Q38_BATCH_RAW_QUALIFICATION:-0}" = 0 ] || exit 2
[ "$#" = 2 ] || exit 2
case "${PIONEER_Q38_WORK_ROOT_COMMAND_REUSE:-0}" in
 0) command_cpp=;;
 1) [ "${PIONEER_Q38_WORK_SENTINEL_POLL:-0}:${PIONEER_Q38_WORK_BATCH4_FETCH:-0}" = 1:0 ] || exit 2
    command_cpp=-DPIONEER_Q38_WORK_ROOT_COMMAND_REUSE;;
 *) exit 2;;
esac
case "${PIONEER_Q38_WORK_BATCH4_FETCH:-0}" in
 0) batch_cpp=;;
 1) [ "${PIONEER_Q38_WORK_SENTINEL_POLL:-0}" = 1 ] || exit 2
    batch_cpp=-DPIONEER_Q38_WORK_BATCH4_FETCH;;
 *) exit 2;;
esac
case "${PIONEER_Q38_WORK_SENTINEL_POLL:-0}" in
 0) sentinel_cpp=;;
 1) [ "${PIONEER_Q38_ROOT_CYCLE_PROFILE:-0}:${PIONEER_Q38_WORK_COHORT_CACHE:-0}:${PIONEER_Q38_WORK_COMPACT_OUTPUT:-0}:${PIONEER_Q38_FAULT_RETURN:-0}:${PIONEER_Q38_TRANSFER_DIAGNOSTIC:-0}:${PIONEER_Q38_PEER_PROGRESS:-0}" = 1:1:1:0:0:0 ] || exit 2
    sentinel_cpp=-DPIONEER_Q38_WORK_SENTINEL_POLL;;
 *) exit 2;;
esac
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
cross=$(CDPATH= cd -- "$1/cross-tools-riscv64/bin" && pwd)/riscv64-unknown-haiku
output=$2
case "${PIONEER_Q38_ROOT_CYCLE_PROFILE:-0}" in
 0) ;;
 1) [ "${PIONEER_Q38_WORK_COMPACT_OUTPUT:-0}:${PIONEER_Q38_FAULT_RETURN:-0}:${PIONEER_Q38_TRANSFER_DIAGNOSTIC:-0}:${PIONEER_Q38_PEER_PROGRESS:-0}" = 1:0:0:0 ] || exit 2;;
 *) exit 2;;
esac
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
case "${PIONEER_Q38_WORK_COHORT_CACHE:-0}" in
 0) ;;
 1) [ "${PIONEER_Q38_REUSE_ACTIVATIONS:-0}" = 1 ] || exit 2
    fault_cpp="$fault_cpp -DPIONEER_Q38_WORK_COHORT_CACHE";;
 *) exit 2;;
esac
case "${PIONEER_Q38_WORK_SKIP_DOT_CHECK:-0}" in
 0) ;;
 1) [ "${PIONEER_Q38_WORK_COHORT_CACHE:-0}" = 1 ] || exit 2
    fault_cpp="$fault_cpp -DPIONEER_Q38_WORK_SKIP_DOT_CHECK";;
 *) exit 2;;
esac
case "${PIONEER_Q38_OUTPUT_TRANSFER_ROOT:-0}" in
 0) ;;
 1) [ "${PIONEER_Q38_WORK_SKIP_DOT_CHECK:-0}" = 1 ] || exit 2
    fault_cpp="$fault_cpp -DPIONEER_Q38_OUTPUT_TRANSFER_ROOT";;
 *) exit 2;;
esac
case "${PIONEER_Q38_WORK_SPARSE_EXPORT:-0}" in
 0) ;;
 1) [ "${PIONEER_Q38_OUTPUT_TRANSFER_ROOT:-0}" = 1 ] || exit 2
    fault_cpp="$fault_cpp -DPIONEER_Q38_WORK_SPARSE_EXPORT";;
 *) exit 2;;
esac
case "${PIONEER_Q38_WORK_COMPACT_OUTPUT:-0}" in
 0) ;;
 1) [ "${PIONEER_Q38_WORK_SPARSE_EXPORT:-0}" = 1 ] || exit 2
    fault_cpp="$fault_cpp -DPIONEER_Q38_WORK_COMPACT_OUTPUT";;
 *) exit 2;;
esac
if [ "${PIONEER_Q38_ROOT_CYCLE_PROFILE:-0}" = 1 ]; then fault_cpp="$fault_cpp -DPIONEER_Q38_ROOT_CYCLE_PROFILE"; fi
fault_cpp="$fault_cpp $sentinel_cpp $batch_cpp $command_cpp"
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
linker=blackhole_qwen38_block.ld; sections='boot text'
if [ "${PIONEER_Q38_OUTPUT_TRANSFER_ROOT:-0}" = 1 ]; then linker=blackhole_qwen38_output_root.ld; sections='boot text output'; fi
"$cross-ld" -m elf32lriscv --no-relax --gc-sections -T "$directory/$linker" \
 "$output/start.o" "$output/program.o" -Map "$output/program.map" -o "$output/program.elf"
for section in $sections; do "$cross-objcopy" -O binary -j ".$section" "$output/program.elf" "$output/$section.bin"; done
"$cross-nm" -n "$output/program.elf" > "$output/program.symbols"
"$cross-objdump" -d "$output/program.elf" > "$output/program.disassembly"
for section in $sections; do shasum -a 256 "$output/$section.bin"; done
printf 'OFFLINE dispatcher only;NO driver/capability/native/graph/inference qualification: %s\n' "$output"
