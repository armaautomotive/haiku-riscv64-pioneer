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
flags="-std=c++17 -Os -Wall -Wextra -Werror -ffreestanding -fno-exceptions -fno-rtti -fno-stack-protector -fno-pic -fno-pie -fno-builtin -ffp-contract=off -ffunction-sections -fdata-sections -fno-unwind-tables -fno-asynchronous-unwind-tables -fstack-usage -msmall-data-limit=0 -mno-relax -march=rv32imf_zicsr -mabi=ilp32"
service_flags=
if [ "${PIONEER_QWEN35_DEVICE_DISPATCH_COMPILE_ONLY:-0}" = 1 ]; then
	[ "${PIONEER_QWEN35_RECOVERY_COMPILE_ONLY:-0}" = 1 ] || exit 2
	service_flags=-DPIONEER_QWEN35_DEVICE_DISPATCH
fi

"$cross-g++" $flags $service_flags -I"$directory" -I"$sdk/headers" \
	-idirafter "$sdk/headers/posix" \
	-c "$directory/PioneerQwen35DataflowServiceImage.cpp" -o "$output/service.o"
"$cross-g++" $flags -I"$directory" -I"$sdk/headers" \
	-idirafter "$sdk/headers/posix" \
	-c "$directory/PioneerQwen35DataflowMemory.cpp" -o "$output/memory.o"
"$cross-as" -march=rv32imf_zicsr -mabi=ilp32 -mno-relax \
	"$directory/blackhole_qwen35_dataflow_service_start.S" -o "$output/start.o"
case "${PIONEER_QWEN35_DEVICE_DISPATCH_COMPILE_ONLY:-0}" in
0)
	"$cross-ld" -m elf32lriscv --no-relax --gc-sections \
		-T "$directory/blackhole_qwen35_dataflow_service.ld" \
		"$output/start.o" "$output/service.o" "$output/memory.o" \
		-Map "$output/service.map" -o "$output/service.elf";;
1)
	[ "${PIONEER_QWEN35_RECOVERY_COMPILE_ONLY:-0}" = 1 ] || exit 2
	"$cross-g++" $flags -O2 -I"$directory" -I"$sdk/headers" -idirafter "$sdk/headers/posix" \
		-c "$directory/tests/pioneer_qwen35_dataflow_device_stream_target_probe.cpp" -o "$output/device-stream.o"
	"$cross-g++" $flags -O2 -I"$directory" -I"$sdk/headers" -idirafter "$sdk/headers/posix" \
		-c "$directory/tests/pioneer_qwen35_dataflow_device_barrier_target_probe.cpp" -o "$output/device-barrier.o"
	"$cross-g++" $flags -O2 -I"$directory" -I"$sdk/headers" -idirafter "$sdk/headers/posix" \
		-c "$directory/tests/pioneer_qwen35_dataflow_device_graph_target_probe.cpp" -o "$output/device-graph.o"
	"$cross-ld" -m elf32lriscv --no-relax --gc-sections \
		-u PioneerQwen35DeviceGraphCompileProbe \
		-T "$directory/blackhole_qwen35_dataflow_dispatch_witness.ld" \
		"$output/start.o" "$output/service.o" "$output/memory.o" \
		"$output/device-stream.o" "$output/device-barrier.o" "$output/device-graph.o" \
		-Map "$output/service.map" -o "$output/service.elf"
	"$cross-objcopy" -O binary -j .dispatch "$output/service.elf" "$output/dispatch.bin";;
*) exit 2;;
esac
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
dispatch_path = out / 'dispatch.bin'
dispatch = dispatch_path.read_bytes() if dispatch_path.exists() else b''
if dispatch and (len(dispatch) > 0x5c00 or len(dispatch) % 4):
    raise SystemExit('unexpected dispatch witness image size')
dispatch_words = ''.join(f'\t0x{word:08x}U,\n' for word, in iter_unpack('<I', dispatch))
dispatch_definition = ('inline constexpr unsigned DataflowServiceDispatchWords[]={\n'
                       + dispatch_words + '};\n') if dispatch else ''
(out / 'PioneerQwen35DataflowServiceImage.h').write_text(
    '#pragma once\nnamespace PioneerQwen35 {\n'
    f'inline constexpr unsigned DataflowServiceImageBootWord=0x{unpack("<I", boot)[0]:08x}U;\n'
    'inline constexpr unsigned DataflowServiceTextWords[]={\n' + words + '};\n'
    + dispatch_definition + '}\n',
    encoding='ascii')
PY
if "$cross-objdump" -d "$output/service.elf" \
	| grep -E '[[:space:]](amo[a-z]+[.]w|lr[.]w|sc[.]w)[[:space:]]' >/dev/null; then
	echo 'resident worker uses an unsupported A-extension atomic' >&2
	exit 1
fi
shasum -a 256 "$output/boot.bin" "$output/text.bin"
if [ -f "$output/dispatch.bin" ]; then shasum -a 256 "$output/dispatch.bin"; fi
if [ -f "$output/device-graph.su" ]; then
python3 - "$output" <<'PY'
from pathlib import Path
import sys

out = Path(sys.argv[1])
entries = []
for name in ('service.su', 'device-graph.su'):
    for line in (out / name).read_text().splitlines():
        parts = line.rsplit('\t', 2)
        if len(parts) == 3 and parts[1].isdigit():
            entries.append((parts[0], int(parts[1]), parts[2]))
groups = (
    'qwen35_dataflow_service_main()',
    'PioneerQwen35DataflowBlackholeWorkerService()',
    'qwen35_graph_run_descriptor(',
    'PioneerQwen35DeviceGraphCompileProbe(',
    'DataflowDeviceGraph<IO, BarrierIO>::Tick()',
    'DataflowDynamicPatch(',
    'DataflowConvInvocation::Valid()',
    'DataflowDeviceBarrier<IO>::Poll()',
    'DataflowBlackholeNoc<Noc>::Transfer(',
    'qwen35_graph_execute(',
    'DataflowWorkerExecuteAuthorized(',
    '::PioneerQwen35DataflowBlackholeWorker(',
    'DataflowPackActivationCache(',
    'PioneerQ38ProjectionQuantize(',
)
total = 1024  # allowance for small nested helpers; not a whole-program proof
for marker in groups:
    matches = [(frame, kind) for name, frame, kind in entries if marker in name]
    if not matches or any(kind != 'static' for _, kind in matches):
        raise SystemExit('missing or unbounded worker stack frame: ' + marker)
    total += max(frame for frame, _ in matches)
if total > 8192:
    raise SystemExit(f'worker stack witness envelope {total} exceeds 8192 bytes')
print(f'qwen35_graph_worker_stack_envelope_bytes={total}/8192 '
      '(includes 1024-byte allowance; not a whole-program proof)')
PY
fi
