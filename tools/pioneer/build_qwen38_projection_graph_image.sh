#!/bin/sh
[ "${PIONEER_Q38_MIXED_RECURRENT_COHORT_ROOT_IMAGE+x}${PIONEER_Q38_MIXED_ROOT_CAPSULE_IMAGE+x}" = "" ] || exit 2
[ "${PIONEER_Q38_MIXED_RECURRENT_PROJECTION_ROOT_IMAGE+x}" != x ] || exit 2
[ "${PIONEER_Q38_DENSE_PREPARE_IMAGE+x}${PIONEER_Q38_DENSE_ATTENTION_IMAGE+x}${PIONEER_Q38_DENSE_PROJECTION_IMAGE+x}${PIONEER_Q38_DENSE_PARTITION_IMAGE+x}${PIONEER_Q38_MIXED_WORKER_IMAGE+x}${PIONEER_Q38_MIXED_CAPSULE_IMAGE+x}${PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE+x}${PIONEER_Q38_MIXED_BLOCK_ROOT_IMAGE+x}${PIONEER_Q38_MIXED_JOINED_ROOT_IMAGE+x}${PIONEER_Q38_MIXED_PROJECTION_ROOT_IMAGE+x}${PIONEER_Q38_MIXED_DENSE_ROOT_IMAGE+x}" = "" ] || exit 2
[ "${PIONEER_Q38_RESIDENT_CONTINUE_IMAGE+x}" != x ] || exit 2
[ "${PIONEER_Q38_RESIDENT_ADVANCE_IMAGE+x}" != x ] || exit 2
[ "${PIONEER_Q38_RESIDENT_LAYER_LINK_IMAGE+x}" != x ] || exit 2
[ "${PIONEER_Q38_RESIDENT_LAYER_VALIDATION_NATIVE+x}" != x ] || exit 2
[ "${PIONEER_Q38_RESIDENT_LAYER_VALIDATION_CLIENT+x}" != x ] || exit 2
[ "${PIONEER_Q38_RESIDENT_LAYER_NATIVE_COMPONENT+x}" != x ] || exit 2
[ "${PIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT+x}" != x ] || exit 2
[ "${PIONEER_Q38_RESIDENT_LAYER_CONTEXT_COMPILE_ONLY+x}" != x ] || exit 2
[ "${PIONEER_Q38_RESIDENT_LAYER_ROOT_IMAGE+x}" != x ] || exit 2
[ "${PIONEER_Q38_RESIDENT_LAYER_NORM_IMAGE+x}" != x ] || exit 2
[ "${PIONEER_Q38_RESIDENT_LAYER_FFN_IMAGE+x}" != x ] || exit 2
[ "${PIONEER_Q38_RESIDENT_OUTPUT_OWNER+x}" != x ] || exit 2
[ "${PIONEER_Q38_RESIDENT_OUTPUT_CONTEXT_COMPILE_ONLY+x}" != x ] || exit 2
[ "${PIONEER_Q38_RESIDENT_COHORT_ROOT_IMAGE-0}" = 0 ] || exit 2
[ "${PIONEER_Q38_RESIDENT_GDN_OPERATOR_IMAGE-0}" = 0 ] || exit 2
[ "${PIONEER_Q38_RESIDENT_GDN_OUTPUT_OPERATOR_IMAGE-0}" = 0 ] || exit 2
[ "${PIONEER_Q38_RESIDENT_OUTPUT_ROOT_IMAGE-0}" = 0 ] || exit 2
[ "${PIONEER_Q38_RESIDENT_OPERATOR_IMAGE-0}" = 0 ] || exit 2
[ "${PIONEER_Q38_RESIDENT_SILU_PRECISION_NATIVE-0}" = 0 ] || exit 2
[ "${PIONEER_Q38_NEXT_FFN_RESIDUAL_NATIVE-0}" = 0 ] || exit 2
[ "${PIONEER_Q38_CONV_SILU_PRECISION_NATIVE-0}:${PIONEER_Q38_CONV_SILU_PRECISION_VALIDATION_CLIENT-0}" = 0:0 ] || exit 2
[ "${PIONEER_Q38_SILU_PRECISION_IMAGE-0}:${PIONEER_Q38_SILU_LEGACY_CONTROL_IMAGE-0}" = 0:0 ] || exit 2
[ "${PIONEER_Q38_NEXT_RESIDUAL_RMS_NATIVE-0}" = 0 ] || exit 2
[ "${PIONEER_Q38_NEXT_RESIDUAL_RMS_IMAGE-0}" = 0 ] || exit 2
# NEW OFFLINE resident-input parent ROOT,not a native selector/card action.
set -eu
[ "${PIONEER_Q38_NEXT_GDN_OUTPUT_NATIVE-0}" = 0 ] || exit 2
[ "${PIONEER_Q38_NEXT_GDN_NATIVE-0}" = 0 ] || exit 2
[ "${PIONEER_Q38_NEXT_CONV_NATIVE-0}" = 0 ] || exit 2
[ "${PIONEER_Q38_NEXT_PROJECTION_NATIVE-0}" = 0 ] || exit 2
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
[ "${PIONEER_Q38_LEARNED_RMS_VALIDATION_NATIVE-0}:${PIONEER_Q38_LEARNED_RMS_VALIDATION_CLIENT-0}" = 0:0 ] && [ "${PIONEER_DRIVER_BUILD_TAG-}" != 2026102200010041 ] || exit 2
[ "${PIONEER_Q38_EMBEDDING_QUALIFICATION-0}:${PIONEER_Q38_EMBEDDING_CLIENT-0}" = 0:0 ] || exit 2
[ "${PIONEER_Q38_LEARNED_RMS-0}:${PIONEER_Q38_LEARNED_RMS_NATIVE-0}" = 0:0 ] || exit 2
[ "${PIONEER_Q38_PROJECTION_EXECUTION_OWNER-0}" = 0 ] || exit 2
[ "${PIONEER_Q38_EMBEDDING_VALIDATION_NATIVE-0}:${PIONEER_Q38_EMBEDDING_VALIDATION_CLIENT-0}" = 0:0 ] || exit 2
[ "${PIONEER_Q38_EMBEDDING_NATIVE-0}" = 0 ] || exit 2
[ "${PIONEER_Q38_PROJECTION_NEXT_ROOT+x}" != x ] || exit 2
[ "$#" = 2 ] || { [ "$#" = 3 ] && { [ "$3" = --result-stashes ] || [ "$3" = --next-layer-stashes ]; }; } || exit 2
stash_flags=
stash_assembler=
if [ "${3-}" = --result-stashes ];then
 stash_flags=-DPIONEER_Q38_PROJECTION_RESULT_STASH=1
 stash_assembler='--defsym TT_Q38_PROJECTION_RESULT_STASH=1'
fi
if [ "${3-}" = --next-layer-stashes ];then
 stash_flags=-DPIONEER_Q38_PROJECTION_NEXT_ROOT=1
 stash_assembler='--defsym TT_Q38_PROJECTION_NEXT_ROOT=1'
fi
for key in $(env | sed -n 's/^\(PIONEER_[A-Z0-9_]*\)=.*/\1/p');do
 [ "$(printenv "$key" || :)" = 0 ] || exit 2
done
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
"$cross-g++" $flags $stash_flags -DPIONEER_Q38_PROJECTION_GRAPH_ROOT=1 -DPIONEER_Q38_PROJECTION_MATRIX_ROOT=1 \
 -DPIONEER_Q38_FFN_RESIDENT_MATRIX -DPIONEER_Q38_COMPACT_ACTIVATIONS -DPIONEER_Q38_REUSE_ACTIVATIONS \
 -DPIONEER_Q38_WORK_COHORT_CACHE -DPIONEER_Q38_WORK_SKIP_DOT_CHECK -DPIONEER_Q38_WORK_SPARSE_EXPORT \
 -DPIONEER_Q38_WORK_COMPACT_OUTPUT -DPIONEER_Q38_ROOT_CYCLE_PROFILE -DPIONEER_Q38_WORK_SENTINEL_POLL \
 -c "$directory/blackhole_qwen38_projection_graph.cpp" -o "$output/program.o"
"$cross-g++" $flags -Dscalar_main=q38_projection_gather_once -Dscalar_fault_park=block_fault_park \
 -c "$directory/blackhole_qwen38_projection_gather.cpp" -o "$output/gather.o"
"$cross-as" -march=rv32imf -mabi=ilp32 -mno-relax --defsym TT_Q38_DISPATCH_ROOT=1 \
 --defsym TT_Q38_PROJECTION_GRAPH_ROOT=1 $stash_assembler "$directory/blackhole_qwen38_block_start.S" -o "$output/start.o"
"$cross-ld" -m elf32lriscv --no-relax --gc-sections -T "$directory/blackhole_qwen38_projection_graph.ld" \
 "$output/start.o" "$output/program.o" "$output/gather.o" -Map "$output/program.map" -o "$output/program.elf"
for section in boot text prefix graph;do "$cross-objcopy" -O binary -j ".$section" "$output/program.elf" "$output/$section.bin";done
"$cross-nm" -n "$output/program.elf" > "$output/program.symbols"
"$cross-objdump" -d "$output/program.elf" > "$output/program.disassembly"
for section in boot text prefix graph;do shasum -a 256 "$output/$section.bin";done
if [ "${3-}" = --next-layer-stashes ];then
 printf 'OFFLINE NEW QPS2 nonzero LINEAR-layer resident-result parent ONLY;NO native/source/input-producer/model-speed grant:%s\n' "$output"
elif [ -n "$stash_flags" ];then
 printf 'OFFLINE NEW QPS1 fixed resident-result parent ONLY;NO native/source/input-producer/model-speed grant:%s\n' "$output"
else
 printf 'OFFLINE NEW QPR1 resident-input full-projection parent ONLY;NO native/source/input-producer/model-speed grant:%s\n' "$output"
fi
