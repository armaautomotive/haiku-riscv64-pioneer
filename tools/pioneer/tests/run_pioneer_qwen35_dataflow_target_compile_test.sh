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
"$size" -A "$temporary/graph-service/service.elf"
"$nm" "$temporary/graph-service/service.elf" | grep ' T PioneerQwen35DeviceGraphRun$' >/dev/null
"$nm" "$temporary/graph-service/service.elf" | grep ' T qwen35_graph_authorize$' >/dev/null

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
	-fsanitize=address,undefined "$directory/pioneer_qwen35_graph_authorization_test.cpp" \
	-o "$temporary/graph-authorization-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/graph-authorization-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror \
	-fsanitize=address,undefined "$directory/pioneer_qwen35_graph_completion_test.cpp" \
	-o "$temporary/graph-completion-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/graph-completion-test"
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
