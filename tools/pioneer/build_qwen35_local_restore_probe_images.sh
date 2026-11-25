#!/bin/sh
# Build a restricted, fixed-image local restore qualification probe. This
# does not create a loadable driver or authorize any hardware operation.
set -eu
[ "$#" = 2 ] || exit 2
[ "${PIONEER_QWEN35_RECOVERY_COMPILE_ONLY:-0}:${PIONEER_QWEN35_DEVICE_DISPATCH_COMPILE_ONLY:-0}:${PIONEER_QWEN35_LOCAL_RESTORE_PROBE_BUILD:-0}" = 1:1:1 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
build=$(CDPATH= cd -- "$1" && pwd)
output=$2
case "$output" in /*) ;; *) exit 2;; esac
[ ! -e "$output" ] && [ ! -L "$output" ] || exit 2
cross="$build/cross-tools-riscv64/bin/riscv64-unknown-haiku"
umask 077
mkdir -m 700 "$output"
sh "$directory/build_qwen35_local_restore_images.sh" "$build" "$output/local-restore"
sharing_asm=
if [ "${PIONEER_QWEN35_ACTIVATION_SHARING_COMPILE_ONLY:-0}" = 1 ]; then
    sharing_asm=-DPIONEER_QWEN35_ACTIVATION_SHARING_WITNESS
fi
"$cross-g++" -c -x assembler-with-cpp -mno-relax -march=rv32imf_zicsr -mabi=ilp32 \
    $sharing_asm -I"$directory" "$directory/blackhole_qwen35_local_restore_probe.S" -o "$output/probe.o"
for role in reader writer; do
    if [ "$role" = reader ]; then start=0x70000; limit=0x70100
    else start=0x70100; limit=0x70200; fi
    "$cross-ld" -m elf32lriscv --no-relax --gc-sections \
        --defsym ROLE_SHIM_ADDRESS="$start" --defsym ROLE_SHIM_LIMIT="$limit" \
        -e "qwen35_restore_probe_${role}_start" -T "$directory/blackhole_qwen35_local_restore_probe.ld" \
        "$output/probe.o" -o "$output/$role.elf"
    "$cross-nm" -u "$output/$role.elf" > "$output/$role.imports"
    test ! -s "$output/$role.imports"
    "$cross-nm" -n "$output/$role.elf" > "$output/$role.symbols"
    "$cross-objcopy" -O binary -j .shim "$output/$role.elf" "$output/$role-shim.bin"
done
python3 - "$output" <<'PY'
from pathlib import Path
from struct import iter_unpack
import os
import sys

out=Path(sys.argv[1])
sharing=os.environ.get('PIONEER_QWEN35_ACTIVATION_SHARING_COMPILE_ONLY')=='1'
writer_start,writer_limit=(0xb6000,0xb9000) if sharing else (0xb5000,0xb8000)
guard=('#if !defined(PIONEER_QWEN35_ACTIVATION_SHARING_WITNESS)\n' if sharing else '#if defined(PIONEER_QWEN35_ACTIVATION_SHARING_WITNESS)\n')+'#error Local restore probe layout mismatch\n#endif\n'
parts=['#pragma once\n'+guard+'namespace PioneerQwen35 {\n']
def signed(n,bits):
    return n-(1<<bits) if n&(1<<(bits-1)) else n
def assert_relocatable(data):
    # The exact restore bytes are moved from the shim slot to the same role's
    # code slot. Reject PC-relative address materialization, indirect jumps,
    # and any branch/jump outside these bytes so relocation stays local.
    for offset,(word,) in enumerate(iter_unpack('<I',data)):
        pc=offset*4;op=word&127
        if op in (0x17,0x67):
            raise SystemExit('restore image is not self-contained and position independent')
        if op==0x63:
            imm=(((word>>31)&1)<<12)|(((word>>7)&1)<<11)|(((word>>25)&63)<<5)|(((word>>8)&15)<<1)
            target=pc+signed(imm,13)
        elif op==0x6f:
            imm=(((word>>31)&1)<<20)|(((word>>12)&255)<<12)|(((word>>20)&1)<<11)|(((word>>21)&1023)<<1)
            target=pc+signed(imm,21)
        else:
            continue
        if target<0 or target>=len(data) or target%4:
            raise SystemExit('restore image control flow escapes its relocated code')
for role,start,limit,code_start,code_limit in (('reader',0x70000,0x70100,0xb0000,writer_start),
                                                ('writer',0x70100,0x70200,writer_start,writer_limit)):
    symbols={}
    for line in (out/f'{role}.symbols').read_text().splitlines():
        fields=line.split()
        if len(fields)==3 and fields[1] in ('T','t'):
            symbols[fields[2]]=int(fields[0],16)
    name=f'qwen35_restore_probe_{role}_start'
    shim=(out/f'{role}-shim.bin').read_bytes()
    code=(out/'local-restore'/f'{role}.bin').read_bytes()
    restore_symbols={}
    for line in (out/'local-restore'/f'{role}.symbols').read_text().splitlines():
        fields=line.split()
        if len(fields)==3 and fields[1] in ('T','t'):
            restore_symbols[fields[2]]=int(fields[0],16)
    original_name=f'qwen35_restore_{role}_start'
    original_park=restore_symbols.get(original_name+'_park',-1)
    if (symbols.get(name)!=start or not shim or len(shim)%4 or len(shim)>limit-start
            or not code or len(code)%4 or len(code)>code_limit-code_start
            or restore_symbols.get(original_name)!=start or original_park<start
            or original_park+4>start+len(code)):
        raise SystemExit(f'invalid {role} qualification image')
    assert_relocatable(code)
    cap=role.capitalize()
    parts.append(f'inline constexpr unsigned DataflowLocalProbe{cap}ParkAddress=0x{code_start+original_park-start:08x}U;\n')
    for kind,data in (('Shim',shim),('Code',code)):
        parts.append(f'inline constexpr unsigned DataflowLocalProbe{cap}{kind}Words[]={{\n')
        parts.extend(f'\t0x{word:08x}U,\n' for word, in iter_unpack('<I',data))
        parts.append('};\n')
parts.append('}\n')
(out/'PioneerQwen35LocalRestoreProbeImages.h').write_text(''.join(parts),encoding='ascii')
PY
shasum -a 256 "$output/reader-shim.bin" "$output/writer-shim.bin" \
    "$output/local-restore/reader.bin" "$output/local-restore/writer.bin" "$output/PioneerQwen35LocalRestoreProbeImages.h"
echo 'restricted local restore probe images are offline only; no hardware authority'
