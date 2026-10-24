#!/bin/sh
# OFFLINE arithmetic/device-state images ONLY; no native build/install/access.
set -eu
[ "${PIONEER_Q38_NEXT_GDN_NATIVE-0}" = 0 ] || exit 2
[ "${PIONEER_Q38_NEXT_CONV_NATIVE-0}" = 0 ] || exit 2
[ "${PIONEER_Q38_NEXT_PROJECTION_NATIVE-0}" = 0 ] || exit 2
[ "${PIONEER_Q38_PROJECTION_NEXT_ROOT+x}" != x ] || exit 2
[ "${PIONEER_Q38_NEXT_RMS_NATIVE-0}" = 0 ] || exit 2
[ "${PIONEER_Q38_FFN_RESIDUAL_NATIVE-0}" = 0 ] || exit 2
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
[ "${PIONEER_Q38_FFN_GRAPH_ROOT:-0}" = 0 ] || exit 2
[ "$#" = 3 ] || exit 2
case "$1" in delta|delta_vector|delta_vector_probe|delta_vector_ggml|resident_gdn|resident_gdn_root|packed|activation|activation0|ffn_activation|ffn_native_activation|ffn_resident_activation|ffn_resident_gather|ffn_resident_gather2|ffn_resident_output|projection_input|projection_gather|gdn_output_gather) kind=$1;; *) exit 2;; esac
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
cross=$(CDPATH= cd -- "$2/cross-tools-riscv64/bin" && pwd)/riscv64-unknown-haiku
output=$3
case "$output" in /*) ;; *) exit 2;; esac
[ ! -e "$output" ] && [ ! -L "$output" ] || exit 2
umask 077
mkdir -m 700 "$output"
"$cross-g++" -std=c++11 -O2 -Wall -Wextra -Werror -ffreestanding -fno-exceptions -fno-rtti \
 -fno-stack-protector -fno-pic -fno-pie -ffp-contract=off -ffunction-sections -fdata-sections \
 -fno-unwind-tables -fno-asynchronous-unwind-tables -msmall-data-limit=0 -mno-relax \
 -march=rv32imf -mabi=ilp32 -fstack-usage -c "$directory/blackhole_qwen38_$kind.cpp" -o "$output/program.o"
"$cross-as" -I"$directory" -march=rv32imf -mabi=ilp32 -mno-relax "$directory/blackhole_qwen38_${kind}_start.S" -o "$output/start.o"
linker=$directory/blackhole_scalar.ld
case "$kind" in delta_vector_probe|delta_vector_ggml) linker=$directory/blackhole_qwen38_delta_vector_probe.ld;; esac
case "$kind" in resident_gdn|resident_gdn_root) linker=$directory/blackhole_qwen38_resident_gdn.ld;; esac
"$cross-ld" -m elf32lriscv --no-relax --gc-sections -T "$linker" \
 "$output/start.o" "$output/program.o" -Map "$output/program.map" -o "$output/program.elf"
"$cross-objcopy" -O binary -j .text "$output/program.elf" "$output/code.bin"
"$cross-objcopy" -O binary -j .boot "$output/program.elf" "$output/boot.bin"
"$cross-nm" -n "$output/program.elf" > "$output/program.symbols"
"$cross-objdump" -d "$output/program.elf" > "$output/program.disassembly"
shasum -a 256 "$output/code.bin" "$output/boot.bin"
printf 'OFFLINE Qwen3.8 local %s candidate,NOT installed/driver-selected: %s\n' "$kind" "$output"
