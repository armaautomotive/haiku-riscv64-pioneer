#!/bin/sh
# Build distinct fixed-address local-RAM restore witnesses, OFFLINE ONLY.
# This does not provide a driver binding, ownership proof or reset operation.
set -eu
[ "$#" = 2 ] || exit 2
[ "${PIONEER_QWEN35_RECOVERY_COMPILE_ONLY:-0}:${PIONEER_QWEN35_DEVICE_DISPATCH_COMPILE_ONLY:-0}" = 1:1 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
build=$(CDPATH= cd -- "$1" && pwd)
output=$2
case "$output" in /*) ;; *) exit 2;; esac
[ ! -e "$output" ] && [ ! -L "$output" ] || exit 2
cross="$build/cross-tools-riscv64/bin/riscv64-unknown-haiku"
umask 077
mkdir -m 700 "$output"
"$cross-g++" -c -x assembler-with-cpp -mno-relax -march=rv32imf_zicsr -mabi=ilp32 \
    -I"$directory" "$directory/blackhole_qwen35_role_local_restore.S" -o "$output/restore.o"
for role in reader writer; do
    if [ "$role" = reader ]; then start=0x70000; limit=0x70100
    else start=0x70100; limit=0x70200; fi
    "$cross-ld" -m elf32lriscv --no-relax --gc-sections \
        --defsym ROLE_SHIM_ADDRESS="$start" --defsym ROLE_SHIM_LIMIT="$limit" \
        -e "qwen35_restore_${role}_start" -T "$directory/blackhole_qwen35_role_local_restore_witness.ld" \
        "$output/restore.o" -o "$output/$role.elf"
    "$cross-nm" -u "$output/$role.elf" > "$output/$role.imports"
    test ! -s "$output/$role.imports"
    "$cross-nm" -n "$output/$role.elf" > "$output/$role.symbols"
    "$cross-objcopy" -O binary -j .shim "$output/$role.elf" "$output/$role.bin"
done
python3 - "$output" <<'PY'
from pathlib import Path
from struct import iter_unpack
import sys
out=Path(sys.argv[1])
parts=['#pragma once\nnamespace PioneerQwen35 {\n']
for role,start,limit in (('reader',0x70000,0x70100),('writer',0x70100,0x70200)):
    symbols={}
    for line in (out/f'{role}.symbols').read_text().splitlines():
        fields=line.split()
        if len(fields)==3 and fields[1] in ('T','t'):
            symbols[fields[2]]=int(fields[0],16)
    name=f'qwen35_restore_{role}_start'
    park=symbols.get(name+'_park',-1)
    data=(out/f'{role}.bin').read_bytes()
    if symbols.get(name)!=start or not data or len(data)%4 or len(data)>limit-start:
        raise SystemExit(f'invalid {role} restore shim')
    offset=park-start
    if offset<0 or offset%4 or offset+4>len(data) or int.from_bytes(data[offset:offset+4],'little')!=0x6f:
        raise SystemExit(f'invalid {role} restore park')
    cap=role.capitalize()
    parts.append(f'inline constexpr unsigned DataflowLocalRestore{cap}ParkAddress=0x{park:08x}U;\n')
    parts.append(f'inline constexpr unsigned DataflowLocalRestore{cap}ShimWords[]={{\n')
    parts.extend(f'\t0x{word:08x}U,\n' for word, in iter_unpack('<I',data))
    parts.append('};\n')
parts.append('}\n')
(out/'PioneerQwen35LocalRestoreImages.h').write_text(''.join(parts),encoding='ascii')
PY
shasum -a 256 "$output/reader.bin" "$output/writer.bin" "$output/PioneerQwen35LocalRestoreImages.h"
echo 'local restore images are offline witnesses; do not install or run them'
