#!/bin/sh
# Separate OFFLINE NEW ABI/image ONLY;NO driver/card/native selection.
set -eu
[ "${PIONEER_Q38_FFN_GRAPH_RESIDUAL_ROOT-0}" = 0 ] || exit 2
[ "${PIONEER_Q38_RESIDUAL_RMS_NATIVE-0}" = 0 ] || exit 2
[ "${PIONEER_Q38_CONV_NATIVE-0}:${PIONEER_Q38_CONV_CLIENT-0}:${PIONEER_Q38_CONV_IMAGE-0}:${PIONEER_Q38_CONV_BATCH_IMAGE-0}" = 0:0:0:0 ] || exit 2
[ "${PIONEER_Q38_DECODER_RUNTIME-0}" = 0 ] || exit 2
[ "${PIONEER_Q38_RESIDENT_DECODER_OWNER-0}" = 0 ] || exit 2
[ "${PIONEER_Q38_RESIDENT_DECODER_NATIVE-0}" = 0 ] || exit 2
[ "${PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE-0}" = 0 ] || exit 2
[ "${PIONEER_Q38_GDN_INPUT_OWNER-0}" = 0 ] || exit 2
[ "${PIONEER_Q38_GDN_STATE_INIT_NATIVE-0}" = 0 ] || exit 2
[ "${PIONEER_Q38_GDN_RECURRENT_NATIVE-0}" = 0 ] || exit 2
[ "${PIONEER_Q38_GDN_RECURRENT_VALIDATION_NATIVE-0}" = 0 ] || exit 2
[ "${PIONEER_Q38_GDN_GATED_NATIVE-0}" = 0 ] || exit 2
[ "${PIONEER_Q38_GDN_RENDEZVOUS_NATIVE-0}" = 0 ] || exit 2
[ "${PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE-0}" = 0 ] || exit 2
[ "${PIONEER_Q38_GDN_OUTPUT_EXECUTION_NATIVE-0}" = 0 ] || exit 2
[ "${PIONEER_Q38_GDN_OUTPUT_PRODUCER_NATIVE-0}" = 0 ] || exit 2
[ "${PIONEER_Q38_GDN_OUTPUT_INPUT_IMAGE-0}" = 0 ] || exit 2
[ "${PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE-0}" = 0 ] || exit 2
[ "${PIONEER_Q38_PROJECTION_VALIDATION_NATIVE-0}:${PIONEER_Q38_PROJECTION_VALIDATION_CLIENT-0}" = 0:0 ] && [ "${PIONEER_DRIVER_BUILD_TAG-}" != 2026102200010042 ] || exit 2
[ "${PIONEER_Q38_PROJECTION_RESULT_STASH-0}" = 0 ] || exit 2
[ "${PIONEER_Q38_LEARNED_RMS_VALIDATION_NATIVE-0}:${PIONEER_Q38_LEARNED_RMS_VALIDATION_CLIENT-0}" = 0:0 ] && [ "${PIONEER_DRIVER_BUILD_TAG-}" != 2026102200010041 ] || exit 2
[ "${PIONEER_Q38_EMBEDDING_QUALIFICATION-0}:${PIONEER_Q38_EMBEDDING_CLIENT-0}" = 0:0 ] || exit 2
[ "${PIONEER_Q38_LEARNED_RMS-0}:${PIONEER_Q38_LEARNED_RMS_NATIVE-0}" = 0:0 ] || exit 2
[ "${PIONEER_Q38_PROJECTION_EXECUTION_OWNER-0}" = 0 ] || exit 2
[ "${PIONEER_Q38_EMBEDDING_VALIDATION_NATIVE-0}:${PIONEER_Q38_EMBEDDING_VALIDATION_CLIENT-0}" = 0:0 ] || exit 2
[ "${PIONEER_Q38_EMBEDDING_NATIVE-0}" = 0 ] || exit 2
[ "${PIONEER_Q38_PROJECTION_WORKSPACE:-0}" = 0 ] || exit 2
[ "${PIONEER_Q38_PROJECTION_GRAPH_MEMORY:-0}" = 0 ] || exit 2
[ "${PIONEER_Q38_PROJECTION_GRAPH_ROOT:-0}" = 0 ] || exit 2
[ "${PIONEER_Q38_PROJECTION_MATRIX_ROOT:-0}" = 0 ] || exit 2
[ "${PIONEER_Q38_FFN_GRAPH_INPUT_ROOT:-0}" = 0 ] || exit 2
[ "${PIONEER_Q38_FFN_GRAPH_ROOT:-0}" = 0 ] || exit 2
[ "${PIONEER_Q38_FFN_PERSISTENT:-0}" = 0 ] || exit 2
[ "${PIONEER_Q38_FFN_RESIDENT_MATRIX:-0}" = 1 ] || exit 2
for key in PIONEER_Q38_OUTPUT_TRANSFER_ROOT PIONEER_Q38_WORK_ROOT_COMMAND_REUSE \
 PIONEER_Q38_WORK_BATCH4_FETCH PIONEER_Q38_PEER_PHASE_ROOT PIONEER_Q38_WORK_RAW_UNPACK_CACHE \
 PIONEER_Q38_RAW_UNPACK_CACHE_QUALIFICATION PIONEER_Q38_WORK_LOCAL_Q4 PIONEER_Q38_LOCAL_Q4_QUALIFICATION; do
 value=$(printenv "$key" || :)
 [ "${value:-0}" = 0 ] || { printf 'STOP incompatible offline FFN matrix selector %s\n' "$key" >&2;exit 2; }
done
[ "$#" = 2 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
cross=$(CDPATH= cd -- "$1/cross-tools-riscv64/bin" && pwd)/riscv64-unknown-haiku
output=$2
case "$output" in /*) ;; *) exit 2;; esac
[ ! -e "$output" ] && [ ! -L "$output" ] || exit 2
umask 077
mkdir -m 700 "$output"
"$cross-g++" -std=c++11 -O2 -Wall -Wextra -Werror -ffreestanding -fno-exceptions -fno-rtti \
 -fno-stack-protector -fno-pic -fno-pie -ffp-contract=off -ffunction-sections -fdata-sections \
 -fno-unwind-tables -fno-asynchronous-unwind-tables -msmall-data-limit=0 -mno-relax \
 -march=rv32imf -mabi=ilp32 -fstack-usage -fomit-frame-pointer \
 -DPIONEER_Q38_FFN_RESIDENT_MATRIX -DPIONEER_Q38_COMPACT_ACTIVATIONS -DPIONEER_Q38_REUSE_ACTIVATIONS \
 -DPIONEER_Q38_WORK_COHORT_CACHE -DPIONEER_Q38_WORK_SKIP_DOT_CHECK -DPIONEER_Q38_WORK_SPARSE_EXPORT \
 -DPIONEER_Q38_WORK_COMPACT_OUTPUT -DPIONEER_Q38_ROOT_CYCLE_PROFILE -DPIONEER_Q38_WORK_SENTINEL_POLL \
 -c "$directory/blackhole_qwen38_dispatch.cpp" -o "$output/program.o"
"$cross-as" -march=rv32imf -mabi=ilp32 -mno-relax --defsym TT_Q38_DISPATCH_ROOT=1 --defsym TT_Q38_FFN_MATRIX_ROOT=1 \
 "$directory/blackhole_qwen38_block_start.S" -o "$output/start.o"
"$cross-ld" -m elf32lriscv --no-relax --gc-sections -T "$directory/blackhole_qwen38_ffn_matrix.ld" \
 "$output/start.o" "$output/program.o" -Map "$output/program.map" -o "$output/program.elf"
for section in boot text prefix;do "$cross-objcopy" -O binary -j ".$section" "$output/program.elf" "$output/$section.bin";done
"$cross-nm" -n "$output/program.elf" > "$output/program.symbols"
"$cross-objdump" -d "$output/program.elf" > "$output/program.disassembly"
shasum -a 256 "$output/boot.bin" "$output/text.bin" "$output/prefix.bin"
printf 'OFFLINE NEW QFM1 resident matrix image ONLY;NO native/controller/model qualification: %s\n' "$output"
