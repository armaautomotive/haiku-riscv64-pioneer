#!/bin/sh
set -eu
[ "$#" = 1 ] || { echo 'Usage: run_pioneer_resident_gemv_image_tests.sh CONFIGURED_RISCV64_BUILD' >&2; exit 2; }
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
cross=$(CDPATH= cd -- "$1/cross-tools-riscv64/bin" && pwd)/riscv64-unknown-haiku
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-resident-gemv-image.XXXXXXXX")
cleanup() {
 for inputs in 1024 5120 6144; do rm -f "$temporary/$inputs.o" "$temporary/$inputs.elf" "$temporary/$inputs.bin"; done
 rm -f "$temporary/boot.bin" "$temporary/test"
 rmdir "$temporary"
}
trap cleanup EXIT HUP INT TERM
for inputs in 1024 5120 6144; do
 "$cross-as" -march=rv32i -mabi=ilp32 -mno-relax --defsym TT_UNPACK_NONUNIFORM=1 --defsym TT_UNPACK_FULL_BF16=1 --defsym TT_UNPACK_RESIDENT=1 --defsym TT_UNPACK_GEMV=1 --defsym "TT_GEMV_INPUTS=$inputs" "$directory/../blackhole_tensix_unpack.S" -o "$temporary/$inputs.o"
 "$cross-ld" -m elf32lriscv --no-relax -T "$directory/../blackhole_resident_gemv.ld" "$temporary/$inputs.o" -o "$temporary/$inputs.elf"
 "$cross-objcopy" -O binary -j .text "$temporary/$inputs.elf" "$temporary/$inputs.bin"
done
"$cross-objcopy" -O binary -j .boot "$temporary/1024.elf" "$temporary/boot.bin"
${CXX:-c++} -std=c++17 -O1 -Wall -Wextra -Werror -fsanitize=address,undefined "$directory/pioneer_resident_gemv_image_test.cpp" -o "$temporary/test"
"$temporary/test" "$temporary/boot.bin" "$temporary/1024.bin" "$temporary/5120.bin" "$temporary/6144.bin"
