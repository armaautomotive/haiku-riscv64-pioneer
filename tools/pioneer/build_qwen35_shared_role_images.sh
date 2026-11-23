#!/bin/sh
# Build the fixed-address BRISC/NCRISC shared-role images, OFFLINE ONLY.
# This creates no driver, boot/reset operation, or hardware binding.
set -eu
[ "$#" = 2 ] || exit 2
[ "${PIONEER_QWEN35_RECOVERY_COMPILE_ONLY:-0}:${PIONEER_QWEN35_DEVICE_DISPATCH_COMPILE_ONLY:-0}" = 1:1 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
build=$(CDPATH= cd -- "$1" && pwd)
output=$2
case "$output" in /*) ;; *) exit 2;; esac
[ ! -e "$output" ] && [ ! -L "$output" ] || exit 2
cross="$build/cross-tools-riscv64/bin/riscv64-unknown-haiku"
sdk="$build/objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop"
umask 077
mkdir -m 700 "$output"
flags='-std=c++17 -O2 -Wall -Wextra -Werror -ffreestanding -fno-exceptions -fno-rtti -fno-stack-protector -fno-pic -fno-pie -fno-builtin -ffp-contract=off -ffunction-sections -fdata-sections -fno-unwind-tables -fno-asynchronous-unwind-tables -fstack-usage -msmall-data-limit=0 -mno-relax -march=rv32imf_zicsr -mabi=ilp32'
"$cross-g++" $flags -I"$directory" -I"$sdk/headers" -idirafter "$sdk/headers/posix" \
	-c "$directory/PioneerQwen35DataflowSharedRoles.cpp" -o "$output/shared-roles.o"
"$cross-g++" -c -x assembler-with-cpp -mno-relax -march=rv32imf_zicsr -mabi=ilp32 \
	-I"$directory" "$directory/blackhole_qwen35_shared_role_start.S" -o "$output/shared-start.o"
for role in reader writer; do
	if [ "$role" = reader ]; then shim=0x70000; shim_limit=0x70100; start=0xb0000; limit=0xb5000
	else shim=0x70100; shim_limit=0x70200; start=0xb5000; limit=0xb8000; fi
	"$cross-ld" -m elf32lriscv --no-relax --gc-sections \
		--defsym ROLE_SHIM_ADDRESS=$shim --defsym ROLE_SHIM_LIMIT=$shim_limit \
		--defsym ROLE_ADDRESS=$start --defsym ROLE_LIMIT=$limit \
		-e "qwen35_shared_${role}_start" -T "$directory/blackhole_qwen35_shared_role_witness.ld" \
		"$output/shared-start.o" "$output/shared-roles.o" -o "$output/$role.elf"
	"$cross-nm" -u "$output/$role.elf" > "$output/$role.imports"
	test ! -s "$output/$role.imports"
	"$cross-nm" -n "$output/$role.elf" > "$output/$role.symbols"
	"$cross-objdump" -d "$output/$role.elf" > "$output/$role.disassembly"
	grep -E '[[:space:]]csrs[[:space:]]+0x7c0,8' "$output/$role.disassembly" >/dev/null
	grep -E '[[:space:]]csrr[[:space:]]+[^,]+,0x7c0' "$output/$role.disassembly" >/dev/null
	if grep -E '[[:space:]](amo[a-z]+[.]w|lr[.]w|sc[.]w|((fdiv|fsqrt)[.][sd])|fcvt[.](w|wu)[.][sd])[[:space:]]' \
		"$output/$role.disassembly" >/dev/null; then
		echo "unsupported instruction in $role image" >&2
		exit 1
	fi
	"$cross-objcopy" -O binary -j .shim "$output/$role.elf" "$output/$role-shim.bin"
	"$cross-objcopy" -O binary -j .text "$output/$role.elf" "$output/$role-code.bin"
done
python3 - "$output" <<'PY'
from pathlib import Path
from struct import iter_unpack
import sys

out=Path(sys.argv[1])
arrays=[]
parks={}
for role,shim_start,shim_limit,code_start,code_limit in (
        ('reader',0x70000,0x70100,0xb0000,0xb5000),
        ('writer',0x70100,0x70200,0xb5000,0xb8000)):
    symbols={}
    for line in (out/f'{role}.symbols').read_text().splitlines():
        parts=line.split()
        if len(parts)==3 and parts[1] in ('T','t'):
            symbols[parts[2]]=int(parts[0],16)
    start=symbols.get(f'qwen35_shared_{role}_start')
    park=symbols.get(f'qwen35_shared_{role}_start_park',-1)
    tick=symbols.get(f'qwen35_shared_{role}_tick',-1)
    if start!=shim_start or not shim_start<=park<shim_limit or not code_start<=tick<code_limit:
        raise SystemExit(f'invalid {role} entry, park or tick address')
    shim=(out/f'{role}-shim.bin').read_bytes()
    park_offset=park-shim_start
    if park_offset%4 or park_offset+4>len(shim) or int.from_bytes(shim[park_offset:park_offset+4],'little')!=0x0000006f:
        raise SystemExit(f'{role} park is not an aligned self-loop')
    parks[role]=park
for role,kind,limit in (('reader','shim',0x100),('reader','code',0x5000),
                        ('writer','shim',0x100),('writer','code',0x3000)):
    data=(out/f'{role}-{kind}.bin').read_bytes()
    if not data or len(data)>limit or len(data)%4:
        raise SystemExit(f'invalid {role} {kind} image size')
    name=f'DataflowShared{role.capitalize()}{kind.capitalize()}Words'
    words=''.join(f'\t0x{word:08x}U,\n' for word, in iter_unpack('<I',data))
    arrays.append(f'inline constexpr unsigned {name}[]={{\n{words}}};\n')
(out/'PioneerQwen35SharedRoleImages.h').write_text(
    '#pragma once\nnamespace PioneerQwen35 {\n'
    +''.join(f'inline constexpr unsigned DataflowShared{role.capitalize()}ParkAddress=0x{park:08x}U;\n'
        for role,park in parks.items())+''.join(arrays)+'}\n',encoding='ascii')
entries=[]
for line in (out/'shared-roles.su').read_text().splitlines():
    fields=line.rsplit('\t',2)
    if len(fields)!=3 or not fields[1].isdigit() or fields[2]!='static':
        raise SystemExit('missing or non-static shared role stack frame')
    entries.append(int(fields[1]))
total=sum(entries)+16+1024
if not entries or total>8192:
    raise SystemExit(f'invalid shared role frame-sum witness {total}/8192')
print(f'shared_role_frame_sum_bytes={total}/8192 (not a whole-program proof)')
PY
# Build distinct entry images for the dedicated-device takeover lifecycle.
# They share role code, but cannot be substituted into the snapshot owner:
# no original private RAM is captured, and there is no snapshot handoff.
"$cross-g++" -c -x assembler-with-cpp -mno-relax -march=rv32imf_zicsr -mabi=ilp32 \
	-I"$directory" "$directory/blackhole_qwen35_exclusive_role_start.S" -o "$output/exclusive-start.o"
for role in reader writer; do
	if [ "$role" = reader ]; then shim=0x70000; shim_limit=0x70100; start=0xb0000; limit=0xb5000
	else shim=0x70100; shim_limit=0x70200; start=0xb5000; limit=0xb8000; fi
	"$cross-ld" -m elf32lriscv --no-relax --gc-sections \
		--defsym ROLE_SHIM_ADDRESS=$shim --defsym ROLE_SHIM_LIMIT=$shim_limit \
		--defsym ROLE_ADDRESS=$start --defsym ROLE_LIMIT=$limit \
		-e "qwen35_shared_${role}_start" -T "$directory/blackhole_qwen35_shared_role_witness.ld" \
		"$output/exclusive-start.o" "$output/shared-roles.o" -o "$output/exclusive-$role.elf"
	"$cross-nm" -u "$output/exclusive-$role.elf" > "$output/exclusive-$role.imports"
	test ! -s "$output/exclusive-$role.imports"
	"$cross-nm" -n "$output/exclusive-$role.elf" > "$output/exclusive-$role.symbols"
	"$cross-objcopy" -O binary -j .shim "$output/exclusive-$role.elf" "$output/exclusive-$role-shim.bin"
	"$cross-objcopy" -O binary -j .text "$output/exclusive-$role.elf" "$output/exclusive-$role-code.bin"
	cmp "$output/exclusive-$role-code.bin" "$output/$role-code.bin"
done
python3 - "$output" <<'PYEXCLUSIVE'
from pathlib import Path
from struct import iter_unpack
import sys

out=Path(sys.argv[1])
parts=['#pragma once\nnamespace PioneerQwen35 {\n']
for role,base,limit,code_start,code_limit in (
        ('reader',0x70000,0x70100,0xb0000,0xb5000),
        ('writer',0x70100,0x70200,0xb5000,0xb8000)):
    symbols={}
    for line in (out/f'exclusive-{role}.symbols').read_text().splitlines():
        fields=line.split()
        if len(fields)==3 and fields[1] in ('T','t'):
            symbols[fields[2]]=int(fields[0],16)
    entry=symbols.get(f'qwen35_shared_{role}_start')
    park=symbols.get(f'qwen35_shared_{role}_start_park',-1)
    tick=symbols.get(f'qwen35_shared_{role}_tick',-1)
    shim=(out/f'exclusive-{role}-shim.bin').read_bytes()
    if entry!=base or not base<=park<limit or not code_start<=tick<code_limit \
            or not shim or len(shim)>limit-base or len(shim)%4:
        raise SystemExit(f'invalid exclusive {role} entry, park, tick or shim length')
    offset=park-base
    if offset%4 or offset+4>len(shim) or int.from_bytes(shim[offset:offset+4],'little')!=0x6f:
        raise SystemExit(f'invalid exclusive {role} park loop')
    if shim==(out/f'{role}-shim.bin').read_bytes():
        raise SystemExit(f'exclusive {role} unexpectedly uses snapshot entry')
    cap=role.capitalize()
    parts.append(f'inline constexpr unsigned DataflowExclusive{cap}ParkAddress=0x{park:08x}U;\n')
    parts.append(f'inline constexpr unsigned DataflowExclusive{cap}ShimWords[]={{\n')
    parts.extend(f'\t0x{word:08x}U,\n' for word, in iter_unpack('<I',shim))
    parts.append('};\n')
parts.append('}\n')
(out/'PioneerQwen35ExclusiveRoleImages.h').write_text(''.join(parts),encoding='ascii')
PYEXCLUSIVE
shasum -a 256 "$output/reader-shim.bin" "$output/reader-code.bin" \
	"$output/writer-shim.bin" "$output/writer-code.bin" "$output/PioneerQwen35SharedRoleImages.h" \
	"$output/exclusive-reader-shim.bin" "$output/exclusive-writer-shim.bin" \
	"$output/PioneerQwen35ExclusiveRoleImages.h"
echo 'shared role images are offline witnesses; do not install or run them'
