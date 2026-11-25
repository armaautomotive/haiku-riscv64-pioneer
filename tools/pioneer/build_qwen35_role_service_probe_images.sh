#!/bin/sh
# Restricted startup/idle-stop/restore qualification images; offline only.
set -eu
[ "$#" = 2 ] || exit 2
[ "${PIONEER_QWEN35_RECOVERY_COMPILE_ONLY:-0}:${PIONEER_QWEN35_DEVICE_DISPATCH_COMPILE_ONLY:-0}:${PIONEER_QWEN35_ROLE_SERVICE_PROBE_BUILD:-0}" = 1:1:1 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
build=$(CDPATH= cd -- "$1" && pwd)
output=$2
case "$output" in /*) ;; *) exit 2;; esac
[ ! -e "$output" ] && [ ! -L "$output" ] || exit 2
cross="$build/cross-tools-riscv64/bin/riscv64-unknown-haiku"
umask 077
mkdir -m 700 "$output"
sh "$directory/build_qwen35_shared_role_images.sh" "$build" "$output/shared"
PIONEER_QWEN35_LOCAL_RESTORE_PROBE_BUILD=1 sh "$directory/build_qwen35_local_restore_probe_images.sh" "$build" "$output/local-probe"
sharing_asm=; writer_start=0xb5000; reader_restore=0xb4e00; writer_restore=0xb7e00
if [ "${PIONEER_QWEN35_ACTIVATION_SHARING_COMPILE_ONLY:-0}" = 1 ]; then
    sharing_asm=-DPIONEER_QWEN35_ACTIVATION_SHARING_WITNESS
    writer_start=0xb6000; reader_restore=0xb5e00; writer_restore=0xb8e00
fi
"$cross-g++" -c -x assembler-with-cpp -mno-relax -march=rv32imf_zicsr -mabi=ilp32 \
    -DPIONEER_QWEN35_ROLE_SERVICE_PROBE=1 $sharing_asm -I"$directory" \
    "$directory/blackhole_qwen35_shared_role_start.S" -o "$output/service-shim.o"
for role in reader writer; do
    if [ "$role" = reader ]; then shim=0x70000; shim_limit=0x70100; start=0xb0000; limit=$reader_restore
    else shim=0x70100; shim_limit=0x70200; start=$writer_start; limit=$writer_restore; fi
    set -- "$output/service-shim.o" "$output/shared/shared-roles.o"
    if [ "${PIONEER_QWEN35_ACTIVATION_SHARING_COMPILE_ONLY:-0}" = 1 ]; then set -- "$@" "$output/shared/memory.o"; fi
    "$cross-ld" -m elf32lriscv --no-relax --gc-sections \
        --defsym ROLE_SHIM_ADDRESS="$shim" --defsym ROLE_SHIM_LIMIT="$shim_limit" \
        --defsym ROLE_ADDRESS="$start" --defsym ROLE_LIMIT="$limit" \
        -e "qwen35_shared_${role}_start" -T "$directory/blackhole_qwen35_shared_role_witness.ld" \
        "$@" -o "$output/$role.elf"
    "$cross-nm" -u "$output/$role.elf" > "$output/$role.imports"
    test ! -s "$output/$role.imports"
    "$cross-nm" -n "$output/$role.elf" > "$output/$role.symbols"
    "$cross-objcopy" -O binary -j .shim "$output/$role.elf" "$output/$role-shim.bin"
    "$cross-objcopy" -O binary -j .text "$output/$role.elf" "$output/$role-service.bin"
    # The qualification must use the exact resident role code, not a mock.
    cmp "$output/$role-service.bin" "$output/shared/$role-code.bin"
done
python3 - "$output" <<'PY'
from pathlib import Path
from struct import iter_unpack
import os
import sys

out=Path(sys.argv[1])
sharing=os.environ.get('PIONEER_QWEN35_ACTIVATION_SHARING_COMPILE_ONLY')=='1'
writer_start,writer_limit,reader_restore,writer_restore=(0xb6000,0xb9000,0xb5e00,0xb8e00) if sharing else (0xb5000,0xb8000,0xb4e00,0xb7e00)
guard=('#if !defined(PIONEER_QWEN35_ACTIVATION_SHARING_WITNESS)\n' if sharing else '#if defined(PIONEER_QWEN35_ACTIVATION_SHARING_WITNESS)\n')+'#error Role service restore image layout mismatch\n#endif\n'
parts=['#pragma once\n'+guard+'namespace PioneerQwen35 {\n']
for role,shim,shim_end,start,restore,limit in (
    ('reader',0x70000,0x70100,0xb0000,reader_restore,writer_start),
    ('writer',0x70100,0x70200,writer_start,writer_restore,writer_limit)):
    startup=(out/f'{role}-shim.bin').read_bytes()
    service=(out/f'{role}-service.bin').read_bytes()
    local_restore=(out/'local-probe'/'local-restore'/f'{role}.bin').read_bytes()
    # The local-probe builder has independently rejected PC-relative control
    # flow that would escape the relocated restore image.
    if (not startup or len(startup)%4 or len(startup)>shim_end-shim
            or not service or len(service)%4 or start+len(service)>restore
            or not local_restore or len(local_restore)%4 or restore+len(local_restore)>limit):
        raise SystemExit(f'invalid {role} service/restore placement')
    shim_symbols={}
    for line in (out/f'{role}.symbols').read_text().splitlines():
        fields=line.split()
        if len(fields)==3 and fields[1] in ('T','t'):
            shim_symbols[fields[2]]=int(fields[0],16)
    start_name=f'qwen35_shared_{role}_start'
    shim_park=shim_symbols.get(start_name+'_park',-1)
    loop=shim_symbols.get(start_name+'_service_loop',-1)
    loop_end=shim_symbols.get(start_name+'_service_loop_end',-1)
    if (shim_symbols.get(start_name)!=shim or shim_park<shim or shim_park+4>shim+len(startup)
            or not shim<=loop<loop_end<=shim_park or (loop|loop_end)&3
            or int.from_bytes(startup[shim_park-shim:shim_park-shim+4],'little')!=0x0000006f):
        raise SystemExit(f'invalid {role} service shim entry, loop or park')
    symbols={}
    for line in (out/'local-probe'/'local-restore'/f'{role}.symbols').read_text().splitlines():
        fields=line.split()
        if len(fields)==3 and fields[1] in ('T','t'):
            symbols[fields[2]]=int(fields[0],16)
    original=f'qwen35_restore_{role}_start'
    original_park=symbols.get(original+'_park',-1)
    if symbols.get(original)!=shim or not shim<=original_park<shim+len(local_restore):
        raise SystemExit(f'invalid {role} restore park symbol')
    code=service+bytes(restore-start-len(service))+local_restore
    cap=role.capitalize()
    parts.append(f'inline constexpr unsigned DataflowServiceProbe{cap}ShimParkAddress=0x{shim_park:08x}U;\n')
    parts.append(f'inline constexpr unsigned DataflowServiceProbe{cap}LoopAddress=0x{loop:08x}U;\n')
    parts.append(f'inline constexpr unsigned DataflowServiceProbe{cap}LoopEnd=0x{loop_end:08x}U;\n')
    parts.append(f'inline constexpr unsigned DataflowServiceProbe{cap}RuntimeCodeWords={len(service)//4}U;\n')
    parts.append(f'inline constexpr unsigned DataflowServiceProbe{cap}RestoreAddress=0x{restore:08x}U;\n')
    parts.append(f'inline constexpr unsigned DataflowServiceProbe{cap}ParkAddress=0x{restore+original_park-shim:08x}U;\n')
    for kind,data in (('Shim',startup),('Code',code)):
        parts.append(f'inline constexpr unsigned DataflowServiceProbe{cap}{kind}Words[]={{\n')
        parts.extend(f'\t0x{word:08x}U,\n' for word, in iter_unpack('<I',data))
        parts.append('};\n')
    (out/f'{role}-composite.bin').write_bytes(code)
parts.append('}\n')
(out/'PioneerQwen35RoleServiceProbeImages.h').write_text(''.join(parts),encoding='ascii')
PY
shasum -a 256 "$output/reader-shim.bin" "$output/writer-shim.bin" "$output/reader-composite.bin" "$output/writer-composite.bin" "$output/PioneerQwen35RoleServiceProbeImages.h"
echo 'restricted service probe images are offline only; no hardware authority'
