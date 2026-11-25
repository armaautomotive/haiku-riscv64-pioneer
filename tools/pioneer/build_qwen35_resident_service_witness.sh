#!/bin/sh
# Build an offline resident-token service using an already compiled paired
# shared-role graph. This does not install a driver or access the card.
set -eu
[ "$#" = 3 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
build=$(CDPATH= cd -- "$1" && pwd)
graph=$(CDPATH= cd -- "$2" && pwd)
output=$3
case "$output" in /*) ;; *) exit 2;; esac
[ ! -e "$output" ] && [ ! -L "$output" ] || exit 2
cross="$build/cross-tools-riscv64/bin/riscv64-unknown-haiku"
sdk="$build/objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop"
for piece in service.o memory.o device-stream.o device-barrier.o device-graph.o shared-roles.o service.su device-graph.su shared-roles.su; do
    [ -f "$graph/$piece" ] || exit 2
done
"$cross-nm" "$graph/shared-roles.o" | grep ' qwen35_shared_compute_coordinate$' >/dev/null
"$cross-nm" "$graph/device-graph.o" | grep ' PioneerQwen35DeviceGraphRun$' >/dev/null
umask 077
mkdir -m 700 "$output"
case "${PIONEER_QWEN35_TREE_SETTLEMENT_WITNESS:-0}" in
    0) tree_flags= ;;
    1) tree_flags=-DPIONEER_QWEN35_TREE_SETTLEMENT ;;
    *) exit 2 ;;
esac
case "${PIONEER_QWEN35_FUSED_RECURRENCE_COMPILE_ONLY:-0}" in
    0) [ ! -e "$graph/fused-recurrence.marker" ] || exit 2; fused_flags= ;;
    1) [ -f "$graph/fused-recurrence.marker" ] || exit 2; fused_flags=-DPIONEER_QWEN35_FUSED_RECURRENCE ;;
    *) exit 2 ;;
esac
flags='-std=c++17 -Os -fno-inline-functions-called-once -Wall -Wextra -Werror -ffreestanding -fno-exceptions -fno-rtti -fno-stack-protector -fno-pic -fno-pie -fno-builtin -ffp-contract=off -ffunction-sections -fdata-sections -fno-unwind-tables -fno-asynchronous-unwind-tables -fstack-usage -msmall-data-limit=0 -mno-relax -march=rv32imf_zicsr -mabi=ilp32'
"$cross-g++" $flags -DPIONEER_QWEN35_DEVICE_DISPATCH -DPIONEER_QWEN35_RECOVERY_COMPILE_ONLY \
    -DPIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS -DPIONEER_QWEN35_PAIRED_RECURRENCE $tree_flags $fused_flags \
    -I"$directory" -I"$sdk/headers" -idirafter "$sdk/headers/posix" \
    -c "$directory/PioneerQwen35DataflowResidentServiceEntry.cpp" -o "$output/resident-service-entry.o"
"$cross-as" -march=rv32imf_zicsr -mabi=ilp32 -mno-relax \
    "$directory/blackhole_qwen35_resident_service_start.S" -o "$output/resident-start.o"
"$cross-ld" -m elf32lriscv --no-relax --gc-sections -u PioneerQwen35DeviceGraphRun \
    -T "$directory/blackhole_qwen35_resident_graph_witness.ld" \
    "$output/resident-start.o" "$graph/service.o" "$graph/memory.o" \
    "$graph/device-stream.o" "$graph/device-barrier.o" "$graph/device-graph.o" \
    "$graph/shared-roles.o" "$output/resident-service-entry.o" \
    -Map "$output/service.map" -o "$output/service.elf"
"$cross-nm" -u "$output/service.elf" > "$output/imports.txt"
test ! -s "$output/imports.txt"
"$cross-nm" -n "$output/service.elf" > "$output/symbols.txt"
"$cross-objdump" -d "$output/service.elf" > "$output/disassembly.txt"
for symbol in qwen35_resident_graph_service_main qwen35_resident_graph_stopped_park qwen35_graph_run_descriptor qwen35_shared_compute_coordinate; do
    grep " $symbol$" "$output/symbols.txt" >/dev/null
done
if [ "${PIONEER_QWEN35_TREE_SETTLEMENT_WITNESS:-0}" = 1 ]; then
    grep 'ReadResidentChildReceipt' "$output/symbols.txt" >/dev/null
    grep 'DataflowBarrierPeerAllowed' "$output/symbols.txt" >/dev/null
fi
for symbol in qwen35_resident_graph_service_main qwen35_graph_run_descriptor qwen35_shared_compute_coordinate; do
    grep -E "jalr.*<$symbol>" "$output/disassembly.txt" >/dev/null
done
# The resident projection must be compiled with graph flags; otherwise its
# first-projection trace silently disappears from the linked compute body.
grep -E 'jalr.*DataflowBlackholeTensixOps.*TraceProjection' "$output/disassembly.txt" >/dev/null
grep -E 'jalr.*DataflowBlackholeTensixOps.*MatrixPanel' "$output/disassembly.txt" >/dev/null
if grep -E '[[:space:]](amo[a-z]+[.]w|lr[.]w|sc[.]w)[[:space:]]' "$output/disassembly.txt" >/dev/null; then
    echo 'resident service witness uses unsupported atomics' >&2;exit 1
fi
if grep -E '[[:space:]]((fdiv|fsqrt)[.][sd]|fcvt[.](w|wu)[.][sd])[[:space:]]' "$output/disassembly.txt" >/dev/null; then
    echo 'resident service witness uses unsupported floating operations' >&2;exit 1
fi
for section in boot text dispatch; do "$cross-objcopy" -O binary -j ".$section" "$output/service.elf" "$output/$section.bin";done
"$cross-size" -A "$output/service.elf"
python3 - "$graph" "$output" "${PIONEER_QWEN35_TREE_SETTLEMENT_WITNESS:-0}" <<'PY'
from pathlib import Path
import sys
graph,out=map(Path,sys.argv[1:3])
tree=sys.argv[3]=='1'
entries=[]
for path in (graph/'service.su',graph/'device-graph.su',graph/'shared-roles.su',out/'resident-service-entry.su'):
    for line in path.read_text().splitlines():
        parts=line.rsplit('\t',2)
        if len(parts)==3 and parts[1].isdigit(): entries.append((parts[0],int(parts[1]),parts[2]))
markers=('qwen35_resident_graph_service_main()', 'DataflowResidentServiceRun<IO, BarrierIO>::Tick()',
    'qwen35_graph_run_descriptor(', 'PioneerQwen35DeviceGraphRun(',
    'DataflowDeviceGraph<IO, BarrierIO>::Tick()', 'qwen35_graph_authorize(',
    'DataflowDynamicPatcher::PatchValidated(', 'DataflowConvInvocation::Valid()',
    'DataflowDeviceBarrier<IO>::Poll()', 'DataflowDeviceBarrier<IO>::ReleaseChildren()',
    'DataflowBlackholeNoc<Noc>::Transfer(', 'qwen35_graph_execute(',
    'DataflowWorkerExecuteAuthorized(', 'qwen35_shared_compute_coordinate(',
    'PioneerQwen35SharedActivationCompute(', 'DataflowBlackholeTensixOps::TensixMatmul(',
    'DataflowBindLocalContext(')
total=1024
for marker in markers:
    matches=[(frame,kind) for name,frame,kind in entries if marker in name]
    if not matches or any(kind!='static' for _,kind in matches):
        raise SystemExit('missing or unbounded resident service stack frame: '+marker)
    total+=max(frame for frame,_ in matches)
if total>8192: raise SystemExit(f'resident graph stack envelope {total} exceeds 8192 bytes')
print(f'resident_graph_stack_envelope_bytes={total}/8192 (includes 1024-byte allowance; not whole-program proof)')
if tree:
    settlement_markers=('qwen35_resident_graph_service_main()', 'DataflowResidentServiceRun<IO, BarrierIO>::Tick()',
        'DataflowResidentLifecycle<IO, BarrierIO>::Tick()', 'DataflowResidentLifecycle<IO, BarrierIO>::Gather()',
        'DataflowBlackholeBarrierIO::ReadResidentChildReceipt(', 'DataflowBlackholeNoc<Noc>::Transfer(',
        'DataflowBarrierPeerAllowed(', 'DataflowBarrierWorkerRoute(')
    settlement=1024
    for marker in settlement_markers:
        matches=[(frame,kind) for name,frame,kind in entries if marker in name]
        if not matches or any(kind!='static' for _,kind in matches):
            raise SystemExit('missing or unbounded settlement stack frame: '+marker)
        settlement+=max(frame for frame,_ in matches)
    if settlement>8192: raise SystemExit(f'resident settlement stack envelope {settlement} exceeds 8192 bytes')
    print(f'resident_settlement_stack_envelope_bytes={settlement}/8192 (includes 1024-byte allowance; not whole-program proof)')
    root_markers=('qwen35_resident_graph_service_main()', 'DataflowResidentServiceRun<IO, BarrierIO>::Tick()',
        'DataflowResidentRootCommandRead(', 'DataflowResidentRootCommitMatches(', 'DataflowResidentCommandValid(')
    root_total=1024
    for marker in root_markers:
        matches=[(frame,kind) for name,frame,kind in entries if marker in name]
        if not matches or any(kind!='static' for _,kind in matches):
            raise SystemExit('missing or unbounded compact root stack frame: '+marker)
        root_total+=max(frame for frame,_ in matches)
    if root_total>8192: raise SystemExit(f'compact root stack envelope {root_total} exceeds 8192 bytes')
    print(f'resident_compact_root_stack_envelope_bytes={root_total}/8192 (includes 1024-byte allowance; not whole-program proof)')
PY
shasum -a 256 "$output/boot.bin" "$output/text.bin" "$output/dispatch.bin"
echo 'OFFLINE resident graph witness only; no driver, card, inference or speed qualification'
