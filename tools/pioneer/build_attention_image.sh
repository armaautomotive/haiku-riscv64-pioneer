#!/bin/sh
# Offline image only: no driver build/install, card access or supplied program IO.
set -eu
[ "$#" = 2 ] || { echo 'Usage: build_attention_image.sh CONFIGURED_RISCV64_BUILD FRESH_ABSOLUTE_OUTPUT' >&2; exit 2; }
attention_source=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
attention_cross=$(CDPATH= cd -- "$1/cross-tools-riscv64/bin" && pwd)/riscv64-unknown-haiku
attention_output=$2
attention_program=blackhole_attention.cpp
case ${PIONEER_COORDINATED_ATTENTION:-0} in 0|1) ;; *) exit 2;; esac
case ${PIONEER_COORDINATED_GLU:-0} in 0|1) ;; *) exit 2;; esac
case ${PIONEER_GRAPH_FAST_DIVIDE:-0} in 0|1) ;; *) exit 2;; esac
case ${PIONEER_ATTENTION_CACHED_QUERY:-0} in 0|1) ;; *) exit 2;; esac
case ${PIONEER_COORDINATED_PULL_INPUT:-0} in 0|1) ;; *) exit 2;; esac
case ${PIONEER_ATTENTION_PARALLEL_ROPE:-0} in 0|1) ;; *) exit 2;; esac
[ "${PIONEER_ATTENTION_PARALLEL_ROPE:-0}" = 0 ] || [ "${PIONEER_COORDINATED_PULL_INPUT:-0}" = 1 ] || exit 2
[ "${PIONEER_COORDINATED_PULL_INPUT:-0}" = 0 ] || { [ "${PIONEER_COORDINATED_GLU:-0}" = 1 ] && [ "${PIONEER_ATTENTION_CACHED_QUERY:-0}" = 1 ]; } || exit 2
[ "${PIONEER_COORDINATED_GLU:-0}" = 0 ] || [ "${PIONEER_COORDINATED_ATTENTION:-0}" = 1 ] || exit 2
case ${PIONEER_RESIDENT_ATTENTION:-0} in
 0) ;;
 1) attention_program=blackhole_resident_attention.cpp ;;
 *) echo 'PIONEER_RESIDENT_ATTENTION must be 0 or 1' >&2; exit 2;;
esac
if [ "${PIONEER_COORDINATED_ATTENTION:-0}" = 1 ]; then
 [ "${PIONEER_RESIDENT_ATTENTION:-0}" = 0 ] || exit 2
 attention_program=blackhole_coordinated_attention.cpp
fi
case "$attention_output" in /*) ;; *) echo 'Absolute output required' >&2; exit 2;; esac
[ ! -e "$attention_output" ] && [ ! -L "$attention_output" ] || { echo 'Fresh output required' >&2; exit 2; }
umask 077
mkdir -m 700 "$attention_output"
"$attention_cross-g++" -std=c++11 -Os -Wall -Wextra -Werror -ffreestanding -DPIONEER_ATTENTION_FREESTANDING \
 -fno-exceptions -fno-rtti -fno-stack-protector -fno-pic -fno-pie \
 -ffunction-sections -fdata-sections -ffp-contract=off -fno-unwind-tables \
 -fno-asynchronous-unwind-tables -msmall-data-limit=0 -mno-relax \
 -march=rv32imf -mabi=ilp32 -fstack-usage -DPIONEER_COORDINATED_GLU=${PIONEER_COORDINATED_GLU:-0} \
 -DPIONEER_GRAPH_FAST_DIVIDE=${PIONEER_GRAPH_FAST_DIVIDE:-0} \
 -DPIONEER_ATTENTION_CACHED_QUERY=${PIONEER_ATTENTION_CACHED_QUERY:-0} \
 -DPIONEER_COORDINATED_PULL_INPUT=${PIONEER_COORDINATED_PULL_INPUT:-0} \
 -DPIONEER_ATTENTION_PARALLEL_ROPE=${PIONEER_ATTENTION_PARALLEL_ROPE:-0} -c "$attention_source/$attention_program" -o "$attention_output/program.o"
set --
if [ "${PIONEER_COORDINATED_ATTENTION:-0}" = 1 ]; then set -- --defsym TT_ATTENTION_COORDINATED=1; fi
"$attention_cross-as" -march=rv32imf -mabi=ilp32 -mno-relax "$@" \
 "$attention_source/blackhole_attention_start.S" -o "$attention_output/start.o"
attention_linker="$attention_source/blackhole_attention.ld"
set -- "$attention_output/start.o" "$attention_output/program.o"
if [ "${PIONEER_ATTENTION_PARALLEL_ROPE:-0}" = 1 ]; then
 "$attention_cross-g++" -std=c++11 -Os -Wall -Wextra -Werror -ffreestanding -fno-exceptions -fno-rtti \
  -fno-stack-protector -fno-pic -fno-pie -ffunction-sections -fdata-sections -ffp-contract=off \
  -fno-unwind-tables -fno-asynchronous-unwind-tables -msmall-data-limit=0 -mno-relax \
  -march=rv32imf -mabi=ilp32 -fstack-usage -DPIONEER_GRAPH_FAST_RMS=1 \
  -DPIONEER_GRAPH_FAST_DIVIDE=1 -DPIONEER_GRAPH_ROPE_FAST_FMA=1 -DPIONEER_GRAPH_FMA_FIXED_NORMALIZE=1 \
  -c "$attention_source/blackhole_rope_group.cpp" -o "$attention_output/rope.o"
 attention_linker="$attention_source/blackhole_rope_attention.ld"
 set -- "$@" "$attention_output/rope.o"
fi
"$attention_cross-ld" -m elf32lriscv --no-relax --gc-sections \
 -T "$attention_linker" "$@" -Map "$attention_output/program.map" -o "$attention_output/program.elf"
"$attention_cross-objcopy" -O binary -j .text "$attention_output/program.elf" "$attention_output/code.bin"
"$attention_cross-objcopy" -O binary -j .boot "$attention_output/program.elf" "$attention_output/boot.bin"
if [ "${PIONEER_ATTENTION_PARALLEL_ROPE:-0}" = 1 ]; then
 "$attention_cross-objcopy" -O binary -j .rope "$attention_output/program.elf" "$attention_output/rope.bin"
 shasum -a 256 "$attention_output/rope.bin"
fi
"$attention_cross-objdump" -d "$attention_output/program.elf" > "$attention_output/program.disassembly"
"$attention_cross-nm" -n "$attention_output/program.elf" > "$attention_output/program.symbols"
shasum -a 256 "$attention_output/code.bin" "$attention_output/boot.bin"
printf 'OFFLINE attention candidate only, not registered/exposed by driver: %s\n' "$attention_output"
