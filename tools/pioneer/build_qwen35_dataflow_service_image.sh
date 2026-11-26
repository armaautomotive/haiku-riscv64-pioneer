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
dispatch=0
case "${PIONEER_QWEN35_DEVICE_DISPATCH_COMPILE_ONLY:-0}:${PIONEER_QWEN35_DEVICE_DISPATCH_TRIAL:-0}:${PIONEER_QWEN35_RECOVERY_COMPILE_ONLY:-0}" in
	0:0:0|0:0:1) ;;
	1:0:1|0:1:0) dispatch=1; service_flags=-DPIONEER_QWEN35_DEVICE_DISPATCH;;
	*) exit 2;;
esac
role_graph=0
case "${PIONEER_QWEN35_SHARED_ROLE_GRAPH_COMPILE_ONLY:-0}" in
0) ;;
1) [ "${PIONEER_QWEN35_DEVICE_DISPATCH_COMPILE_ONLY:-0}:${PIONEER_QWEN35_DEVICE_DISPATCH_TRIAL:-0}:${PIONEER_QWEN35_RECOVERY_COMPILE_ONLY:-0}" = 1:0:1 ] || exit 2
	role_graph=1
	service_flags="$service_flags -DPIONEER_QWEN35_RECOVERY_COMPILE_ONLY -DPIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS -DPIONEER_QWEN35_PAIRED_RECURRENCE";;
*) exit 2;;
esac
case "${PIONEER_QWEN35_FUSED_RECURRENCE_COMPILE_ONLY:-0}" in
0) ;;
1) [ "$role_graph" = 1 ] || exit 2
	service_flags="$service_flags -DPIONEER_QWEN35_FUSED_RECURRENCE"
	: > "$output/fused-recurrence.marker" ;;
*) exit 2 ;;
esac
case "${PIONEER_QWEN35_ACTIVATION_SHARING_COMPILE_ONLY:-0}" in
0) ;;
1) [ "$role_graph:${PIONEER_QWEN35_RESIDENT_SERVICE_COMPILE_ONLY:-0}" = 1:1 ] || exit 2
	service_flags="$service_flags -DPIONEER_QWEN35_ACTIVATION_SHARING_WITNESS"
	: > "$output/activation-sharing.marker" ;;
*) exit 2 ;;
esac
case "${PIONEER_QWEN35_DELTA_SHARD_COMPILE_ONLY:-0}" in
0) ;;
1) [ "$role_graph:${PIONEER_QWEN35_FUSED_RECURRENCE_COMPILE_ONLY:-0}:${PIONEER_QWEN35_ACTIVATION_SHARING_COMPILE_ONLY:-0}" = 1:1:1 ] || exit 2
	service_flags="$service_flags -DPIONEER_QWEN35_DELTA_SHARD_COMPILE_ONLY"
	: > "$output/delta-shard.marker" ;;
*) exit 2 ;;
esac
case "${PIONEER_QWEN35_DENSE_SHARD_COMPILE_ONLY:-0}" in
0) ;;
1) [ "$role_graph:${PIONEER_QWEN35_DELTA_SHARD_COMPILE_ONLY:-0}" = 1:1 ] || exit 2
	service_flags="$service_flags -DPIONEER_QWEN35_DENSE_SHARD_COMPILE_ONLY"
	: > "$output/dense-shard.marker" ;;
*) exit 2 ;;
esac
case "${PIONEER_QWEN35_FLASH_DECODE_COMPILE_ONLY:-0}" in
0) ;;
1) [ "$role_graph:${PIONEER_QWEN35_DELTA_SHARD_COMPILE_ONLY:-0}:${PIONEER_QWEN35_DENSE_SHARD_COMPILE_ONLY:-0}:${PIONEER_QWEN35_TTSIM_UNPROFILED_COMPILE_ONLY:-0}" = 1:1:0:1 ] || exit 2
	service_flags="$service_flags -DPIONEER_QWEN35_FLASH_DECODE_COMPILE_ONLY"
	: > "$output/flash-hybrid.marker" ;;
*) exit 2 ;;
esac
case "${PIONEER_QWEN35_FFN_PAIR_COMPILE_ONLY:-0}" in
0) ;;
1) [ "${PIONEER_QWEN35_FLASH_DECODE_COMPILE_ONLY:-0}:${PIONEER_QWEN35_ACTIVATION_SHARING_COMPILE_ONLY:-0}" = 1:1 ] || exit 2
	service_flags="$service_flags -DPIONEER_QWEN35_FFN_PAIR_COMPILE_ONLY"
	: > "$output/ffn-pair.marker" ;;
*) exit 2 ;;
esac
case "${PIONEER_QWEN35_FUSED_RMS_SAVE_COMPILE_ONLY:-0}" in
0) ;;
1) [ "${PIONEER_QWEN35_FFN_PAIR_COMPILE_ONLY:-0}:${PIONEER_QWEN35_FLASH_DECODE_COMPILE_ONLY:-0}" = 1:1 ] || exit 2
	service_flags="$service_flags -DPIONEER_QWEN35_FUSED_RMS_SAVE_COMPILE_ONLY"
	: > "$output/fused-rms-save.marker" ;;
*) exit 2 ;;
esac
case "${PIONEER_QWEN35_TTSIM_UNPROFILED_COMPILE_ONLY:-0}" in
0) ;;
1) [ "$role_graph:${PIONEER_QWEN35_DELTA_SHARD_COMPILE_ONLY:-0}" = 1:1 ] || exit 2
	service_flags="$service_flags -DPIONEER_QWEN35_TTSIM_UNPROFILED_COMPILE_ONLY"
	: > "$output/ttsim-unprofiled.marker" ;;
*) exit 2 ;;
esac

"$cross-g++" $flags $service_flags -I"$directory" -I"$sdk/headers" \
	-idirafter "$sdk/headers/posix" \
	-c "$directory/PioneerQwen35DataflowServiceImage.cpp" -o "$output/service.o"
"$cross-g++" $flags -I"$directory" -I"$sdk/headers" \
	-idirafter "$sdk/headers/posix" \
	-c "$directory/PioneerQwen35DataflowMemory.cpp" -o "$output/memory.o"
"$cross-as" -march=rv32imf_zicsr -mabi=ilp32 -mno-relax \
	"$directory/blackhole_qwen35_dataflow_service_start.S" -o "$output/start.o"
case "$dispatch" in
0)
	"$cross-ld" -m elf32lriscv --no-relax --gc-sections \
		-T "$directory/blackhole_qwen35_dataflow_service.ld" \
		"$output/start.o" "$output/service.o" "$output/memory.o" \
		-Map "$output/service.map" -o "$output/service.elf";;
1)
	graph_flags=
	if [ "$role_graph" = 1 ]; then graph_flags=$service_flags; fi
	"$cross-g++" $flags -O2 $graph_flags -I"$directory" -I"$sdk/headers" -idirafter "$sdk/headers/posix" \
		-c "$directory/tests/pioneer_qwen35_dataflow_device_stream_target_probe.cpp" -o "$output/device-stream.o"
	"$cross-g++" $flags -O2 $graph_flags -I"$directory" -I"$sdk/headers" -idirafter "$sdk/headers/posix" \
		-c "$directory/tests/pioneer_qwen35_dataflow_device_barrier_target_probe.cpp" -o "$output/device-barrier.o"
	# Type-check the new resident token fanout against the actual RV32 NoC
	# transport. It is not linked into the service until lifecycle integration.
	"$cross-g++" $flags -O2 $graph_flags -I"$directory" -I"$sdk/headers" -idirafter "$sdk/headers/posix" \
		-c "$directory/tests/pioneer_qwen35_resident_dispatch_target_probe.cpp" -o "$output/resident-dispatch-probe.o"
	"$cross-g++" $flags -O2 $graph_flags -I"$directory" -I"$sdk/headers" -idirafter "$sdk/headers/posix" \
		-c "$directory/tests/pioneer_qwen35_resident_lifecycle_target_probe.cpp" -o "$output/resident-lifecycle-probe.o"
	"$cross-g++" $flags -O2 $graph_flags -I"$directory" -I"$sdk/headers" -idirafter "$sdk/headers/posix" \
		-c "$directory/PioneerQwen35DataflowDeviceDispatch.cpp" -o "$output/device-graph.o"
	if [ "$role_graph" = 1 ]; then
		"$cross-g++" $flags $graph_flags -I"$directory" -I"$sdk/headers" -idirafter "$sdk/headers/posix" \
			-c "$directory/PioneerQwen35DataflowSharedRoles.cpp" -o "$output/shared-roles.o"
		linker_script="$directory/blackhole_qwen35_dataflow_dispatch_witness.ld"
		if [ "${PIONEER_QWEN35_FLASH_DECODE_COMPILE_ONLY:-0}" = 1 ]; then
			linker_script="$directory/blackhole_qwen35_flash_dataflow_dispatch_ttsim.ld"
		fi
		if [ "${PIONEER_QWEN35_FUSED_RMS_SAVE_COMPILE_ONLY:-0}" = 1 ]; then
			linker_script="$directory/blackhole_qwen35_rms_save_dataflow_dispatch_ttsim.ld"
		fi
		"$cross-ld" -m elf32lriscv --no-relax --gc-sections \
			-u PioneerQwen35DeviceGraphRun \
			-T "$linker_script" \
			"$output/start.o" "$output/service.o" "$output/memory.o" \
			"$output/device-stream.o" "$output/device-barrier.o" "$output/device-graph.o" \
			"$output/shared-roles.o" -Map "$output/service.map" -o "$output/service.elf"
	else
		"$cross-ld" -m elf32lriscv --no-relax --gc-sections \
		-u PioneerQwen35DeviceGraphRun \
		-T "$directory/blackhole_qwen35_dataflow_dispatch_witness.ld" \
		"$output/start.o" "$output/service.o" "$output/memory.o" \
		"$output/device-stream.o" "$output/device-barrier.o" "$output/device-graph.o" \
		-Map "$output/service.map" -o "$output/service.elf"
	fi
	"$cross-objcopy" -O binary -j .dispatch "$output/service.elf" "$output/dispatch.bin";;
*) exit 2;;
esac
if [ "${PIONEER_QWEN35_FLASH_DECODE_COMPILE_ONLY:-0}" = 1 ]; then
	"$cross-objcopy" -O binary -j .flash "$output/service.elf" "$output/flash.bin"
fi
"$cross-objcopy" -O binary -j .boot "$output/service.elf" "$output/boot.bin"
"$cross-objcopy" -O binary -j .text "$output/service.elf" "$output/text.bin"
"$cross-nm" -u "$output/service.elf" > "$output/imports.txt"
test ! -s "$output/imports.txt"
"$cross-nm" -n "$output/service.elf" > "$output/symbols.txt"
"$cross-objdump" -d "$output/service.elf" > "$output/disassembly.txt"
if [ "$role_graph" = 1 ]; then
	grep -E 'jalr.*<qwen35_shared_compute_coordinate>' "$output/disassembly.txt" >/dev/null
	if "$cross-nm" -C "$output/service.elf" | grep -F 'PioneerQwen35DataflowBlackholeWorker(' >/dev/null; then
		echo 'shared role graph unexpectedly contains the single-controller projection' >&2
		exit 1
	fi
fi
python3 - "$output" "$role_graph" "${PIONEER_QWEN35_FUSED_RMS_SAVE_COMPILE_ONLY:-0}" <<'PY'
from pathlib import Path
from struct import iter_unpack, unpack
import sys

out = Path(sys.argv[1])
role_graph = sys.argv[2] == '1'
fused_rms_save = sys.argv[3] == '1'
boot = (out / 'boot.bin').read_bytes()
code = (out / 'text.bin').read_bytes()
symbol_rows = [line.split() for line in (out / 'symbols.txt').read_text().splitlines()]
symbols = {name: int(value, 16) for value, kind, name in symbol_rows if kind == 'T'}
if boot != bytes.fromhex('6fc00600') or not 0 < len(code) <= 0xe000 or len(code) % 4:
    raise SystemExit('unexpected resident dataflow image size or boot jump')
if symbols.get('qwen35_dataflow_entry') != 0x72000:
    raise SystemExit('unexpected resident dataflow entry address')
park_definition = ''
if role_graph:
    park = symbols.get('qwen35_dataflow_stopped_park', 0)
    offset = park - 0x72000
    if offset < 0 or offset % 4 or offset + 4 > len(code) or code[offset:offset+4] != bytes.fromhex('6f000000'):
        raise SystemExit('unexpected shared compute stopped park')
    park_definition = f'inline constexpr unsigned DataflowServiceStoppedParkAddress=0x{park:08x}U;\n'
words = ''.join(f'\t0x{word:08x}U,\n' for word, in iter_unpack('<I', code))
dispatch_path = out / 'dispatch.bin'
dispatch = dispatch_path.read_bytes() if dispatch_path.exists() else b''
if dispatch and (len(dispatch) > 0x5c00 or len(dispatch) % 4):
    raise SystemExit('unexpected dispatch witness image size')
if dispatch:
    rms = [int(value, 16) for value, kind, name in symbol_rows
           if kind in ('T', 'W') and 'PioneerQwen35DataflowRmsWorker' in name
           and 'DataflowRmsSaveAdapter' not in name]
    bridge = symbols.get('qwen35_graph_execute', 0)
    if len(rms) != 1 or not all(0xa0400 <= address < 0xa0400 + len(dispatch)
                                for address in (rms[0], bridge)):
        raise SystemExit('RMS worker or graph bridge is outside the verified dispatch image')
    if fused_rms_save:
        fused = [int(value, 16) for value, kind, name in symbol_rows
                 if kind in ('T', 'W') and 'PioneerQwen35DataflowRmsWorker' in name
                 and 'DataflowRmsSaveAdapter' in name]
        flash = (out / 'flash.bin').read_bytes()
        if len(fused) != 1 or not 0xd0000 <= fused[0] < 0xd0000 + len(flash):
            raise SystemExit('fused RMS worker is outside the simulator flash image')
dispatch_words = ''.join(f'\t0x{word:08x}U,\n' for word, in iter_unpack('<I', dispatch))
dispatch_definition = ('inline constexpr unsigned DataflowServiceDispatchWords[]={\n'
                       + dispatch_words + '};\n') if dispatch else ''
(out / 'PioneerQwen35DataflowServiceImage.h').write_text(
    '#pragma once\n'
    + ('#error "fused RMS/save simulator image is not a driver image"\n' if fused_rms_save else '')
    + 'namespace PioneerQwen35 {\n'
    f'inline constexpr unsigned DataflowServiceImageBootWord=0x{unpack("<I", boot)[0]:08x}U;\n'
    + park_definition
    + 'inline constexpr unsigned DataflowServiceTextWords[]={\n' + words + '};\n'
    + dispatch_definition + '}\n',
    encoding='ascii')
PY
if "$cross-objdump" -d "$output/service.elf" \
	| grep -E '[[:space:]](amo[a-z]+[.]w|lr[.]w|sc[.]w)[[:space:]]' >/dev/null; then
	echo 'resident worker uses an unsupported A-extension atomic' >&2
	exit 1
fi
if grep -E '[[:space:]]((fdiv|fsqrt)[.][sd]|fcvt[.](w|wu)[.][sd])[[:space:]]' "$output/disassembly.txt" >/dev/null; then
	echo 'resident BRISC worker uses an unsupported floating divide/square root or a rounding-sensitive float-to-integer conversion' >&2
	exit 1
fi
shasum -a 256 "$output/boot.bin" "$output/text.bin"
if [ -f "$output/dispatch.bin" ]; then shasum -a 256 "$output/dispatch.bin"; fi
if [ -f "$output/device-graph.su" ]; then
python3 - "$output" "$role_graph" <<'PY'
from pathlib import Path
import sys

out = Path(sys.argv[1])
shared = sys.argv[2] == '1'
entries = []
for name in (('service.su', 'device-graph.su', 'shared-roles.su') if shared
             else ('service.su', 'device-graph.su')):
    for line in (out / name).read_text().splitlines():
        parts = line.rsplit('\t', 2)
        if len(parts) == 3 and parts[1].isdigit():
            entries.append((parts[0], int(parts[1]), parts[2]))
groups = (
    'qwen35_dataflow_service_main()',
    'PioneerQwen35DataflowBlackholeWorkerService()',
    'qwen35_graph_run_descriptor(',
    'PioneerQwen35DeviceGraphRun(',
    'DataflowDeviceGraph<IO, BarrierIO>::Tick()',
    'qwen35_graph_authorize(',
    'DataflowDynamicPatcher::PatchValidated(',
    'DataflowConvInvocation::Valid()',
    'DataflowDeviceBarrier<IO>::Poll()',
    'DataflowDeviceBarrier<IO>::ReleaseChildren()',
    'DataflowBlackholeNoc<Noc>::Transfer(',
    'qwen35_graph_execute(',
    'DataflowWorkerExecuteAuthorized(',
)
groups += (('qwen35_shared_compute_coordinate(', 'PioneerQwen35SharedActivationCompute(',
            'DataflowBlackholeTensixOps::TensixMatmul(', 'DataflowBindLocalContext(')
           if shared else ('::PioneerQwen35DataflowBlackholeWorker(',
                           'DataflowPackActivationCache(', 'PioneerQ38ProjectionQuantize('))
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
