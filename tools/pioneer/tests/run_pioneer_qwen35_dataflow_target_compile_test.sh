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
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-qwen35-dataflow-target.XXXXXX")
trap 'rm -rf "$temporary"' EXIT HUP INT TERM
object="$temporary/dataflow.o"
barrier_object="$temporary/device-barrier.o"

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
"$objdump" -d "$barrier_object" > "$temporary/device-barrier.disassembly"
if grep -E '[[:space:]](amo[a-z]+|lr\.w|sc\.w)[[:space:]]' "$temporary/device-barrier.disassembly" >/dev/null; then
	echo 'target device barrier uses an unsupported atomic instruction' >&2
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
	-fsanitize=address,undefined -DPIONEER_QWEN35_DEVICE_DISPATCH \
	"$directory/pioneer_qwen35_dataflow_service_owner_test.cpp" -o "$temporary/device-barrier-owner-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/device-barrier-owner-test"
echo 'PASS: Qwen35 dataflow roles compile as RV32IMF without A atomics or runtime helpers'
