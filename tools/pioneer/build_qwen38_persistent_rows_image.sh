#!/bin/sh
# OFFLINE ONLY persistent mixed-quant peer;NO driver install/card operations.
set -eu
[ "${PIONEER_Q38_WORK_ROOT_COMMAND_REUSE:-0}" = 0 ] || exit 2
[ "${PIONEER_Q38_COMMAND_REUSE_QUALIFICATION:-0}" = 0 ] || exit 2
[ "$#" = 2 ] || exit 2
case "${PIONEER_Q38_WORK_TILE_PREFETCH:-0}" in
 0) ;;
 1) [ "${PIONEER_Q38_WORK_DIRECT_SRCB:-0}:${PIONEER_Q38_WORK_AFFINE_REPLAY:-0}:${PIONEER_Q38_VECTOR_PERSISTENT_IMAGE:-0}:${PIONEER_Q38_WORK_FRONTEND_BARRIER:-0}:${PIONEER_Q38_WORK_RAW_UNPACK_CACHE:-0}:${PIONEER_Q38_WORK_BATCH4_FETCH:-0}:${PIONEER_Q38_WORK_BATCH_RAW_INPUT:-0}" = 1:1:9:0:0:0:0 ] || exit 2;;
 *) exit 2;;
esac
case "${PIONEER_Q38_WORK_RAW_UNPACK_CACHE:-0}" in
 0) ;;
 1) [ "${PIONEER_Q38_WORK_DIRECT_SRCB:-0}:${PIONEER_Q38_WORK_AFFINE_REPLAY:-0}:${PIONEER_Q38_STATIC_VECTOR:-0}:${PIONEER_Q38_VECTOR_PERSISTENT_IMAGE:-0}:${PIONEER_Q38_WORK_FRONTEND_BARRIER:-0}:${PIONEER_Q38_WORK_BATCH4_FETCH:-0}:${PIONEER_Q38_WORK_BATCH_RAW_INPUT:-0}" = 1:1:1:9:0:0:0 ] || exit 2;;
 *) exit 2;;
esac
case "${PIONEER_Q38_WORK_FRONTEND_BARRIER:-0}" in
 0) ;;
 1) [ "${PIONEER_Q38_WORK_DIRECT_SRCB:-0}:${PIONEER_Q38_WORK_AFFINE_REPLAY:-0}:${PIONEER_Q38_WORK_BATCH4_FETCH:-0}:${PIONEER_Q38_WORK_BATCH_RAW_INPUT:-0}:${PIONEER_Q38_VECTOR_PERSISTENT_IMAGE:-0}" = 1:1:0:0:9 ] || exit 2;;
 *) exit 2;;
esac
case "${PIONEER_Q38_WORK_BATCH_RAW_INPUT:-0}" in
 0) ;;
 1) [ "${PIONEER_Q38_WORK_BATCH4_FETCH:-0}" = 1 ] || exit 2;;
 *) exit 2;;
esac
case "${PIONEER_Q38_WORK_BATCH4_FETCH:-0}" in
 0) ;;
 1) [ "${PIONEER_Q38_WORK_DIRECT_SRCB:-0}:${PIONEER_Q38_WORK_AFFINE_REPLAY:-0}:${PIONEER_Q38_WORK_DOT_ZERO_ELISION:-0}" = 1:1:1 ] || exit 2;;
 *) exit 2;;
esac
case "${PIONEER_Q38_WORK_DIRECT_SRCB:-0}" in
 0) ;;
 1) [ "${PIONEER_Q38_BLOCK_SCALE_REUSE:-0}:${PIONEER_Q38_BLOCK_AFFINE_DST:-0}:${PIONEER_Q38_INLINE_CONFIG:-0}:${PIONEER_Q38_WORK_DEFER_AFFINE:-0}:${PIONEER_Q38_WORK_ZERO_MINIMUM:-0}:${PIONEER_Q38_WORK_INPUT_SUM_CACHE:-0}:${PIONEER_Q38_WORK_SKIP_DOT_CHECK:-0}:${PIONEER_Q38_VECTOR_PERSISTENT_IMAGE:-0}:${PIONEER_Q38_NATIVE_Q8:-0}:${PIONEER_Q38_PREDECODED_Q4:-0}:${PIONEER_Q38_Q4_GROUP32:-0}:${PIONEER_Q38_PREPARED_Q4_BLOCK:-0}:${PIONEER_Q38_PREPARED_Q4_AFFINE:-0}" = 1:1:1:1:1:1:1:9:0:0:0:0:0 ] || exit 2;;
 *) exit 2;;
esac
case "${PIONEER_Q38_WORK_DOT_ZERO_ELISION:-0}" in
 0) ;;
 1) [ "${PIONEER_Q38_BLOCK_SCALE_REUSE:-0}:${PIONEER_Q38_BLOCK_AFFINE_DST:-0}:${PIONEER_Q38_INLINE_CONFIG:-0}:${PIONEER_Q38_WORK_DEFER_AFFINE:-0}:${PIONEER_Q38_WORK_ZERO_MINIMUM:-0}:${PIONEER_Q38_VECTOR_PERSISTENT_IMAGE:-0}:${PIONEER_Q38_Q4_GROUP32:-0}:${PIONEER_Q38_PREPARED_Q4_AFFINE:-0}:${PIONEER_Q38_PREDECODED_Q4:-0}:${PIONEER_Q38_PREPARED_Q4_BLOCK:-0}:${PIONEER_Q38_NATIVE_Q8:-0}" = 1:1:1:1:1:9:0:0:0:0:0 ] || exit 2;;
 *) exit 2;;
esac
case "${PIONEER_Q38_WORK_AFFINE_REPLAY:-0}" in
 0) ;;
 1) [ "${PIONEER_Q38_BLOCK_SCALE_REUSE:-0}:${PIONEER_Q38_BLOCK_AFFINE_DST:-0}:${PIONEER_Q38_INLINE_CONFIG:-0}:${PIONEER_Q38_Q4_GROUP32:-0}:${PIONEER_Q38_PREPARED_Q4_AFFINE:-0}:${PIONEER_Q38_PREDECODED_Q4:-0}:${PIONEER_Q38_PREPARED_Q4_BLOCK:-0}:${PIONEER_Q38_VECTOR_PERSISTENT_IMAGE:-0}:${PIONEER_Q38_WORK_DEFER_AFFINE:-0}:${PIONEER_Q38_NATIVE_OPERAND_STORE:-0}:${PIONEER_Q38_WORK_ZERO_MINIMUM:-0}:${PIONEER_Q38_WORK_INPUT_SUM_CACHE:-0}:${PIONEER_Q38_QUEUED_MATRIX_ENTRY:-0}:${PIONEER_Q38_WORK_SKIP_DOT_CHECK:-0}:${PIONEER_Q38_REPLAY_OPERAND:-0}:${PIONEER_Q38_CACHED_AFFINE:-0}:${PIONEER_Q38_STATIC_VECTOR:-0}:${PIONEER_Q38_PIPELINED_MATRIX:-0}" = 1:1:1:0:0:0:0:9:1:1:1:1:1:1:1:1:1:1 ] || exit 2;;
 *) exit 2;;
esac
case "${PIONEER_Q38_PREPARED_Q4_BLOCK:-0}" in
 0) ;;
 1) [ "${PIONEER_Q38_BLOCK_SCALE_REUSE:-0}:${PIONEER_Q38_BLOCK_AFFINE_DST:-0}:${PIONEER_Q38_INLINE_CONFIG:-0}:${PIONEER_Q38_Q4_GROUP32:-0}:${PIONEER_Q38_PREPARED_Q4_AFFINE:-0}:${PIONEER_Q38_PREDECODED_Q4:-0}:${PIONEER_Q38_NATIVE_Q8:-0}:${PIONEER_Q38_VECTOR_PERSISTENT_IMAGE:-0}:${PIONEER_Q38_WORK_DEFER_AFFINE:-0}:${PIONEER_Q38_NATIVE_OPERAND_STORE:-0}:${PIONEER_Q38_WORK_ZERO_MINIMUM:-0}:${PIONEER_Q38_WORK_INPUT_SUM_CACHE:-0}:${PIONEER_Q38_QUEUED_MATRIX_ENTRY:-0}:${PIONEER_Q38_WORK_SKIP_DOT_CHECK:-0}:${PIONEER_Q38_REPLAY_OPERAND:-0}:${PIONEER_Q38_CACHED_AFFINE:-0}:${PIONEER_Q38_STATIC_VECTOR:-0}:${PIONEER_Q38_PIPELINED_MATRIX:-0}" = 1:1:1:0:0:0:0:9:1:1:1:1:1:1:1:1:1:1 ] || exit 2;;
 *) exit 2;;
esac
case "${PIONEER_Q38_PREDECODED_Q4_STICKY:-0}" in
 0) ;;
 1) [ "${PIONEER_Q38_PREDECODED_Q4:-0}:${PIONEER_Q38_NATIVE_Q8:-0}:${PIONEER_Q38_BLOCK_SCALE_REUSE:-0}:${PIONEER_Q38_BLOCK_AFFINE_DST:-0}:${PIONEER_Q38_INLINE_CONFIG:-0}:${PIONEER_Q38_Q4_GROUP32:-0}:${PIONEER_Q38_PREPARED_Q4_AFFINE:-0}:${PIONEER_Q38_VECTOR_PERSISTENT_IMAGE:-0}:${PIONEER_Q38_WORK_DEFER_AFFINE:-0}:${PIONEER_Q38_NATIVE_OPERAND_STORE:-0}:${PIONEER_Q38_WORK_ZERO_MINIMUM:-0}:${PIONEER_Q38_WORK_INPUT_SUM_CACHE:-0}:${PIONEER_Q38_QUEUED_MATRIX_ENTRY:-0}:${PIONEER_Q38_WORK_SKIP_DOT_CHECK:-0}:${PIONEER_Q38_REPLAY_OPERAND:-0}:${PIONEER_Q38_CACHED_AFFINE:-0}:${PIONEER_Q38_STATIC_VECTOR:-0}:${PIONEER_Q38_PIPELINED_MATRIX:-0}" = 1:0:1:1:1:0:0:9:1:1:1:1:1:1:1:1:1:1 ] || exit 2;;
 *) exit 2;;
esac
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
cross=$(CDPATH= cd -- "$1/cross-tools-riscv64/bin" && pwd)/riscv64-unknown-haiku
output=$2
case "${PIONEER_Q38_FAULT_RETURN:-0}" in 0) fault_as=;; 1) fault_as='--defsym PIONEER_Q38_FAULT_RETURN=1';; *) exit 2;; esac
case "${PIONEER_Q38_PEER_DWARF:-0}" in 0) dwarf=;; 1) dwarf=-g;; *) exit 2;; esac
case "${PIONEER_Q38_PEER_PROGRESS:-0}" in 0) ;; 1) [ "${PIONEER_Q38_FAULT_RETURN:-0}:${PIONEER_Q38_VECTOR_PERSISTENT_IMAGE:-0}" = 1:8 ] || exit 2; dwarf="$dwarf -DPIONEER_Q38_PEER_PROGRESS";; *) exit 2;; esac
case "${PIONEER_Q38_VECTOR_PERSISTENT_IMAGE:-0}" in
 0) source=blackhole_qwen38_persistent.cpp;;
 1) source=blackhole_qwen38_vector_persistent.cpp;;
 2) source=blackhole_qwen38_vector_reuse_persistent.cpp;;
 3) source=blackhole_qwen38_vector_fixed_persistent.cpp;;
 4) source=blackhole_qwen38_vector_phase_persistent.cpp;;
 5) source=blackhole_qwen38_vector_resident_persistent.cpp;;
 6) source=blackhole_qwen38_bfloat_persistent.cpp;;
 7) source=blackhole_qwen38_sfpu_accumulator_persistent.cpp;;
 8) [ "${PIONEER_Q38_PEER_PROGRESS:-0}" = 1 ] || exit 2; source=blackhole_qwen38_progress_persistent.cpp;;
 9) source=blackhole_qwen38_int_sfpu_persistent.cpp;;
 *) exit 2;;
esac
case "$output" in /*) ;; *) exit 2;; esac
case "${PIONEER_Q38_Q4_TWO_PHASE:-0}" in
 0) q4_phases=;;
 1) case "${PIONEER_Q38_VECTOR_PERSISTENT_IMAGE:-0}" in 5|9) ;; *) exit 2;; esac
    q4_phases=-DPIONEER_Q38_Q4_TWO_PHASE;;
 *) exit 2;;
esac
case "${PIONEER_Q38_PIPELINED_MATRIX:-0}" in
 0) ;;
 1) [ "${PIONEER_Q38_VECTOR_PERSISTENT_IMAGE:-0}:${PIONEER_Q38_Q4_TWO_PHASE:-0}" = 9:1 ] || exit 2
    q4_phases="$q4_phases -DPIONEER_Q38_PIPELINED_MATRIX";;
 *) exit 2;;
esac
case "${PIONEER_Q38_STATIC_VECTOR:-0}" in
 0) ;;
 1) [ "${PIONEER_Q38_PIPELINED_MATRIX:-0}" = 1 ] || exit 2
    q4_phases="$q4_phases -DPIONEER_Q38_STATIC_VECTOR";;
 *) exit 2;;
esac
case "${PIONEER_Q38_CACHED_AFFINE:-0}" in
 0) ;;
 1) [ "${PIONEER_Q38_STATIC_VECTOR:-0}" = 1 ] || exit 2
    q4_phases="$q4_phases -DPIONEER_Q38_CACHED_AFFINE";;
 *) exit 2;;
esac
case "${PIONEER_Q38_REPLAY_OPERAND:-0}" in
 0) ;;
 1) [ "${PIONEER_Q38_CACHED_AFFINE:-0}" = 1 ] || exit 2
    q4_phases="$q4_phases -DPIONEER_Q38_REPLAY_OPERAND";;
 *) exit 2;;
esac
case "${PIONEER_Q38_WORK_SKIP_DOT_CHECK:-0}" in
 0) ;;
 1) [ "${PIONEER_Q38_REPLAY_OPERAND:-0}" = 1 ] || exit 2
    q4_phases="$q4_phases -DPIONEER_Q38_WORK_SKIP_DOT_CHECK";;
 *) exit 2;;
esac
case "${PIONEER_Q38_QUEUED_MATRIX_ENTRY:-0}" in
 0) ;;
 1) [ "${PIONEER_Q38_WORK_SKIP_DOT_CHECK:-0}" = 1 ] && [ "${PIONEER_Q38_PIPELINED_MATRIX:-0}" = 1 ] || exit 2
    q4_phases="$q4_phases -DPIONEER_Q38_QUEUED_MATRIX_ENTRY";;
 *) exit 2;;
esac
case "${PIONEER_Q38_WORK_INPUT_SUM_CACHE:-0}" in
 0) ;;
 1) [ "${PIONEER_Q38_VECTOR_PERSISTENT_IMAGE:-0}:${PIONEER_Q38_QUEUED_MATRIX_ENTRY:-0}" = 9:1 ] || exit 2
    q4_phases="$q4_phases -DPIONEER_Q38_WORK_INPUT_SUM_CACHE";;
 *) exit 2;;
esac
case "${PIONEER_Q38_WORK_ZERO_MINIMUM:-0}" in
 0) ;;
 1) [ "${PIONEER_Q38_WORK_INPUT_SUM_CACHE:-0}" = 1 ] || exit 2
    q4_phases="$q4_phases -DPIONEER_Q38_WORK_ZERO_MINIMUM";;
 *) exit 2;;
esac
case "${PIONEER_Q38_NATIVE_OPERAND_STORE:-0}" in
 0) ;;
 1) [ "${PIONEER_Q38_WORK_ZERO_MINIMUM:-0}" = 1 ] || exit 2
    q4_phases="$q4_phases -DPIONEER_Q38_NATIVE_OPERAND_STORE";;
 *) exit 2;;
esac
case "${PIONEER_Q38_WORK_DEFER_AFFINE:-0}" in
 0) ;;
 1) [ "${PIONEER_Q38_NATIVE_OPERAND_STORE:-0}" = 1 ] || exit 2
    q4_phases="$q4_phases -DPIONEER_Q38_WORK_DEFER_AFFINE";;
 *) exit 2;;
esac
case "${PIONEER_Q38_INLINE_CONFIG:-0}" in
 0) ;;
 1) [ "${PIONEER_Q38_WORK_DEFER_AFFINE:-0}" = 1 ] || exit 2
    q4_phases="$q4_phases -DPIONEER_Q38_INLINE_CONFIG";;
 *) exit 2;;
esac
case "${PIONEER_Q38_PREPARED_Q4_BLOCK:-0}" in
 0) ;;
 1) q4_phases="$q4_phases -DPIONEER_Q38_PREPARED_Q4_BLOCK";;
esac
case "${PIONEER_Q38_WORK_AFFINE_REPLAY:-0}" in
 0) ;;
 1) q4_phases="$q4_phases -DPIONEER_Q38_WORK_AFFINE_REPLAY";;
esac
case "${PIONEER_Q38_WORK_DIRECT_SRCB:-0}" in
 0) ;;
 1) q4_phases="$q4_phases -DPIONEER_Q38_WORK_DIRECT_SRCB";;
esac
if [ "${PIONEER_Q38_WORK_BATCH4_FETCH:-0}" = 1 ]; then q4_phases="$q4_phases -DPIONEER_Q38_WORK_BATCH4_FETCH"; fi
if [ "${PIONEER_Q38_WORK_BATCH_RAW_INPUT:-0}" = 1 ]; then q4_phases="$q4_phases -DPIONEER_Q38_WORK_BATCH_RAW_INPUT"; fi
if [ "${PIONEER_Q38_WORK_FRONTEND_BARRIER:-0}" = 1 ]; then q4_phases="$q4_phases -DPIONEER_Q38_WORK_FRONTEND_BARRIER"; fi
if [ "${PIONEER_Q38_WORK_RAW_UNPACK_CACHE:-0}" = 1 ]; then q4_phases="$q4_phases -DPIONEER_Q38_WORK_RAW_UNPACK_CACHE"; fi
if [ "${PIONEER_Q38_WORK_TILE_PREFETCH:-0}" = 1 ]; then q4_phases="$q4_phases -DPIONEER_Q38_WORK_TILE_PREFETCH"; fi
case "${PIONEER_Q38_WORK_DOT_ZERO_ELISION:-0}" in
 0) ;;
 1) q4_phases="$q4_phases -DPIONEER_Q38_WORK_DOT_ZERO_ELISION";;
esac
case "${PIONEER_Q38_PREPARED_Q4_AFFINE:-0}" in
 0) ;;
 1) [ "${PIONEER_Q38_INLINE_CONFIG:-0}" = 1 ] || exit 2
    q4_phases="$q4_phases -DPIONEER_Q38_PREPARED_Q4_AFFINE";;
 *) exit 2;;
esac
case "${PIONEER_Q38_BLOCK_AFFINE_DST:-0}" in
 0) ;;
 1) [ "${PIONEER_Q38_INLINE_CONFIG:-0}:${PIONEER_Q38_PREPARED_Q4_AFFINE:-0}" = 1:0 ] || exit 2
    q4_phases="$q4_phases -DPIONEER_Q38_BLOCK_AFFINE_DST";;
 *) exit 2;;
esac
case "${PIONEER_Q38_BLOCK_SCALE_REUSE:-0}" in
 0) ;;
 1) [ "${PIONEER_Q38_BLOCK_AFFINE_DST:-0}" = 1 ] || exit 2
    q4_phases="$q4_phases -DPIONEER_Q38_BLOCK_SCALE_REUSE";;
 *) exit 2;;
esac
case "${PIONEER_Q38_Q4_GROUP32:-0}" in
 0) ;;
 1) [ "${PIONEER_Q38_BLOCK_SCALE_REUSE:-0}:${PIONEER_Q38_BLOCK_AFFINE_DST:-0}:${PIONEER_Q38_WORK_SKIP_DOT_CHECK:-0}" = 1:1:1 ] || exit 2
    q4_phases="$q4_phases -DPIONEER_Q38_Q4_GROUP32";;
 *) exit 2;;
esac
case "${PIONEER_Q38_PREDECODED_Q4:-0}" in
 0) ;;
 1) [ "${PIONEER_Q38_BLOCK_SCALE_REUSE:-0}:${PIONEER_Q38_Q4_GROUP32:-0}:${PIONEER_Q38_PREPARED_Q4_AFFINE:-0}" = 1:0:0 ] || exit 2
    q4_phases="$q4_phases -DPIONEER_Q38_PREDECODED_Q4";;
 *) exit 2;;
esac
case "${PIONEER_Q38_PREDECODED_Q4_STICKY:-0}" in
 0) ;;
 1) [ "${PIONEER_Q38_PREDECODED_Q4:-0}:${PIONEER_Q38_NATIVE_Q8:-0}" = 1:0 ] || exit 2
    q4_phases="$q4_phases -DPIONEER_Q38_PREDECODED_Q4_STICKY";;
 *) exit 2;;
esac
case "${PIONEER_Q38_NATIVE_Q8:-0}" in
 0) ;;
 1) [ "${PIONEER_Q38_BLOCK_SCALE_REUSE:-0}:${PIONEER_Q38_Q4_GROUP32:-0}:${PIONEER_Q38_PREDECODED_Q4:-0}:${PIONEER_Q38_PREPARED_Q4_AFFINE:-0}" = 1:0:0:0 ] || exit 2
    q4_phases="$q4_phases -DPIONEER_Q38_NATIVE_Q8";;
 *) exit 2;;
esac
[ ! -e "$output" ] && [ ! -L "$output" ] || exit 1
umask 077; mkdir -m 700 "$output"
"$cross-g++" -std=c++11 -O2 -Wall -Wextra -Werror -ffreestanding -fno-exceptions -fno-rtti \
 -fno-stack-protector -fno-pic -fno-pie -ffp-contract=off -ffunction-sections -fdata-sections \
 -fno-unwind-tables -fno-asynchronous-unwind-tables -msmall-data-limit=0 -mno-relax \
 -march=rv32imf -mabi=ilp32 -fstack-usage $dwarf $q4_phases -DPIONEER_Q38_FAST_PANEL -DPIONEER_Q38_FAST_SCALE \
 -c "$directory/$source" -o "$output/program.o"
"$cross-as" -march=rv32imf -mabi=ilp32 -mno-relax $fault_as "$directory/blackhole_qwen38_persistent_start.S" -o "$output/start.o"
linker=blackhole_qwen38_block.ld; sections='boot text'
if [ "${PIONEER_Q38_VECTOR_PERSISTENT_IMAGE:-0}" = 9 ]; then linker=blackhole_qwen38_int_sfpu.ld; sections='boot text affine'; fi
if [ "${PIONEER_Q38_WORK_BATCH4_FETCH:-0}" = 1 ] || [ "${PIONEER_Q38_WORK_FRONTEND_BARRIER:-0}" = 1 ] || [ "${PIONEER_Q38_WORK_RAW_UNPACK_CACHE:-0}" = 1 ] || [ "${PIONEER_Q38_WORK_TILE_PREFETCH:-0}" = 1 ]; then sections="$sections fetch"; fi
"$cross-ld" -m elf32lriscv --no-relax --gc-sections -T "$directory/$linker" \
 "$output/start.o" "$output/program.o" -Map "$output/program.map" -o "$output/program.elf"
for section in $sections; do "$cross-objcopy" -O binary -j ".$section" "$output/program.elf" "$output/$section.bin"; done
"$cross-nm" -n "$output/program.elf" > "$output/program.symbols"
"$cross-objdump" -d "$output/program.elf" > "$output/program.disassembly"
for section in $sections; do shasum -a 256 "$output/$section.bin"; done
printf 'OFFLINE persistent peer build only;NO installation/native or graph qualification: %s\n' "$output"
