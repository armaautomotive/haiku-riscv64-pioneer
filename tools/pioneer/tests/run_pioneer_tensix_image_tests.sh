#!/bin/sh
set -eu
[ "$#" = 1 ] || { echo 'Usage: run_pioneer_tensix_image_tests.sh CONFIGURED_RISCV64_BUILD' >&2; exit 2; }
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
cross=$(CDPATH= cd -- "$1/cross-tools-riscv64/bin" && pwd)/riscv64-unknown-haiku
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-tensix-image.XXXXXXXX")
cleanup() {
 for name in image config load store operand matrix zeroacc movd kernel kerneloperand kernelmatrix dstread srca srcb purea pureb purematrix unpack unpacka unpackb unpackconfig identity dense densea patternb data; do
  rm -f "$temporary/$name.o" "$temporary/$name.elf" "$temporary/$name.bin"
 done
 rm -f "$temporary/boot.bin" "$temporary/code.bin" "$temporary/test"
 rmdir "$temporary"
}
trap cleanup EXIT HUP INT TERM
"$cross-as" -march=rv32i -mabi=ilp32 -mno-relax "$directory/../blackhole_tensix_matmul.S" -o "$temporary/image.o"
"$cross-ld" -m elf32lriscv --no-relax -T "$directory/../blackhole_tensix_matmul.ld" "$temporary/image.o" -o "$temporary/image.elf"
"$cross-objcopy" -O binary -j .boot "$temporary/image.elf" "$temporary/boot.bin"
"$cross-objcopy" -O binary -j .text "$temporary/image.elf" "$temporary/code.bin"
"$cross-as" -march=rv32i -mabi=ilp32 -mno-relax --defsym TT_CONFIG_SMOKE=1 "$directory/../blackhole_tensix_matmul.S" -o "$temporary/config.o"
"$cross-ld" -m elf32lriscv --no-relax -T "$directory/../blackhole_tensix_matmul.ld" "$temporary/config.o" -o "$temporary/config.elf"
"$cross-objcopy" -O binary -j .text "$temporary/config.elf" "$temporary/config.bin"
"$cross-as" -march=rv32i -mabi=ilp32 -mno-relax --defsym TT_SFPU_LOAD_SMOKE=1 "$directory/../blackhole_tensix_matmul.S" -o "$temporary/load.o"
"$cross-ld" -m elf32lriscv --no-relax -T "$directory/../blackhole_tensix_matmul.ld" "$temporary/load.o" -o "$temporary/load.elf"
"$cross-objcopy" -O binary -j .text "$temporary/load.elf" "$temporary/load.bin"
"$cross-as" -march=rv32i -mabi=ilp32 -mno-relax --defsym TT_SFPU_STORE_SMOKE=1 "$directory/../blackhole_tensix_matmul.S" -o "$temporary/store.o"
"$cross-ld" -m elf32lriscv --no-relax -T "$directory/../blackhole_tensix_matmul.ld" "$temporary/store.o" -o "$temporary/store.elf"
"$cross-objcopy" -O binary -j .text "$temporary/store.elf" "$temporary/store.bin"
for stage in operand matrix zeroacc movd; do
 case "$stage" in operand) symbol=TT_OPERAND_SMOKE;; matrix) symbol=TT_MATRIX_EXEC_SMOKE;; zeroacc) symbol=TT_ZEROACC_SMOKE;; movd) symbol=TT_MOVD_SMOKE;; esac
 "$cross-as" -march=rv32i -mabi=ilp32 -mno-relax --defsym "$symbol=1" "$directory/../blackhole_tensix_matmul.S" -o "$temporary/$stage.o"
 "$cross-ld" -m elf32lriscv --no-relax -T "$directory/../blackhole_tensix_matmul.ld" "$temporary/$stage.o" -o "$temporary/$stage.elf"
 "$cross-objcopy" -O binary -j .text "$temporary/$stage.elf" "$temporary/$stage.bin"
done
for stage in unpack unpacka unpackb unpackconfig identity dense densea patternb data; do
 set --
 case "$stage" in unpacka) set -- --defsym TT_UNPACK_SRCA=1;; unpackb) set -- --defsym TT_UNPACK_SRCB=1;; unpackconfig) set -- --defsym TT_UNPACK_CONFIG=1;; esac
 case "$stage" in identity) set -- --defsym TT_UNPACK_NONUNIFORM=1;; dense) set -- --defsym TT_UNPACK_NONUNIFORM=1 --defsym TT_UNPACK_DENSE=1;; densea) set -- --defsym TT_UNPACK_NONUNIFORM=1 --defsym TT_UNPACK_DENSE=1 --defsym TT_UNPACK_SRCA=1;; patternb) set -- --defsym TT_UNPACK_NONUNIFORM=1 --defsym TT_UNPACK_SRCB=1;; esac
 case "$stage" in data) set -- --defsym TT_UNPACK_NONUNIFORM=1 --defsym TT_UNPACK_FULL_BF16=1;; esac
 "$cross-as" -march=rv32i -mabi=ilp32 -mno-relax "$@" "$directory/../blackhole_tensix_unpack.S" -o "$temporary/$stage.o"
 "$cross-ld" -m elf32lriscv --no-relax -T "$directory/../blackhole_tensix_unpack.ld" "$temporary/$stage.o" -o "$temporary/$stage.elf"
 "$cross-objcopy" -O binary -j .text "$temporary/$stage.elf" "$temporary/$stage.bin"
done
for stage in kernel kerneloperand kernelmatrix dstread srca srcb purea pureb purematrix; do
 set -- --defsym TT_KERNEL_HANDOFF=1
 case "$stage" in kerneloperand) set -- "$@" --defsym TT_OPERAND_SMOKE=1;; kernelmatrix) set -- "$@" --defsym TT_MATRIX_EXEC_SMOKE=1;; dstread) set -- --defsym TT_DST_READ_SMOKE=1;; srca) set -- "$@" --defsym TT_SRCA_ROUNDTRIP=1;; srcb) set -- "$@" --defsym TT_SRCB_ROUNDTRIP=1;; esac
 case "$stage" in purea) set -- "$@" --defsym TT_PURE_KERNEL_HANDOFF=1 --defsym TT_SRCA_ROUNDTRIP=1;; pureb) set -- "$@" --defsym TT_PURE_KERNEL_HANDOFF=1 --defsym TT_SRCB_ROUNDTRIP=1;; purematrix) set -- "$@" --defsym TT_PURE_KERNEL_HANDOFF=1;; esac
 "$cross-as" -march=rv32i -mabi=ilp32 -mno-relax "$@" "$directory/../blackhole_tensix_matmul.S" -o "$temporary/$stage.o"
 "$cross-ld" -m elf32lriscv --no-relax -T "$directory/../blackhole_tensix_matmul.ld" "$temporary/$stage.o" -o "$temporary/$stage.elf"
 "$cross-objcopy" -O binary -j .text "$temporary/$stage.elf" "$temporary/$stage.bin"
done
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined "$directory/pioneer_tensix_image_test.cpp" -o "$temporary/test"
"$temporary/test" "$temporary/boot.bin" "$temporary/code.bin" "$temporary/config.bin" "$temporary/load.bin" "$temporary/store.bin" "$temporary/operand.bin" "$temporary/matrix.bin" "$temporary/zeroacc.bin" "$temporary/movd.bin" "$temporary/kernel.bin" "$temporary/kerneloperand.bin" "$temporary/kernelmatrix.bin" "$temporary/dstread.bin" "$temporary/srca.bin" "$temporary/srcb.bin" "$temporary/purea.bin" "$temporary/pureb.bin" "$temporary/purematrix.bin" "$temporary/unpack.bin" "$temporary/unpacka.bin" "$temporary/unpackb.bin" "$temporary/unpackconfig.bin" "$temporary/identity.bin" "$temporary/dense.bin" "$temporary/densea.bin" "$temporary/patternb.bin" "$temporary/data.bin"
printf 'PASS: fixed RV32I image, whitelisted MMIO and reference matrix fixture; no hardware operation\n'
