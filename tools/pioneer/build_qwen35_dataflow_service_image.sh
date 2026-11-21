#!/bin/sh
# Build the isolated, resident per-core Qwen35 dataflow worker image offline.
set -eu
[ "$#" = 2 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
build=$(CDPATH= cd -- "$1" && pwd)
output=$2
case "$output" in /*) ;; *) exit 2;; esac
[ ! -e "$output" ] && [ ! -L "$output" ] || exit 2
cross="$build/cross-tools-riscv64/bin/riscv64-unknown-haiku"
objects="$build/objects/haiku/riscv64"
sdk="$objects/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop"
umask 077
mkdir -m 700 "$output"
flags="-std=c++17 -Os -Wall -Wextra -Werror -ffreestanding -fno-exceptions -fno-rtti -fno-stack-protector -fno-pic -fno-pie -fno-builtin -ffp-contract=off -ffunction-sections -fdata-sections -fno-unwind-tables -fno-asynchronous-unwind-tables -msmall-data-limit=0 -mno-relax -march=rv32imf_zicsr -mabi=ilp32"

"$cross-g++" $flags -I"$directory" -I"$sdk/headers" \
	-idirafter "$sdk/headers/posix" \
	-c "$directory/PioneerQwen35DataflowServiceImage.cpp" -o "$output/service.o"
"$cross-g++" $flags -I"$directory" -I"$sdk/headers" \
	-idirafter "$sdk/headers/posix" \
	-c "$directory/PioneerQwen35DataflowMemory.cpp" -o "$output/memory.o"
"$cross-as" -march=rv32imf_zicsr -mabi=ilp32 -mno-relax \
	"$directory/blackhole_qwen35_dataflow_service_start.S" -o "$output/start.o"
"$cross-ld" -m elf32lriscv --no-relax --gc-sections \
	-T "$directory/blackhole_qwen35_dataflow_service.ld" \
	"$output/start.o" "$output/service.o" "$output/memory.o" \
	-Map "$output/service.map" \
	-o "$output/service.elf"
"$cross-objcopy" -O binary -j .boot "$output/service.elf" "$output/boot.bin"
"$cross-objcopy" -O binary -j .text "$output/service.elf" "$output/text.bin"
"$cross-nm" -u "$output/service.elf" > "$output/imports.txt"
test ! -s "$output/imports.txt"
"$cross-nm" -n "$output/service.elf" > "$output/symbols.txt"
"$cross-objdump" -d "$output/service.elf" > "$output/disassembly.txt"
python3 - "$output" <<'PY'
from pathlib import Path
from struct import iter_unpack, unpack
import sys

out = Path(sys.argv[1])
boot = (out / 'boot.bin').read_bytes()
code = (out / 'text.bin').read_bytes()
symbols = {name: int(value, 16) for value, kind, name in
           (line.split() for line in (out / 'symbols.txt').read_text().splitlines())
           if kind == 'T'}
if boot != bytes.fromhex('6fc00600') or not 0 < len(code) <= 0xe000 or len(code) % 4:
    raise SystemExit('unexpected resident dataflow image size or boot jump')
if symbols.get('qwen35_dataflow_entry') != 0x72000:
    raise SystemExit('unexpected resident dataflow entry address')
words = ''.join(f'\t0x{word:08x}U,\n' for word, in iter_unpack('<I', code))
(out / 'PioneerQwen35DataflowServiceImage.h').write_text(
    '#pragma once\nnamespace PioneerQwen35 {\n'
    f'inline constexpr unsigned DataflowServiceImageBootWord=0x{unpack("<I", boot)[0]:08x}U;\n'
    'inline constexpr unsigned DataflowServiceTextWords[]={\n' + words + '};\n}\n',
    encoding='ascii')
PY
if "$cross-objdump" -d "$output/service.elf" \
	| grep -E '[[:space:]](amo[a-z]+[.]w|lr[.]w|sc[.]w)[[:space:]]' >/dev/null; then
	echo 'resident worker uses an unsupported A-extension atomic' >&2
	exit 1
fi
shasum -a 256 "$output/boot.bin" "$output/text.bin"
