#!/bin/sh
# Compile only: the resident image is compact, while the qualification header is a placeholder.
# All artifacts are temporary and retain the hardware-denial guard; none are installed.
set -eu
[ "$#" = 3 ] || exit 2
build=$(CDPATH= cd -- "$1" && pwd)
images=$(CDPATH= cd -- "$2" && pwd)
compact=$(CDPATH= cd -- "$3" && pwd)
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
project=$(CDPATH= cd -- "$directory/../../.." && pwd)
sdk="$build/objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop"
cxx="$build/cross-tools-riscv64/bin/riscv64-unknown-haiku-g++"
objdump="$build/cross-tools-riscv64/bin/riscv64-unknown-haiku-objdump"
test -x "$cxx" && test -x "$objdump"
for header in \
 qwen35-resident-service-image/PioneerQwen35DataflowServiceImage.h \
 qwen35-resident-service-image/PioneerQwen35QualificationServiceImage.h \
 qwen35-dataflow-service-image/PioneerQwen35DataflowServiceImage.h \
 qwen35-shared-role-images/PioneerQwen35SharedRoleImages.h \
 qwen35-local-restore-images/PioneerQwen35LocalRestoreImages.h \
 qwen35-snapshot-service-restore-images/PioneerQwen35RoleServiceProbeImages.h; do
 test -f "$images/$header"
done
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q35-fused-driver.XXXXXXXX")
trap 'rm -rf "$temporary"' EXIT HUP INT TERM
python3 -B "$directory/pioneer_qwen35_compact_header_for_compile.py" "$compact" \
 "$temporary/PioneerQwen35DataflowServiceImage.h"
set -- -std=gnu++17 -O2 -Wall -Wextra -Werror -fno-exceptions -fno-rtti -ffreestanding \
 -fno-semantic-interposition -fno-tree-vectorize -fno-use-cxa-atexit -fno-strict-aliasing \
 -fno-delete-null-pointer-checks -mcmodel=medany -fpic -D_KERNEL_MODE \
 -DTT_PIONEER_EXPERIMENTAL_Q38_BANKS -DTT_PIONEER_EXPERIMENTAL_Q38_PANEL \
 -DTT_PIONEER_EXPERIMENTAL_Q38_AFFINE -DTT_PIONEER_EXPERIMENTAL_Q38_BLOCK \
 -DTT_PIONEER_EXPERIMENTAL_Q38_STREAM -DTT_PIONEER_EXPERIMENTAL_Q38_ROWS \
 -DTT_PIONEER_EXPERIMENTAL_Q38_COHORT -DTT_PIONEER_EXPERIMENTAL_Q38_COHORT_SEALED \
 -DTT_PIONEER_EXPERIMENTAL_Q38_COHORT_PAYLOAD -DTT_PIONEER_EXPERIMENTAL_Q38_DISPATCH \
 -DTT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_VECTOR -DTT_PIONEER_DRIVER_RELOAD \
 -DPIONEER_DRIVER_BUILD_TAG=0x2026112100010125ULL -DPIONEER_QWEN35_DATAFLOW_REWRITE \
 -DPIONEER_QWEN35_RECOVERY_COMPILE_ONLY -DPIONEER_QWEN35_RECOVERY_RUN_ENABLE \
 -DPIONEER_QWEN35_RETAINED_SCOPE_WITNESS -DPIONEER_QWEN35_RETAINED_GATEWAY_WITNESS \
 -DPIONEER_QWEN35_DEVICE_DISPATCH -DPIONEER_QWEN35_ROLE_WRITE_COMPILE_ONLY \
 -DPIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS -DPIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS \
 -DPIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS -DPIONEER_QWEN35_PAIRED_RECURRENCE \
 -DPIONEER_QWEN35_FUSED_RECURRENCE -DPIONEER_QWEN35_RESIDENT_SERVICE_WITNESS \
 -DPIONEER_QWEN35_ACTIVATION_SHARING_WITNESS -DPIONEER_QWEN35_DELTA_SHARD_COMPILE_ONLY \
 -DPIONEER_QWEN35_TREE_SETTLEMENT \
 -isystem "$project/headers/private/kernel" -isystem "$project/headers/private/kernel/arch/riscv64" \
 -isystem "$project/headers/private/kernel/arch/generic" -isystem "$project/headers/private" \
 -isystem "$project/headers/private/drivers" -isystem "$project/headers/private/system" \
 -isystem "$project/headers/private/system/arch/riscv64" -isystem "$project/build/config_headers" \
 -isystem "$project/headers/private/shared" -I"$build/objects/haiku/riscv64/release/system/kernel" \
 -I"$build/objects/haiku/riscv64/debug/system/kernel" -I"$sdk/headers" -idirafter "$sdk/headers/posix" \
 -I"$images/qwen35-resident-service-image" -I"$images/qwen35-dataflow-service-image" \
 -I"$images/qwen35-shared-role-images" -I"$images/qwen35-local-restore-images" \
 -I"$images/qwen35-snapshot-service-restore-images"
for include in "$sdk/headers/os" "$sdk"/headers/os/*; do
 [ ! -d "$include" ] || set -- "$@" -isystem "$include"
done
source="$project/src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp"
"$cxx" "$@" -fsyntax-only "$source"
"$cxx" -I"$temporary" "$@" -DPIONEER_QWEN35_COMPACT_DRIVER_COMPILE_ONLY \
 -DPIONEER_QWEN35_FLASH_DECODE_COMPILE_ONLY \
 -DPIONEER_QWEN35_FFN_PAIR_COMPILE_ONLY -DPIONEER_QWEN35_FUSED_RMS_SAVE_COMPILE_ONLY \
 -fstack-usage -c "$source" -o "$temporary/fused-driver.o"
"$objdump" -dr -j .text "$temporary/fused-driver.o" > "$temporary/text.disassembly"
python3 -B - "$temporary/text.disassembly" <<'PY'
from pathlib import Path
import re
import sys

text = Path(sys.argv[1]).read_text()
match = re.search(r'<init_hardware>:\n(.*?)(?:\n\s*\n|\n[0-9a-f]+ <)', text, re.S)
if not match:
    raise SystemExit('compile-only init_hardware was not found')
instructions = []
for line in match.group(1).splitlines():
    found = re.match(r'\s*[0-9a-f]+:\s+[0-9a-f]+\s+([a-z.]+)\s*(.*)', line)
    if found:
        instructions.append((found.group(1), found.group(2).split('#', 1)[0].replace(' ', '').strip()))
allowed = {('add', 'sp,sp,-16'), ('sd', 's0,8(sp)'), ('add', 's0,sp,16'),
           ('ld', 's0,8(sp)'), ('lui', 'a0,0x80000'), ('add', 'a0,a0,15'),
           ('add', 'sp,sp,16'), ('ret', '')}
if (not instructions or any(instruction not in allowed for instruction in instructions)
        or ('lui', 'a0,0x80000') not in instructions or ('add', 'a0,a0,15') not in instructions
        or instructions[-1] != ('ret', '')):
    raise SystemExit('compile-only init_hardware is not the expected no-I/O B_NOT_ALLOWED return')
print('PASS compile-only init_hardware returns B_NOT_ALLOWED without I/O')
PY
python3 -B - "$temporary/fused-driver.o" "$compact/text.bin" "$compact/dispatch.bin" <<'PY'
import hashlib
from pathlib import Path
import sys

obj = Path(sys.argv[1]).read_bytes()
for name in sys.argv[2:]:
    image = Path(name).read_bytes()
    if not image or obj.count(image) != 1:
        raise SystemExit('compact image missing or duplicated in driver object: ' + name)
    print(Path(name).name + ' SHA-256 ' + hashlib.sha256(image).hexdigest())
PY
"$cxx" -I"$temporary" "$@" -DPIONEER_QWEN35_COMPACT_DRIVER_COMPILE_ONLY \
 -DPIONEER_QWEN35_FLASH_DECODE_COMPILE_ONLY -DPIONEER_QWEN35_FFN_PAIR_COMPILE_ONLY \
 -DPIONEER_QWEN35_FUSED_RMS_SAVE_COMPILE_ONLY -c \
 "$project/src/add-ons/kernel/drivers/misc/PioneerPciProfile.cpp" -o "$temporary/profile.o"
release="$build/objects/haiku/riscv64/release"
kernel="$release/system/kernel/kernel_riscv64"
test -f "$kernel" && test -f "$release/system/glue/haiku_version_glue.o"
begin=$("$cxx" -print-file-name=crtbeginS.o)
end=$("$cxx" -print-file-name=crtendS.o)
libgcc=$("$cxx" -print-file-name=libgcc.a)
"$cxx" -shared -nostdlib -Wl,--no-undefined,-soname=tenstorrent_pci_probe \
 "$begin" "$release/system/glue/haiku_version_glue.o" \
 "$temporary/fused-driver.o" "$temporary/profile.o" "$kernel" "$libgcc" "$end" \
 -o "$temporary/tenstorrent_pci_probe-compile-only"
"$objdump" -dr -j .text "$temporary/tenstorrent_pci_probe-compile-only" > "$temporary/linked.disassembly"
python3 -B - "$temporary/text.disassembly" "$temporary/linked.disassembly" \
 "$temporary/tenstorrent_pci_probe-compile-only" "$compact/text.bin" "$compact/dispatch.bin" <<'PY'
from pathlib import Path
import re
import sys

def init_instructions(path):
    match = re.search(r'<init_hardware>:\n(.*?)(?:\n\s*\n|\n[0-9a-f]+ <)', Path(path).read_text(), re.S)
    if not match:
        raise SystemExit('linked compile-only init_hardware was not found')
    return [(found.group(1), found.group(2).split('#', 1)[0].replace(' ', '').strip())
            for line in match.group(1).splitlines()
            if (found := re.match(r'\s*[0-9a-f]+:\s+[0-9a-f]+\s+([a-z.]+)\s*(.*)', line))]

if init_instructions(sys.argv[1]) != init_instructions(sys.argv[2]):
    raise SystemExit('link changed the compile-only init_hardware guard')
linked = Path(sys.argv[3]).read_bytes()
for name in sys.argv[4:]:
    image = Path(name).read_bytes()
    if not image or linked.count(image) != 1:
        raise SystemExit('compact image missing or duplicated in linked driver: ' + name)
print('PASS linked driver retains the no-I/O init guard and exact compact images')
PY
echo 'PASS compact fused Haiku driver compile and link; qualification placeholder, NO INSTALL OR HARDWARE'
