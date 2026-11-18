#!/bin/sh
# Compile the standalone 4B ROOT and checked Tensix projection code.
set -eu
[ "$#" = 2 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
cross=$(CDPATH= cd -- "$1/cross-tools-riscv64/bin" && pwd)/riscv64-unknown-haiku
output=$2
case "$output" in /*) ;; *) exit 2;; esac
umask 077
mkdir -m 700 "$output"
flags='-std=c++17 -Os -Wall -Wextra -Werror -ffreestanding -fno-exceptions -fno-rtti -fno-stack-protector -fno-pic -fno-pie -fno-builtin -ffp-contract=off -ffunction-sections -fdata-sections -fno-unwind-tables -fno-asynchronous-unwind-tables -msmall-data-limit=0 -mno-relax -march=rv32imf_zicsr -mabi=ilp32 -fstack-usage'
for source in blackhole_qwen35_work blackhole_qwen35_token_core blackhole_qwen35_device_io; do
 "$cross-g++" $flags -c "$directory/$source.cpp" -o "$output/$source.o"
done
"$cross-g++" $flags -DPIONEER_Q35_PROJECTION_IMAGE=1 -Dblock_main=q35_work_matrix_band \
 -c "$directory/blackhole_qwen38_rows.cpp" -o "$output/matrix.o"
"$cross-as" -march=rv32imf_zicsr -mabi=ilp32 -mno-relax "$directory/blackhole_qwen35_work_start.S" -o "$output/start.o"
"$cross-ld" -m elf32lriscv --no-relax --gc-sections -T "$directory/blackhole_qwen35_work.ld" \
 "$output/start.o" "$output/blackhole_qwen35_work.o" "$output/blackhole_qwen35_token_core.o" \
 "$output/blackhole_qwen35_device_io.o" "$output/matrix.o" -Map "$output/work.map" -o "$output/work.elf"
for section in boot text; do "$cross-objcopy" -O binary -j ".$section" "$output/work.elf" "$output/$section.bin"; done
"$cross-nm" -n "$output/work.elf" > "$output/work.symbols"
"$cross-objdump" -d "$output/work.elf" > "$output/work.disassembly"
"$cross-nm" -u "$output/work.elf" > "$output/work.imports"
test ! -s "$output/work.imports"
awk '$3 ~ /^(div|divu|rem|remu|mulh|mulhu|mulhsu|fdiv[.]s|fsqrt[.]s|fmadd[.]s|fmsub[.]s|fnmadd[.]s|fnmsub[.]s|fcvt[.]w[.]s|fcvt[.]wu[.]s)$/ {
 print; bad=1
} END {exit bad}' "$output/work.disassembly"
python3 - "$output" <<'PY'
from pathlib import Path
from struct import iter_unpack, unpack
import sys

out = Path(sys.argv[1])
boot = (out / 'boot.bin').read_bytes()
code = (out / 'text.bin').read_bytes()
symbols = {name: int(value, 16) for value, kind, name in
           (line.split() for line in (out / 'work.symbols').read_text().splitlines())
           if kind == 'T'}
if boot != bytes.fromhex('6fa00300') or not 0 < len(code) <= 0x20000 or len(code) % 4:
    raise SystemExit('unexpected ROOT image size or boot jump')
park = symbols['q35_work_park']
fault = symbols['q35_work_fault_park']
for address in (park, fault):
    relative = address - symbols['q35_work_entry']
    if relative < 0 or relative+8 > len(code) or code[relative:relative+8].hex() != '6f0000006ff0dfff':
        raise SystemExit('unexpected terminal park instructions')
words = ''.join(f'\t0x{word:08x}U,\n' for word, in iter_unpack('<I', code))
(out / 'PioneerQwen35WorkImage.h').write_text(
    '#pragma once\nnamespace PioneerQwen35 {\n'
    f'inline constexpr unsigned WorkBootWord=0x{unpack("<I", boot)[0]:08x}U;\n'
    f'inline constexpr unsigned WorkParkPc=0x{park:08x}U, WorkFaultPc=0x{fault:08x}U;\n'
    'inline constexpr unsigned WorkTextWords[]={\n' + words + '};\n}\n',
    encoding='ascii')
PY
shasum -a 256 "$output/boot.bin" "$output/text.bin"
