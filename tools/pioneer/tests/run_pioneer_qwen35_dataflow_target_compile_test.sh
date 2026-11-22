#!/bin/sh
# Compile the dataflow role contracts for the actual freestanding Work ISA.
set -eu
[ "$#" = 1 ] || exit 2
test -d "$1/cross-tools-riscv64/bin"
build=$(CDPATH= cd -- "$1" && pwd)
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
root=$(CDPATH= cd -- "$directory/../../.." && pwd)
objects="$build/objects/haiku/riscv64"
sdk="$objects/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop"
cxx="$build/cross-tools-riscv64/bin/riscv64-unknown-haiku-g++"
nm="$build/cross-tools-riscv64/bin/riscv64-unknown-haiku-nm"
objdump="$build/cross-tools-riscv64/bin/riscv64-unknown-haiku-objdump"
readelf="$build/cross-tools-riscv64/bin/riscv64-unknown-haiku-readelf"
ld="$build/cross-tools-riscv64/bin/riscv64-unknown-haiku-ld"
size="$build/cross-tools-riscv64/bin/riscv64-unknown-haiku-size"
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-qwen35-dataflow-target.XXXXXX")
trap 'rm -rf "$temporary"' EXIT HUP INT TERM
object="$temporary/dataflow.o"
barrier_object="$temporary/device-barrier.o"
stream_object="$temporary/device-stream.o"
queued_object="$temporary/queued-compute.o"
roles_object="$temporary/queued-roles.o"

"$cxx" -std=c++17 -O2 -Wall -Wextra -Werror -ffreestanding \
	-fno-exceptions -fno-rtti -fno-stack-protector -fno-pic -fno-pie \
	-fno-builtin -ffp-contract=off -ffunction-sections -fdata-sections \
	-fno-unwind-tables -fno-asynchronous-unwind-tables -msmall-data-limit=0 \
	-mno-relax -march=rv32imf_zicsr -mabi=ilp32 \
	-I"$root/tools/pioneer" -I"$sdk/headers" -idirafter "$sdk/headers/posix" \
	-c "$directory/pioneer_qwen35_dataflow_target_compile_probe.cpp" \
	-o "$object"

"$cxx" -std=c++17 -O2 -Wall -Wextra -Werror -ffreestanding \
	-fno-exceptions -fno-rtti -fno-stack-protector -fno-pic -fno-pie \
	-fno-builtin -ffp-contract=off -ffunction-sections -fdata-sections \
	-fno-unwind-tables -fno-asynchronous-unwind-tables -msmall-data-limit=0 \
	-mno-relax -march=rv32imf_zicsr -mabi=ilp32 \
	-I"$root/tools/pioneer" -I"$sdk/headers" -idirafter "$sdk/headers/posix" \
	-c "$directory/pioneer_qwen35_dataflow_device_barrier_target_probe.cpp" \
	-o "$barrier_object"

"$cxx" -std=c++17 -O2 -Wall -Wextra -Werror -ffreestanding \
	-fno-exceptions -fno-rtti -fno-stack-protector -fno-pic -fno-pie \
	-fno-builtin -ffp-contract=off -ffunction-sections -fdata-sections \
	-fno-unwind-tables -fno-asynchronous-unwind-tables -msmall-data-limit=0 \
	-mno-relax -march=rv32imf_zicsr -mabi=ilp32 \
	-I"$root/tools/pioneer" -I"$sdk/headers" -idirafter "$sdk/headers/posix" \
	-c "$directory/pioneer_qwen35_dataflow_device_stream_target_probe.cpp" \
	-o "$stream_object"

# Offline-only separate compute role: compile the concrete Tensix backend
# against ticket-paired queue pages. It is not linked into the service image.
"$cxx" -std=c++17 -O2 -Wall -Wextra -Werror -ffreestanding \
	-fno-exceptions -fno-rtti -fno-stack-protector -fno-pic -fno-pie \
	-fno-builtin -ffp-contract=off -ffunction-sections -fdata-sections \
	-fno-unwind-tables -fno-asynchronous-unwind-tables -msmall-data-limit=0 \
	-mno-relax -march=rv32imf_zicsr -mabi=ilp32 \
	-I"$root/tools/pioneer" -I"$sdk/headers" -idirafter "$sdk/headers/posix" \
	-c "$directory/pioneer_qwen35_dataflow_queued_compute_target_probe.cpp" \
	-o "$queued_object"
if [ -n "$("$nm" -u "$queued_object")" ]; then
	echo 'queued compute probe has unresolved runtime helpers' >&2
	exit 1
fi
"$objdump" -d "$queued_object" > "$temporary/queued-compute.disassembly"
if grep -E '[[:space:]](amo[a-z]+|lr\.w|sc\.w)[[:space:]]' "$temporary/queued-compute.disassembly" >/dev/null; then
	echo 'queued compute probe uses unsupported atomics' >&2
	exit 1
fi
# Concrete once-only reader, Tensix compute and writer entry bodies. Compile
# only; no boot vector, image owner or on-card launch is provided here.
"$cxx" -std=c++17 -O2 -Wall -Wextra -Werror -ffreestanding \
	-fno-exceptions -fno-rtti -fno-stack-protector -fno-pic -fno-pie \
	-fno-builtin -ffp-contract=off -ffunction-sections -fdata-sections \
	-fno-unwind-tables -fno-asynchronous-unwind-tables -msmall-data-limit=0 \
	-mno-relax -march=rv32imf_zicsr -mabi=ilp32 \
	-I"$root/tools/pioneer" -I"$sdk/headers" -idirafter "$sdk/headers/posix" \
	-c "$root/tools/pioneer/PioneerQwen35DataflowQueuedRoles.cpp" -o "$roles_object"
if [ -n "$("$nm" -u "$roles_object")" ]; then
	echo 'queued role bodies have unresolved runtime helpers' >&2
	exit 1
fi
for role in reader compute writer; do
	"$nm" "$roles_object" | grep " T qwen35_queued_${role}_once$" >/dev/null
done
"$objdump" -d "$roles_object" > "$temporary/queued-roles.disassembly"
if grep -E '[[:space:]](amo[a-z]+|lr\.w|sc\.w)[[:space:]]' "$temporary/queued-roles.disassembly" >/dev/null; then
	echo 'queued role bodies use unsupported atomics' >&2
	exit 1
fi
echo 'PASS concrete queued role bodies compile for RV32IMF; OFFLINE ONLY'
# Separate link-only size witnesses deliberately share a hypothetical base.
# They are not service images, are not mutually placed, and are never loaded.
for role in reader compute writer; do
	witness="$temporary/queued-${role}-size-witness.elf"
	"$ld" -m elf32lriscv --no-relax --gc-sections -Ttext 0xc0000 \
		-e "qwen35_queued_${role}_once" "$roles_object" -o "$witness"
	if [ -n "$("$nm" -u "$witness")" ]; then
		echo "queued ${role} size witness has unresolved imports" >&2
		exit 1
	fi
	"$size" "$witness"
done
echo 'PASS separate queued role link-size witnesses; NOT SERVICE IMAGES'
# Check proposed disjoint code slots beyond the currently saved L1 view. This
# is only a link witness: the current owner must never load either result.
for role in reader writer; do
	if [ "$role" = reader ]; then start=0xb0000; limit=0xb4000
	else start=0xb4000; limit=0xb6000; fi
	"$ld" -m elf32lriscv --no-relax --gc-sections \
		--defsym ROLE_ADDRESS=$start --defsym ROLE_LIMIT=$limit \
		-e "qwen35_queued_${role}_once" \
		-T "$root/tools/pioneer/blackhole_qwen35_queued_role_witness.ld" \
		"$roles_object" -o "$temporary/queued-${role}-layout-witness.elf"
	if [ -n "$("$nm" -u "$temporary/queued-${role}-layout-witness.elf")" ]; then
		echo "queued ${role} layout witness has unresolved imports" >&2
		exit 1
	fi
	"$size" -A "$temporary/queued-${role}-layout-witness.elf"
done
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror \
	-fsanitize=address,undefined "$directory/pioneer_qwen35_dataflow_role_memory_plan_test.cpp" \
	-o "$temporary/role-memory-plan-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/role-memory-plan-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror \
	-fsanitize=address,undefined "$directory/pioneer_qwen35_dataflow_queued_input_test.cpp" \
	-o "$temporary/queued-input-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/queued-input-test"

# This links both compile probes into a second saved L1 region. It is a size
# witness only: the production image still has one code region and does not
# load or run either dispatch probe.
sh "$root/tools/pioneer/build_qwen35_dataflow_service_image.sh" "$build" "$temporary/service"
"$ld" -m elf32lriscv --no-relax --gc-sections \
	-u PioneerQwen35DeviceStreamCompileProbe -u PioneerQwen35DeviceBarrierCompileProbe \
	-T "$root/tools/pioneer/blackhole_qwen35_dataflow_dispatch_witness.ld" \
	"$temporary/service/start.o" "$temporary/service/service.o" "$temporary/service/memory.o" \
	"$stream_object" "$barrier_object" -o "$temporary/dispatch-size-witness.elf"
if [ -n "$("$nm" -u "$temporary/dispatch-size-witness.elf")" ]; then
	echo 'split dispatch witness has unresolved imports' >&2
	exit 1
fi
"$size" -A "$temporary/dispatch-size-witness.elf"
# The non-loadable graph witness links real stream, barrier, local record
# validation, and worker-executor code. Model authority remains host-owned.
PIONEER_QWEN35_DEVICE_DISPATCH_COMPILE_ONLY=1 PIONEER_QWEN35_RECOVERY_COMPILE_ONLY=1 \
	sh "$root/tools/pioneer/build_qwen35_dataflow_service_image.sh" "$build" "$temporary/graph-service"
PIONEER_QWEN35_DEVICE_DISPATCH_TRIAL=1 \
	sh "$root/tools/pioneer/build_qwen35_dataflow_service_image.sh" "$build" "$temporary/graph-trial-service"
for section in boot text dispatch; do
	cmp "$temporary/graph-service/$section.bin" "$temporary/graph-trial-service/$section.bin"
done
if python3 "$root/tools/pioneer/build_qwen35_dataflow.py" --build-device-dispatch-trial \
	--compile-recovery-prototype "$build" "$temporary/invalid-mixed-build" >"$temporary/mixed.log" 2>&1; then
	echo 'graph trial unexpectedly admitted the reset compile witness' >&2
	exit 1
else
	[ "$?" = 2 ] && grep -q 'cannot enable a recovery compile witness' "$temporary/mixed.log"
fi
echo 'PASS graph qualification image matches witness and excludes reset witness mode'
"$size" -A "$temporary/graph-service/service.elf"
"$nm" "$temporary/graph-service/service.elf" | grep ' T PioneerQwen35DeviceGraphRun$' >/dev/null
"$nm" "$temporary/graph-service/service.elf" | grep ' T qwen35_graph_authorize$' >/dev/null
"$nm" -C "$temporary/graph-service/service.elf" > "$temporary/graph-symbols.txt"
python3 - "$temporary/graph-symbols.txt" <<'PYTEST'
import sys
from pathlib import Path
lines=[line for line in Path(sys.argv[1]).read_text().splitlines() if 'DataflowBlackholeTensixOps::TensixBeginOutput(' in line]
assert len(lines)==1,lines
address=int(lines[0].split()[0],16)
assert 0xa0400<=address<0xaa000,hex(address)
print('PASS Tensix setup helper resides in the saved dispatch image')
PYTEST

attributes=$("$readelf" -A "$object")
printf '%s\n' "$attributes" | grep 'Tag_RISCV_arch: "rv32i' >/dev/null
if printf '%s\n' "$attributes" | grep '_a[0-9]' >/dev/null; then
	echo 'target object unexpectedly enables the RISC-V A extension' >&2
	exit 1
fi
barrier_undefined=$("$nm" -u "$barrier_object")
if [ -n "$barrier_undefined" ]; then
	printf '%s\n' "$barrier_undefined" >&2
	echo 'target device barrier has unresolved runtime helpers' >&2
	exit 1
fi
stream_undefined=$("$nm" -u "$stream_object")
if [ -n "$stream_undefined" ]; then
	printf '%s\n' "$stream_undefined" >&2
	echo 'target device stream has unresolved runtime helpers' >&2
	exit 1
fi
"$objdump" -d "$barrier_object" > "$temporary/device-barrier.disassembly"
if grep -E '[[:space:]](amo[a-z]+|lr\.w|sc\.w)[[:space:]]' "$temporary/device-barrier.disassembly" >/dev/null; then
	echo 'target device barrier uses an unsupported atomic instruction' >&2
	exit 1
fi
"$objdump" -d "$stream_object" > "$temporary/device-stream.disassembly"
if grep -E '[[:space:]](amo[a-z]+|lr\.w|sc\.w)[[:space:]]' "$temporary/device-stream.disassembly" >/dev/null; then
	echo 'target device stream uses an unsupported atomic instruction' >&2
	exit 1
fi
undefined=$("$nm" -u "$object")
if [ -n "$undefined" ]; then
	printf '%s\n' "$undefined" >&2
	echo 'target dataflow object has unresolved runtime helpers' >&2
	exit 1
fi
"$objdump" -d "$object" > "$temporary/dataflow.disassembly"
if grep -E '[[:space:]](amo[a-z]+|lr\.w|sc\.w)[[:space:]]' \
	"$temporary/dataflow.disassembly" >/dev/null; then
	echo 'target dataflow object uses an unsupported atomic instruction' >&2
	exit 1
fi
grep -E '[[:space:]]fence[[:space:]]+r,rw' \
	"$temporary/dataflow.disassembly" >/dev/null
grep -E '[[:space:]]fence[[:space:]]+rw,w' \
	"$temporary/dataflow.disassembly" >/dev/null
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror \
	-fsanitize=address,undefined "$directory/pioneer_qwen35_dataflow_noc_protocol_test.cpp" \
	-o "$temporary/noc-protocol-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/noc-protocol-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror \
	-fsanitize=address,undefined "$directory/pioneer_qwen35_dataflow_noc1_audit_test.cpp" \
	-o "$temporary/noc1-audit-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/noc1-audit-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror \
	-fsanitize=address,undefined "$directory/pioneer_qwen35_dataflow_device_barrier_test.cpp" \
	-o "$temporary/device-barrier-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/device-barrier-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror \
	-fsanitize=address,undefined "$directory/pioneer_qwen35_dataflow_device_stream_test.cpp" \
	-o "$temporary/device-stream-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/device-stream-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror \
	-fsanitize=address,undefined "$directory/pioneer_qwen35_dataflow_device_graph_test.cpp" \
	-o "$temporary/device-graph-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/device-graph-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror \
	-fsanitize=address,undefined "$directory/pioneer_qwen35_dataflow_stream_recorder_test.cpp" \
	-o "$temporary/stream-recorder-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/stream-recorder-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror \
	-fsanitize=address,undefined "$directory/pioneer_qwen35_stream_bank_sink_test.cpp" \
	-o "$temporary/stream-bank-sink-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/stream-bank-sink-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror \
	-fsanitize=address,undefined "$directory/pioneer_qwen35_stream_recording_integration_test.cpp" \
	-o "$temporary/stream-recording-integration-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/stream-recording-integration-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror \
	-fsanitize=address,undefined "$directory/pioneer_qwen35_dynamic_token_test.cpp" \
	-o "$temporary/dynamic-token-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/dynamic-token-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror \
	-fsanitize=address,undefined "$directory/pioneer_qwen35_graph_publication_test.cpp" \
	-o "$temporary/graph-publication-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/graph-publication-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror \
	-fsanitize=address,undefined "$directory/pioneer_qwen35_graph_stage_ledger_test.cpp" \
	-o "$temporary/graph-stage-ledger-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/graph-stage-ledger-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror \
	-fsanitize=address,undefined "$directory/pioneer_qwen35_graph_descriptor_update_test.cpp" \
	-o "$temporary/graph-descriptor-update-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/graph-descriptor-update-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror \
	-fsanitize=address,undefined "$directory/pioneer_qwen35_graph_authorization_test.cpp" \
	-o "$temporary/graph-authorization-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/graph-authorization-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror \
	-fsanitize=address,undefined "$directory/pioneer_qwen35_graph_completion_test.cpp" \
	-o "$temporary/graph-completion-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/graph-completion-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror \
	-fsanitize=address,undefined "$directory/pioneer_qwen35_dataflow_graph_profile_test.cpp" \
	-o "$temporary/dataflow-graph-profile-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/dataflow-graph-profile-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror \
	-fsanitize=address,undefined "$directory/pioneer_qwen35_dataflow_graph_trace_test.cpp" \
	-o "$temporary/dataflow-graph-trace-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/dataflow-graph-trace-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror \
	-I"$directory/cli-stubs" -fsanitize=address,undefined \
	"$directory/pioneer_qwen35_dataflow_trace_protocol_test.cpp" -o "$temporary/dataflow-trace-protocol-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/dataflow-trace-protocol-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror \
	-fsanitize=address,undefined "$directory/pioneer_qwen35_dataflow_affine_layout_test.cpp" \
	-o "$temporary/dataflow-affine-layout-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/dataflow-affine-layout-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror \
	-fsanitize=address,undefined "$directory/pioneer_qwen35_dataflow_phases_test.cpp" \
	-o "$temporary/dataflow-phases-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/dataflow-phases-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror \
	-fsanitize=address,undefined "$directory/pioneer_qwen35_dataflow_tensix_init_test.cpp" \
	-o "$temporary/dataflow-tensix-init-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/dataflow-tensix-init-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror \
	-fsanitize=address,undefined "$directory/pioneer_qwen35_dataflow_silu_test.cpp" \
	-o "$temporary/dataflow-silu-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/dataflow-silu-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror \
	-fsanitize=address,undefined "$directory/pioneer_qwen35_dataflow_conv_test.cpp" \
	-o "$temporary/dataflow-conv-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/dataflow-conv-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror \
	-fsanitize=address,undefined "$directory/pioneer_qwen35_dataflow_recovery_retry_test.cpp" \
	-o "$temporary/dataflow-recovery-retry-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/dataflow-recovery-retry-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror \
	-fsanitize=address,undefined "$directory/pioneer_qwen35_graph_fault_audit_test.cpp" \
	-o "$temporary/graph-fault-audit-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/graph-fault-audit-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror \
	-fsanitize=address,undefined "$directory/pioneer_qwen35_graph_service_owner_test.cpp" \
	-o "$temporary/graph-service-owner-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/graph-service-owner-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror \
	-fsanitize=address,undefined "$directory/pioneer_qwen35_graph_model_transaction_test.cpp" \
	-o "$temporary/graph-model-transaction-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/graph-model-transaction-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror \
	-fsanitize=address,undefined "$directory/pioneer_qwen35_graph_session_test.cpp" \
	-o "$temporary/graph-session-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/graph-session-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror \
	-fsanitize=address,undefined "$directory/pioneer_qwen35_graph_owned_runner_test.cpp" \
	-o "$temporary/graph-owned-runner-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/graph-owned-runner-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror \
	-fsanitize=address,undefined "$directory/pioneer_qwen35_graph_allocation_test.cpp" \
	-o "$temporary/graph-allocation-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/graph-allocation-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror \
	-fsanitize=address,undefined "$directory/pioneer_qwen35_graph_generation_test.cpp" \
	-o "$temporary/graph-generation-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/graph-generation-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror \
	-fsanitize=address,undefined "$directory/pioneer_qwen35_graph_generation_integration_test.cpp" \
	-o "$temporary/graph-generation-integration-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/graph-generation-integration-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror \
	-fsanitize=address,undefined "$directory/pioneer_qwen35_graph_metadata_test.cpp" \
	-o "$temporary/graph-metadata-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/graph-metadata-test"
"$cxx" -std=gnu++17 -O2 -Wall -Wextra -Werror -fno-exceptions -fno-rtti \
	-ffreestanding -fstack-usage -mcmodel=medany -fpic -D_KERNEL_MODE \
	-I"$sdk/headers" -idirafter "$sdk/headers/posix" \
	-c "$directory/pioneer_qwen35_graph_metadata_kernel_probe.cpp" -o "$temporary/graph-metadata-kernel.o"
awk -F '\t' 'NF==3 { if ($2+0>maximum) maximum=$2+0; if ($3!="static") bad=1; seen=1 }
	END { if(!seen||bad) exit 1; print "graph_metadata_max_single_frame_bytes=" maximum }' "$temporary/graph-metadata-kernel.su"
"$cxx" -std=gnu++17 -O2 -Wall -Wextra -Werror -fno-exceptions -fno-rtti \
	-ffreestanding -fstack-usage -mcmodel=medany -fpic -D_KERNEL_MODE \
	-I"$sdk/headers" -idirafter "$sdk/headers/posix" \
	-c "$directory/pioneer_qwen35_dataflow_stream_recorder_kernel_probe.cpp" -o "$temporary/stream-recorder-kernel.o"
awk -F '\t' 'NF==3 { if ($2+0>maximum) maximum=$2+0; if ($3!="static") bad=1; seen=1 }
	END { if(!seen||bad) exit 1; print "stream_recorder_max_single_frame_bytes=" maximum }' "$temporary/stream-recorder-kernel.su"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror \
	-fsanitize=address,undefined -DPIONEER_QWEN35_DEVICE_DISPATCH \
	"$directory/pioneer_qwen35_dataflow_service_owner_test.cpp" -o "$temporary/device-barrier-owner-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/device-barrier-owner-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror \
	-fsanitize=address,undefined -DPIONEER_QWEN35_DEVICE_DISPATCH \
	"$directory/pioneer_qwen35_dataflow_pci_memory_test.cpp" -o "$temporary/device-dispatch-pci-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/device-dispatch-pci-test"
echo 'PASS: Qwen35 dataflow roles compile as RV32IMF without A atomics or runtime helpers'
