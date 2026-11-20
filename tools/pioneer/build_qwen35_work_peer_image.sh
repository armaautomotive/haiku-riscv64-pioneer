#!/bin/sh
# Build the exact Qwen3.5 checked persistent matrix peer image (offline only).
set -eu
[ "$#" = 2 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
cross=$(CDPATH= cd -- "$1/cross-tools-riscv64/bin" && pwd)/riscv64-unknown-haiku
output=$2
case "$output" in /*) ;; *) exit 2;; esac
[ ! -e "$output" ] && [ ! -L "$output" ] || exit 2
umask 077
mkdir -m 700 "$output"
optimization=${PIONEER_Q35_WORK_PEER_OPT_LEVEL:-s}
case "$optimization" in s|1|2|3) ;; *) exit 2;; esac
flags="-std=c++11 -O$optimization -Wall -Wextra -Werror -ffreestanding -fno-exceptions -fno-rtti -fno-stack-protector -fno-pic -fno-pie -fno-builtin -ffp-contract=off -ffunction-sections -fdata-sections -fno-unwind-tables -fno-asynchronous-unwind-tables -msmall-data-limit=0 -mno-relax -march=rv32imf_zicsr -mabi=ilp32 -fstack-usage"
batch_flags=
case "${PIONEER_Q35_WORK_BATCH_FETCH:-0}" in
	0) ;;
	1) batch_flags=-DPIONEER_Q35_WORK_BATCH_FETCH=1 ;;
	*) exit 2 ;;
esac
prefetch_flags=
case "${PIONEER_Q35_WORK_TILE_PREFETCH:-0}" in
	0) ;;
	1) prefetch_flags=-DPIONEER_Q35_WORK_TILE_PREFETCH=1 ;;
	*) exit 2 ;;
esac
"$cross-g++" $flags $batch_flags $prefetch_flags -DPIONEER_Q35_PROJECTION_IMAGE=1 -DPIONEER_Q35_WORK_PEER_IMAGE=1 \
 -c "$directory/blackhole_qwen38_persistent.cpp" -o "$output/peer.o"
"$cross-as" --defsym PIONEER_Q35_WORK_PEER_IMAGE=1 -march=rv32imf_zicsr -mabi=ilp32 -mno-relax \
 "$directory/blackhole_qwen38_persistent_start.S" -o "$output/start.o"
"$cross-ld" -m elf32lriscv --no-relax --gc-sections -T "$directory/blackhole_qwen38_block.ld" \
 "$output/start.o" "$output/peer.o" -Map "$output/peer.map" -o "$output/peer.elf"
"$cross-objcopy" -O binary -j .text "$output/peer.elf" "$output/text.bin"
"$cross-objcopy" -O binary -j .boot "$output/peer.elf" "$output/boot.bin"
"$cross-nm" -n "$output/peer.elf" > "$output/peer.symbols"
"$cross-objdump" -d "$output/peer.elf" > "$output/peer.disassembly"
python3 - "$output" <<'PY'
from pathlib import Path
from struct import iter_unpack, pack
import sys

out = Path(sys.argv[1])
boot = (out / 'boot.bin').read_bytes()
code = (out / 'text.bin').read_bytes()
symbols = {name: int(value, 16) for value, kind, name in
           (line.split() for line in (out / 'peer.symbols').read_text().splitlines())
           if kind == 'T'}
disassembly = (out / 'peer.disassembly').read_text().splitlines()
if boot != pack('<I', 0x0006c06f) or not code or len(code) > 0x2000 or len(code) % 4:
    raise SystemExit('unexpected persistent peer image size or boot jump')
for name, address in (('block_entry', 0x72000), ('block_peer_wait_load', 0x7204c),
                      ('block_peer_wait_branch', 0x72050), ('block_fault_park', 0x72080),
                      ('block_fault_park_fetch', 0x720a8)):
    if symbols.get(name) != address:
        raise SystemExit(f'unexpected persistent peer symbol {name}')
branches = {}
for line in disassembly:
    fields = line.split()
    if len(fields) >= 4 and fields[0].endswith(':'):
        branches[fields[0][:-1]] = fields[1:4]
if branches.get('720a4') != ['0000006f', 'j', '720a4'] \
        or branches.get('720a8') != ['ffdff06f', 'j', '720a4']:
    raise SystemExit('Work fault park must contain two exact local branch instructions')
words = ''.join(f'\t0x{word:08x}U,\n' for word, in iter_unpack('<I', code))
(out / 'PioneerQwen35WorkPeerImage.h').write_text(
    '#pragma once\nnamespace PioneerQwen35 {\n'
    'inline constexpr unsigned WorkPeerWords[]={\n' + words + '};\n}\n',
    encoding='ascii')
PY
shasum -a 256 "$output/boot.bin" "$output/text.bin"
