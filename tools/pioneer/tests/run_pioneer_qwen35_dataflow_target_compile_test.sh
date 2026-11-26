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
objcopy="$build/cross-tools-riscv64/bin/riscv64-unknown-haiku-objcopy"
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
# The compute coordinator witness includes the concrete Tensix backend, but
# it is not placed in the resident graph and is not a loadable service image.
"$ld" -m elf32lriscv --no-relax --gc-sections -Ttext 0xc0000 \
	-e qwen35_queued_compute_coordinate "$roles_object" \
	-o "$temporary/queued-compute-coordinator-witness.elf"
if [ -n "$("$nm" -u "$temporary/queued-compute-coordinator-witness.elf")" ]; then
	echo 'queued compute coordinator witness has unresolved imports' >&2
	exit 1
fi
"$size" "$temporary/queued-compute-coordinator-witness.elf"
echo 'PASS concrete queued Tensix coordinator link witness; NOT A SERVICE IMAGE'
# Alternative shared-activation bodies: these are separately linked witnesses,
# never simultaneous with the queued-input variants and never loaded.
"$cxx" -std=c++17 -O2 -Wall -Wextra -Werror -ffreestanding \
	-fno-exceptions -fno-rtti -fno-stack-protector -fno-pic -fno-pie \
	-fno-builtin -ffp-contract=off -ffunction-sections -fdata-sections \
	-fno-unwind-tables -fno-asynchronous-unwind-tables -fstack-usage -msmall-data-limit=0 \
	-mno-relax -march=rv32imf_zicsr -mabi=ilp32 \
	-I"$root/tools/pioneer" -I"$sdk/headers" -idirafter "$sdk/headers/posix" \
	-c "$root/tools/pioneer/PioneerQwen35DataflowSharedRoles.cpp" -o "$temporary/shared-roles.o"
if [ -n "$("$nm" -u "$temporary/shared-roles.o")" ]; then
	echo 'shared activation role witness has unresolved imports' >&2
	exit 1
fi
"$objdump" -d "$temporary/shared-roles.o" > "$temporary/shared-roles.disassembly"
if grep -E '[[:space:]](amo[a-z]+|lr\.w|sc\.w)[[:space:]]' "$temporary/shared-roles.disassembly" >/dev/null; then
	echo 'shared activation role witness uses unsupported atomics' >&2
	exit 1
fi
if grep -E '[[:space:]]((fdiv|fsqrt)[.][sd]|fcvt[.](w|wu)[.][sd])[[:space:]]' "$temporary/shared-roles.disassembly" >/dev/null; then
	echo 'shared role witness uses an unsupported divide, square root or rounding-sensitive conversion' >&2
	exit 1
fi
for role in reader writer; do
	if [ "$role" = reader ]; then start=0xb0000; limit=0xb5000
	else start=0xb5000; limit=0xb8000; fi
	"$ld" -m elf32lriscv --no-relax --gc-sections \
		--defsym ROLE_ADDRESS=$start --defsym ROLE_LIMIT=$limit \
		-e "qwen35_shared_${role}_tick" \
		-T "$root/tools/pioneer/blackhole_qwen35_queued_role_witness.ld" \
		"$temporary/shared-roles.o" -o "$temporary/shared-${role}-witness.elf"
	"$size" -A "$temporary/shared-${role}-witness.elf"
done
"$ld" -m elf32lriscv --no-relax --gc-sections -Ttext 0xc0000 \
	-e qwen35_shared_compute_coordinate "$temporary/shared-roles.o" \
	-o "$temporary/shared-compute-witness.elf"
if [ -n "$("$nm" -u "$temporary/shared-compute-witness.elf")" ]; then
	echo 'shared activation compute witness has unresolved imports' >&2
	exit 1
fi
"$size" "$temporary/shared-compute-witness.elf"
"$objdump" -d "$temporary/shared-compute-witness.elf" > "$temporary/shared-compute.disassembly"
if ! grep -E '[[:space:]]csrs[[:space:]]+0x7c0,8' "$temporary/shared-compute.disassembly" >/dev/null \
	|| ! grep -E '[[:space:]]csrr[[:space:]]+[^,]+,0x7c0' "$temporary/shared-compute.disassembly" >/dev/null; then
	echo 'shared compute witness missing cache-bypass set/readback' >&2
	exit 1
fi
echo 'PASS shared activation reader, Tensix compute and writer witnesses; OFFLINE ONLY'
python3 - "$temporary/shared-roles.su" <<'PYSTACK'
from pathlib import Path
import sys
entries=[]
for line in Path(sys.argv[1]).read_text().splitlines():
    parts=line.rsplit('\t',2)
    if len(parts)!=3 or not parts[1].isdigit() or parts[2]!='static':
        raise SystemExit('missing or non-static shared role stack frame')
    entries.append(int(parts[1]))
if not entries:
    raise SystemExit('missing shared role stack usage')
total=sum(entries)+16+1024
if total>8192:
    raise SystemExit(f'shared role frame-sum witness {total} exceeds 8192 bytes')
print(f'PASS shared role frame-sum witness {total}/8192 bytes; not a recursion or whole-program proof')
PYSTACK
# Startup/park shims are link witnesses only. No boot vector, reset write,
# physical owner, memory transfer or live service installation is provided.
"$cxx" -c -x assembler-with-cpp -mno-relax -march=rv32imf_zicsr -mabi=ilp32 \
	-I"$root/tools/pioneer" "$root/tools/pioneer/blackhole_qwen35_shared_role_start.S" \
	-o "$temporary/shared-start.o"
for role in reader writer; do
	if [ "$role" = reader ]; then shim=0x70000; shim_limit=0x70100; start=0xb0000; limit=0xb5000
	else shim=0x70100; shim_limit=0x70200; start=0xb5000; limit=0xb8000; fi
	"$ld" -m elf32lriscv --no-relax --gc-sections \
		--defsym ROLE_SHIM_ADDRESS=$shim --defsym ROLE_SHIM_LIMIT=$shim_limit \
		--defsym ROLE_ADDRESS=$start --defsym ROLE_LIMIT=$limit \
		-e "qwen35_shared_${role}_start" \
		-T "$root/tools/pioneer/blackhole_qwen35_shared_role_witness.ld" \
		"$temporary/shared-start.o" "$temporary/shared-roles.o" \
		-o "$temporary/shared-${role}-start-witness.elf"
	if [ -n "$("$nm" -u "$temporary/shared-${role}-start-witness.elf")" ]; then
		echo "shared ${role} start witness has unresolved imports" >&2
		exit 1
	fi
	"$size" -A "$temporary/shared-${role}-start-witness.elf"
	"$objdump" -d "$temporary/shared-${role}-start-witness.elf" > "$temporary/shared-${role}-start.disassembly"
	if ! grep -E '[[:space:]]csrs[[:space:]]+0x7c0,8' "$temporary/shared-${role}-start.disassembly" >/dev/null \
		|| ! grep -E '[[:space:]]csrr[[:space:]]+[^,]+,0x7c0' "$temporary/shared-${role}-start.disassembly" >/dev/null; then
		echo "shared ${role} start witness missing cache-bypass set/readback" >&2
		exit 1
	fi
done
if sh "$root/tools/pioneer/build_qwen35_shared_role_images.sh" "$build" "$temporary/invalid-role-images" >/dev/null 2>&1; then
	echo 'shared role images compiled without the recovery-only guard' >&2
	exit 1
fi
PIONEER_QWEN35_RECOVERY_COMPILE_ONLY=1 PIONEER_QWEN35_DEVICE_DISPATCH_COMPILE_ONLY=1 \
	sh "$root/tools/pioneer/build_qwen35_shared_role_images.sh" "$build" "$temporary/shared-role-images"
python3 "$directory/pioneer_qwen35_dataflow_noc_source_order_test.py" \
	"$temporary/shared-role-images/writer.disassembly"
for role in reader writer; do
	for section in shim text; do
		if [ "$section" = text ]; then suffix=code; else suffix=shim; fi
		"$objcopy" -O binary -j ".$section" "$temporary/shared-${role}-start-witness.elf" \
			"$temporary/expected-${role}-${suffix}.bin"
		cmp "$temporary/expected-${role}-${suffix}.bin" "$temporary/shared-role-images/${role}-${suffix}.bin"
	done
	python3 "$directory/pioneer_qwen35_role_stackless_copy_test.py" \
		"$temporary/shared-role-images/${role}-shim.bin" "$role"
done
echo 'PASS reproducible shared reader/writer image export; OFFLINE ONLY'
# An independent local-RAM restore witness has no driver or reset binding.
# Execute its actual linked RV32 bytes, including fault and bypass cases.
"$cxx" -c -x assembler-with-cpp -mno-relax -march=rv32imf_zicsr -mabi=ilp32 \
	-I"$root/tools/pioneer" "$root/tools/pioneer/blackhole_qwen35_role_local_restore.S" \
	-o "$temporary/local-restore.o"
for role in reader writer; do
	if [ "$role" = reader ]; then address=0x70000; else address=0x70100; fi
	"$ld" -m elf32lriscv --no-relax --gc-sections -Ttext="$address" \
		-e "qwen35_restore_${role}_start" "$temporary/local-restore.o" \
		-o "$temporary/local-restore-${role}.elf"
	if [ -n "$("$nm" -u "$temporary/local-restore-${role}.elf")" ]; then
		echo 'local restore witness has unresolved imports' >&2; exit 1
	fi
	"$objcopy" -O binary -j .text "$temporary/local-restore-${role}.elf" \
		"$temporary/local-restore-${role}.bin"
	python3 "$directory/pioneer_qwen35_role_local_restore_test.py" \
		"$temporary/local-restore-${role}.bin" "$role"
done
if PIONEER_QWEN35_RECOVERY_COMPILE_ONLY=0 PIONEER_QWEN35_DEVICE_DISPATCH_COMPILE_ONLY=0 \
    sh "$root/tools/pioneer/build_qwen35_local_restore_images.sh" "$build" \
        "$temporary/invalid-local-restore-images" >/dev/null 2>&1; then
    echo 'local restore images accepted a build without recovery guards' >&2; exit 1
fi
PIONEER_QWEN35_RECOVERY_COMPILE_ONLY=1 PIONEER_QWEN35_DEVICE_DISPATCH_COMPILE_ONLY=1 \
    sh "$root/tools/pioneer/build_qwen35_local_restore_images.sh" "$build" "$temporary/local-restore-images"
for role in reader writer; do
    cmp "$temporary/local-restore-${role}.bin" "$temporary/local-restore-images/${role}.bin"
    python3 "$directory/pioneer_qwen35_role_local_restore_test.py" \
        "$temporary/local-restore-images/${role}.bin" "$role"
done
python3 "$directory/pioneer_qwen35_role_local_restore_images_test.py" "$temporary/local-restore-images"
if PIONEER_QWEN35_RECOVERY_COMPILE_ONLY=1 PIONEER_QWEN35_DEVICE_DISPATCH_COMPILE_ONLY=1 \
    sh "$root/tools/pioneer/build_qwen35_local_restore_probe_images.sh" "$build" "$temporary/invalid-local-probe"; then
    echo 'restricted local restore probe builder accepted missing guard' >&2; exit 1
fi
PIONEER_QWEN35_RECOVERY_COMPILE_ONLY=1 PIONEER_QWEN35_DEVICE_DISPATCH_COMPILE_ONLY=1 \
    PIONEER_QWEN35_LOCAL_RESTORE_PROBE_BUILD=1 \
    sh "$root/tools/pioneer/build_qwen35_local_restore_probe_images.sh" "$build" "$temporary/local-restore-probe-images"
for role in reader writer; do
    cmp "$temporary/local-restore-images/${role}.bin" "$temporary/local-restore-probe-images/local-restore/${role}.bin"
    python3 "$directory/pioneer_qwen35_local_restore_probe_test.py" \
        "$temporary/local-restore-probe-images/${role}-shim.bin" \
        "$temporary/local-restore-probe-images/local-restore/${role}.bin" "$role"
done
python3 "$directory/pioneer_qwen35_local_restore_probe_images_test.py" "$temporary/local-restore-probe-images"
if sh "$root/tools/pioneer/build_qwen35_role_service_probe_images.sh" "$build" "$temporary/invalid-service-probe" >/dev/null 2>&1; then
    echo 'restricted service probe builder accepted missing guard' >&2; exit 1
fi
PIONEER_QWEN35_RECOVERY_COMPILE_ONLY=1 PIONEER_QWEN35_DEVICE_DISPATCH_COMPILE_ONLY=1 \
    PIONEER_QWEN35_ROLE_SERVICE_PROBE_BUILD=1 \
    sh "$root/tools/pioneer/build_qwen35_role_service_probe_images.sh" "$build" "$temporary/role-service-probe-images"
for role in reader writer; do
    cmp "$temporary/shared-role-images/${role}-code.bin" "$temporary/role-service-probe-images/${role}-service.bin"
    python3 "$directory/pioneer_qwen35_role_stackless_copy_test.py" "$temporary/role-service-probe-images/${role}-shim.bin" "$role"
done
python3 "$directory/pioneer_qwen35_role_service_probe_test.py" "$temporary/role-service-probe-images"
python3 "$directory/pioneer_qwen35_role_fault_image_test.py" "$temporary/role-service-probe-images"
python3 "$directory/pioneer_qwen35_role_service_guard_test.py" "$build"
PYTHONDONTWRITEBYTECODE=1 python3 "$directory/pioneer_qwen35_firmware_trace_guard_test.py" "$build"
for witness in copy_aware core_release core_restore_stage core_restore_handoff core_retirement; do
    "$cxx" -std=gnu++17 -O2 -Wall -Wextra -Werror -ffreestanding \
        -fno-exceptions -fno-rtti -fno-stack-protector -mcmodel=medany -fpic -D_KERNEL_MODE \
        -fstack-usage -I"$sdk/headers" -idirafter "$sdk/headers/posix" \
        -c "$directory/pioneer_qwen35_${witness}_kernel_compile.cpp" -o "$temporary/${witness}_kernel.o"
done
python3 - "$temporary/copy_aware_kernel.su" "$temporary/core_release_kernel.su" "$temporary/core_restore_stage_kernel.su" "$temporary/core_restore_handoff_kernel.su" "$temporary/core_retirement_kernel.su" <<'PYSTACK'
from pathlib import Path
import sys
for filename,markers in ((sys.argv[1],('PioneerQ35CopyAwareCompileWitness','PioneerQ35CoreLocalFleetCompileWitness')),
                         (sys.argv[2],('PioneerQ35CoreLocalReleaseCompileWitness',)),
                         (sys.argv[3],('PioneerQ35CoreRestoreStageCompileWitness','PioneerQ35CoreRestoreReleaseCompileWitness')),
                         (sys.argv[4],('PioneerQ35CoreRestoreHandoffCompileWitness',)),
                         (sys.argv[5],('PioneerQ35CoreRetirementCompileWitness',))):
    rows=[line.rsplit('\t',2) for line in Path(filename).read_text().splitlines()]
    if not rows or any(len(row)!=3 or not row[1].isdigit() or row[2]!='static' for row in rows):
        raise SystemExit('copy-aware compile witness has missing or non-static stack frames')
    if any(not any(marker in row[0] for row in rows) for marker in markers):
        raise SystemExit('copy-aware compile witness entry is missing')
    total=sum(int(row[1]) for row in rows)
    if total>4096:
        raise SystemExit(f'copy-aware compile witness selected frames exceed budget: {total}')
    print(f'PASS {Path(filename).name} selected frames {total}/4096; IO/caller not audited')
PYSTACK
# Dedicated takeover entry has no private-RAM preservation or snapshot
# handoff. Keep it separately linked and unavailable to the existing owner.
"$cxx" -c -x assembler-with-cpp -mno-relax -march=rv32imf_zicsr -mabi=ilp32 \
	-I"$root/tools/pioneer" "$root/tools/pioneer/blackhole_qwen35_exclusive_role_start.S" \
	-o "$temporary/exclusive-role-start.o"
for role in reader writer; do
	if [ "$role" = reader ]; then shim=0x70000; shim_limit=0x70100; start=0xb0000; limit=0xb5000
	else shim=0x70100; shim_limit=0x70200; start=0xb5000; limit=0xb8000; fi
	"$ld" -m elf32lriscv --no-relax --gc-sections \
		--defsym ROLE_SHIM_ADDRESS=$shim --defsym ROLE_SHIM_LIMIT=$shim_limit \
		--defsym ROLE_ADDRESS=$start --defsym ROLE_LIMIT=$limit \
		-e "qwen35_shared_${role}_start" -T "$root/tools/pioneer/blackhole_qwen35_shared_role_witness.ld" \
		"$temporary/exclusive-role-start.o" "$temporary/shared-roles.o" -o "$temporary/exclusive-$role.elf"
	if [ -n "$("$nm" -u "$temporary/exclusive-$role.elf")" ]; then
		echo "exclusive $role image has unresolved imports" >&2; exit 1
	fi
	"$objcopy" -O binary -j .shim "$temporary/exclusive-$role.elf" "$temporary/exclusive-$role-shim.bin"
	cmp "$temporary/exclusive-$role-shim.bin" "$temporary/shared-role-images/exclusive-$role-shim.bin"
	if cmp -s "$temporary/exclusive-$role-shim.bin" "$temporary/shared-role-images/$role-shim.bin"; then
		echo "exclusive $role shim unexpectedly matches snapshot shim" >&2; exit 1
	fi
	tick=$("$nm" -n "$temporary/exclusive-$role.elf" | awk -v symbol="qwen35_shared_${role}_tick" '$3==symbol {print $1}')
	test -n "$tick"
	python3 "$directory/pioneer_qwen35_exclusive_role_start_test.py" "$temporary/exclusive-$role-shim.bin" "$role" "$tick"
done
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
	-I"$temporary/shared-role-images" "$directory/pioneer_qwen35_exclusive_role_fleet_test.cpp" \
	-o "$temporary/exclusive-role-fleet-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/exclusive-role-fleet-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
	"$directory/pioneer_qwen35_exclusive_service_start_test.cpp" -o "$temporary/exclusive-service-start-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/exclusive-service-start-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
	"$directory/pioneer_qwen35_dedicated_fleet_test.cpp" -o "$temporary/dedicated-fleet-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/dedicated-fleet-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
	"$directory/pioneer_qwen35_dedicated_peer_lease_test.cpp" -o "$temporary/dedicated-peer-lease-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/dedicated-peer-lease-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
	"$directory/pioneer_qwen35_dedicated_fleet_peer_test.cpp" -o "$temporary/dedicated-fleet-peer-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/dedicated-fleet-peer-test"
sed -n '/PIONEER_Q38_DISPATCH_PREPARE_BEGIN/,/PIONEER_Q38_DISPATCH_PREPARE_END/p' \
	"$root/src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp" > "$temporary/pioneer_dedicated_prepare.inc"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
	-I"$temporary" "$directory/pioneer_qwen35_dedicated_prepare_binding_test.cpp" -o "$temporary/dedicated-prepare-binding-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/dedicated-prepare-binding-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -DTEST_SNAPSHOT_TAKEOVER \
	-fsanitize=address,undefined -I"$temporary" "$directory/pioneer_qwen35_dedicated_prepare_binding_test.cpp" \
	-o "$temporary/dedicated-snapshot-prepare-binding-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/dedicated-snapshot-prepare-binding-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
	"$directory/pioneer_qwen35_dedicated_snapshot_lifecycle_test.cpp" -o "$temporary/dedicated-snapshot-lifecycle-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/dedicated-snapshot-lifecycle-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
	"$directory/pioneer_qwen35_exclusive_role_stop_test.cpp" -o "$temporary/exclusive-role-stop-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/exclusive-role-stop-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror \
	-fsanitize=address,undefined "$directory/pioneer_qwen35_dataflow_role_boot_test.cpp" \
	-o "$temporary/role-boot-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/role-boot-test"
"$cxx" -std=c++17 -O2 -Wall -Wextra -Werror -ffreestanding \
	-fno-exceptions -fno-rtti -fno-stack-protector -fno-pic -fno-pie \
	-fno-builtin -ffp-contract=off -ffunction-sections -fdata-sections \
	-fno-unwind-tables -fno-asynchronous-unwind-tables -msmall-data-limit=0 \
	-mno-relax -march=rv32imf_zicsr -mabi=ilp32 \
	-I"$root/tools/pioneer" -I"$sdk/headers" -idirafter "$sdk/headers/posix" \
	-c "$directory/pioneer_qwen35_dataflow_role_boot_owner_target_probe.cpp" \
	-o "$temporary/role-boot-owner.o"
if [ -n "$("$nm" -u "$temporary/role-boot-owner.o")" ]; then
	echo 'role boot owner target probe has unresolved imports' >&2
	exit 1
fi
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror \
	-fsanitize=address,undefined "$directory/pioneer_qwen35_dataflow_role_boot_owner_test.cpp" \
	-o "$temporary/role-boot-owner-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/role-boot-owner-test"
"$cxx" -std=c++17 -O2 -Wall -Wextra -Werror -ffreestanding \
	-fno-exceptions -fno-rtti -fno-stack-protector -fno-pic -fno-pie \
	-fno-builtin -ffp-contract=off -ffunction-sections -fdata-sections \
	-fno-unwind-tables -fno-asynchronous-unwind-tables -msmall-data-limit=0 \
	-mno-relax -march=rv32imf_zicsr -mabi=ilp32 \
	-I"$root/tools/pioneer" -I"$sdk/headers" -idirafter "$sdk/headers/posix" \
	-c "$directory/pioneer_qwen35_dataflow_role_fleet_boot_owner_target_probe.cpp" \
	-o "$temporary/role-fleet-boot-owner.o"
if [ -n "$("$nm" -u "$temporary/role-fleet-boot-owner.o")" ]; then
	echo 'role fleet boot owner target probe has unresolved imports' >&2
	exit 1
fi
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror \
	-fsanitize=address,undefined "$directory/pioneer_qwen35_dataflow_role_fleet_boot_owner_test.cpp" \
	-o "$temporary/role-fleet-boot-owner-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/role-fleet-boot-owner-test"
# Composed fleet image ownership remains a fake-IO-only contract.
"$cxx" -std=c++17 -O2 -Wall -Wextra -Werror -ffreestanding \
	-fno-exceptions -fno-rtti -fno-stack-protector -fno-pic -fno-pie \
	-fno-builtin -ffp-contract=off -ffunction-sections -fdata-sections \
	-fno-unwind-tables -fno-asynchronous-unwind-tables -msmall-data-limit=0 \
	-mno-relax -march=rv32imf_zicsr -mabi=ilp32 \
	-I"$root/tools/pioneer" -I"$sdk/headers" -idirafter "$sdk/headers/posix" \
	-c "$directory/pioneer_qwen35_dataflow_role_fleet_images_owner_target_probe.cpp" \
	-o "$temporary/role-fleet-images-owner.o"
if [ -n "$("$nm" -u "$temporary/role-fleet-images-owner.o")" ]; then
	echo 'role fleet images owner target probe has unresolved imports' >&2
	exit 1
fi
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror \
	-fsanitize=address,undefined "$directory/pioneer_qwen35_dataflow_role_fleet_images_owner_test.cpp" \
	-o "$temporary/role-fleet-images-owner-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/role-fleet-images-owner-test"
# B/NC private RAM and composed recovery are simulated; no reset is issued.
"$cxx" -std=c++17 -O2 -Wall -Wextra -Werror -ffreestanding \
	-fno-exceptions -fno-rtti -fno-stack-protector -fno-pic -fno-pie \
	-fno-builtin -ffp-contract=off -ffunction-sections -fdata-sections \
	-fno-unwind-tables -fno-asynchronous-unwind-tables -msmall-data-limit=0 \
	-mno-relax -march=rv32imf_zicsr -mabi=ilp32 \
	-I"$root/tools/pioneer" -I"$sdk/headers" -idirafter "$sdk/headers/posix" \
	-c "$directory/pioneer_qwen35_dataflow_role_fleet_recovery_owner_target_probe.cpp" \
	-o "$temporary/role-fleet-recovery-owner.o"
if [ -n "$("$nm" -u "$temporary/role-fleet-recovery-owner.o")" ]; then
	echo 'role fleet recovery target probe has unresolved imports' >&2
	exit 1
fi
for test in role_local_ram_owner role_fleet_local_ram_owner role_restore_handoff_owner role_fleet_recovery_owner; do
	${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror \
		-fsanitize=address,undefined "$directory/pioneer_qwen35_dataflow_${test}_test.cpp" \
		-o "$temporary/${test}-test"
	ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/${test}-test"
done
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
    "$directory/pioneer_qwen35_auto_restored_fleet_ledger_test.cpp" -o "$temporary/auto-restored-fleet-ledger-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/auto-restored-fleet-ledger-test"
# Check proposed disjoint code slots including service-tick adapters, beyond
# the currently saved L1 view. No resident loop, boot shim or owner is linked;
# the current owner must never load either result.
for role in reader writer; do
	if [ "$role" = reader ]; then start=0xb0000; limit=0xb5000
	else start=0xb5000; limit=0xb8000; fi
	"$ld" -m elf32lriscv --no-relax --gc-sections \
		--defsym ROLE_ADDRESS=$start --defsym ROLE_LIMIT=$limit \
		-e "qwen35_queued_${role}_tick" \
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
# Snapshot/stage/restore remains a simulated IO contract; compile its target
# instantiation separately and exercise partial failures on the host.
"$cxx" -std=c++17 -O2 -Wall -Wextra -Werror -ffreestanding \
	-fno-exceptions -fno-rtti -fno-stack-protector -fno-pic -fno-pie \
	-fno-builtin -ffp-contract=off -ffunction-sections -fdata-sections \
	-fno-unwind-tables -fno-asynchronous-unwind-tables -msmall-data-limit=0 \
	-mno-relax -march=rv32imf_zicsr -mabi=ilp32 \
	-I"$root/tools/pioneer" -I"$sdk/headers" -idirafter "$sdk/headers/posix" \
	-c "$directory/pioneer_qwen35_dataflow_role_code_owner_target_probe.cpp" \
	-o "$temporary/role-code-owner.o"
if [ -n "$("$nm" -u "$temporary/role-code-owner.o")" ]; then
	echo 'role code owner target probe has unresolved imports' >&2
	exit 1
fi
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror \
	-fsanitize=address,undefined "$directory/pioneer_qwen35_dataflow_role_code_owner_test.cpp" \
	-o "$temporary/role-code-owner-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/role-code-owner-test"
# The 120-core code snapshot contract is simulation only and does not bind
# a physical PCI implementation or prove boot/reset/NoC recovery.
"$cxx" -std=c++17 -O2 -Wall -Wextra -Werror -ffreestanding \
	-fno-exceptions -fno-rtti -fno-stack-protector -fno-pic -fno-pie \
	-fno-builtin -ffp-contract=off -ffunction-sections -fdata-sections \
	-fno-unwind-tables -fno-asynchronous-unwind-tables -msmall-data-limit=0 \
	-mno-relax -march=rv32imf_zicsr -mabi=ilp32 \
	-I"$root/tools/pioneer" -I"$sdk/headers" -idirafter "$sdk/headers/posix" \
	-c "$directory/pioneer_qwen35_dataflow_role_fleet_code_owner_target_probe.cpp" \
	-o "$temporary/role-fleet-code-owner.o"
if [ -n "$("$nm" -u "$temporary/role-fleet-code-owner.o")" ]; then
	echo 'fleet code owner target probe has unresolved imports' >&2
	exit 1
fi
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror \
	-fsanitize=address,undefined "$directory/pioneer_qwen35_dataflow_role_fleet_code_owner_test.cpp" \
	-o "$temporary/role-fleet-code-owner-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/role-fleet-code-owner-test"
# Fixed read-only PCI view planning, partial-map cleanup and SDK adapter;
# the guard here is fake and the object is not linked to the live driver.
"$cxx" -std=c++17 -O2 -Wall -Wextra -Werror -ffreestanding \
	-fno-exceptions -fno-rtti -fno-stack-protector -fno-pic -fno-pie \
	-fno-builtin -ffp-contract=off -ffunction-sections -fdata-sections \
	-fno-unwind-tables -fno-asynchronous-unwind-tables -msmall-data-limit=0 \
	-mno-relax -march=rv32imf_zicsr -mabi=ilp32 \
	-I"$root/tools/pioneer" -I"$sdk/headers" -idirafter "$sdk/headers/posix" \
	-c "$directory/pioneer_qwen35_dataflow_role_views_target_probe.cpp" \
	-o "$temporary/role-views.o"
if [ -n "$("$nm" -u "$temporary/role-views.o")" ]; then
	echo 'role views target probe has unresolved imports' >&2
	exit 1
fi
for test in role_physical_plan role_read_only_views; do
	${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror \
		-fsanitize=address,undefined "$directory/pioneer_qwen35_dataflow_${test}_test.cpp" \
		-o "$temporary/${test}-test"
	ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/${test}-test"
done
"$cxx" -std=gnu++17 -O2 -Wall -Wextra -Werror -fno-exceptions -fno-rtti \
	-ffreestanding -fstack-usage -mcmodel=medany -fpic -D_KERNEL_MODE \
	-I"$sdk/headers" -I"$sdk/headers/os" -I"$sdk/headers/os/drivers" \
	-I"$sdk/headers/os/kernel" -I"$sdk/headers/os/support" -I"$sdk/headers/os/storage" \
	-idirafter "$sdk/headers/posix" \
	-c "$directory/pioneer_qwen35_dataflow_role_haiku_views_kernel_compile.cpp" \
	-o "$temporary/role-haiku-views-kernel.o"
awk -F '\t' 'NF==3 {if($2+0>2048||$3!="static")bad=1;seen=1} END{exit !(seen&&!bad)}' \
	"$temporary/role-haiku-views-kernel.su"
echo 'PASS read-only Haiku role mapper SDK compile; NO LIVE DRIVER BINDING'
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
	-I"$directory/role_haiku_stubs" "$directory/pioneer_qwen35_dataflow_role_writable_views_test.cpp" \
	-o "$temporary/role-writable-views-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/role-writable-views-test"
if "$cxx" -std=gnu++17 -O2 -fno-exceptions -fno-rtti -ffreestanding -mcmodel=medany -fpic -D_KERNEL_MODE \
	-DPIONEER_QWEN35_ROLE_WRITE_COMPILE_ONLY -I"$sdk/headers" -I"$sdk/headers/os" \
	-I"$sdk/headers/os/drivers" -I"$sdk/headers/os/kernel" -I"$sdk/headers/os/support" \
	-I"$sdk/headers/os/storage" -idirafter "$sdk/headers/posix" \
	-c "$directory/pioneer_qwen35_dataflow_role_writable_views_kernel_probe.cpp" \
	-o "$temporary/invalid-role-writable-views.o" >"$temporary/invalid-role-writable-views.log" 2>&1; then
	echo 'writable role mapper compiled without recovery-only guard' >&2
	exit 1
fi
"$cxx" -std=gnu++17 -O2 -Wall -Wextra -Werror -fno-exceptions -fno-rtti \
	-ffreestanding -fstack-usage -mcmodel=medany -fpic -D_KERNEL_MODE \
	-DPIONEER_QWEN35_DEVICE_DISPATCH -DPIONEER_QWEN35_RECOVERY_COMPILE_ONLY \
	-DPIONEER_QWEN35_ROLE_WRITE_COMPILE_ONLY \
	-I"$sdk/headers" -I"$sdk/headers/os" -I"$sdk/headers/os/drivers" \
	-I"$sdk/headers/os/kernel" -I"$sdk/headers/os/support" -I"$sdk/headers/os/storage" \
	-idirafter "$sdk/headers/posix" \
	-c "$directory/pioneer_qwen35_dataflow_role_writable_views_kernel_probe.cpp" \
	-o "$temporary/role-writable-views-kernel.o"
awk -F '\t' 'NF==3 {if($2+0>2048||$3!="static")bad=1;seen=1} END{exit !(seen&&!bad)}' \
	"$temporary/role-writable-views-kernel.su"
echo 'PASS writable Haiku role mapper SDK compile; NO LIVE DRIVER BINDING'
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror \
	-fsanitize=address,undefined "$directory/pioneer_qwen35_dataflow_role_route_guard_test.cpp" \
	-o "$temporary/role-route-guard-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/role-route-guard-test"
"$cxx" -std=gnu++17 -O2 -Wall -Wextra -Werror -fno-exceptions -fno-rtti \
	-ffreestanding -fstack-usage -mcmodel=medany -fpic -D_KERNEL_MODE \
	-I"$sdk/headers" -I"$sdk/headers/os" -I"$sdk/headers/os/drivers" \
	-I"$sdk/headers/os/kernel" -I"$sdk/headers/os/support" -I"$sdk/headers/os/storage" \
	-idirafter "$sdk/headers/posix" \
	-c "$directory/pioneer_qwen35_dataflow_role_route_guard_kernel_compile.cpp" \
	-o "$temporary/role-route-guard-kernel.o"
awk -F '\t' 'NF==3 {if($2+0>2048||$3!="static")bad=1;seen=1} END{exit !(seen&&!bad)}' \
	"$temporary/role-route-guard-kernel.su"
echo 'PASS role selected-route guard SDK compile; NO LIVE SESSION SCOPE'
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
	-DPIONEER_QWEN35_DEVICE_DISPATCH -DPIONEER_QWEN35_RECOVERY_COMPILE_ONLY \
	-DPIONEER_QWEN35_ROLE_WRITE_COMPILE_ONLY "$directory/pioneer_qwen35_dataflow_role_route_guard_test.cpp" \
	-o "$temporary/role-write-route-guard-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/role-write-route-guard-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror \
	-fsanitize=address,undefined -I"$directory/role_haiku_stubs" \
	"$directory/pioneer_qwen35_dataflow_role_haiku_scope_test.cpp" \
	-o "$temporary/role-haiku-scope-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/role-haiku-scope-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
	-DPIONEER_QWEN35_ROLE_WRITE_COMPILE_ONLY -I"$directory/role_haiku_stubs" \
	"$directory/pioneer_qwen35_dataflow_role_haiku_scope_test.cpp" -o "$temporary/role-write-haiku-scope-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/role-write-haiku-scope-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
	"$directory/pioneer_qwen35_dataflow_l2cpu_reset_probe_test.cpp" -o "$temporary/l2cpu-reset-probe-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/l2cpu-reset-probe-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
	"$directory/pioneer_qwen35_dataflow_held_fleet_sample_test.cpp" -o "$temporary/held-fleet-sample-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/held-fleet-sample-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
	"$directory/pioneer_qwen35_dataflow_firmware_identity_test.cpp" -o "$temporary/firmware-identity-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/firmware-identity-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
	-I"$directory/role_haiku_stubs" "$directory/pioneer_qwen35_dataflow_role_fleet_pci_io_test.cpp" \
	-o "$temporary/role-fleet-pci-io-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/role-fleet-pci-io-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
	-DPIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS -I"$directory/role_haiku_stubs" \
	"$directory/pioneer_qwen35_dataflow_role_fleet_pci_io_test.cpp" -o "$temporary/core-restore-pci-io-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/core-restore-pci-io-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
	-DPIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS -DPIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS -I"$directory/role_haiku_stubs" \
	"$directory/pioneer_qwen35_dataflow_role_fleet_pci_io_test.cpp" -o "$temporary/exclusive-role-pci-io-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/exclusive-role-pci-io-test"
if python3 "$root/tools/pioneer/build_qwen35_dataflow.py" --compile-role-write-prototype \
	"$build" "$temporary/invalid-role-write-driver" >"$temporary/invalid-role-write-driver.log" 2>&1; then
	echo 'role write driver accepted an unguarded build' >&2
	exit 1
fi
if python3 "$root/tools/pioneer/build_qwen35_dataflow.py" --compile-device-dispatch-prototype \
	--compile-shared-role-owner-prototype "$build" "$temporary/invalid-shared-owner-driver" \
	>"$temporary/invalid-shared-owner-driver.log" 2>&1; then
	echo 'shared role owner accepted a build without the role-write guard' >&2
	exit 1
fi
if python3 -B "$root/tools/pioneer/build_qwen35_dataflow.py" --compile-snapshot-takeover-prototype \
	"$build" "$temporary/invalid-snapshot-owner" >"$temporary/invalid-snapshot-owner.log" 2>&1; then
	echo 'snapshot takeover accepted a build without the shared-role owner' >&2; exit 1
fi
if python3 -B "$root/tools/pioneer/build_qwen35_dataflow.py" --compile-fabric-guard-prototype \
	"$build" "$temporary/invalid-fabric-guard" >"$temporary/invalid-fabric-guard.log" 2>&1; then
	echo 'fabric guard accepted a build without the non-loadable snapshot witness' >&2; exit 1
fi
if python3 -B "$root/tools/pioneer/build_qwen35_dataflow.py" --compile-device-dispatch-prototype \
	--compile-role-write-prototype --compile-shared-role-owner-prototype \
	--compile-exclusive-takeover-prototype --compile-snapshot-takeover-prototype \
	"$build" "$temporary/invalid-snapshot-exclusive" >"$temporary/invalid-snapshot-exclusive.log" 2>&1; then
	echo 'snapshot and destructive takeover were accepted together' >&2; exit 1
fi
# Per-tile handoff uses only single-writer words and the existing fences.
"$cxx" -std=c++17 -O2 -Wall -Wextra -Werror -ffreestanding \
	-fno-exceptions -fno-rtti -fno-stack-protector -fno-pic -fno-pie \
	-fno-builtin -ffp-contract=off -ffunction-sections -fdata-sections \
	-fno-unwind-tables -fno-asynchronous-unwind-tables -msmall-data-limit=0 \
	-mno-relax -march=rv32imf_zicsr -mabi=ilp32 \
	-I"$root/tools/pioneer" -I"$sdk/headers" -idirafter "$sdk/headers/posix" \
	-c "$directory/pioneer_qwen35_dataflow_role_mailbox_target_probe.cpp" \
	-o "$temporary/role-mailbox.o"
if [ -n "$("$nm" -u "$temporary/role-mailbox.o")" ]; then
	echo 'role mailbox target probe has unresolved imports' >&2
	exit 1
fi
"$objdump" -d "$temporary/role-mailbox.o" > "$temporary/role-mailbox.disassembly"
if grep -E '[[:space:]](amo[a-z]+|lr\.w|sc\.w)[[:space:]]' "$temporary/role-mailbox.disassembly" >/dev/null; then
	echo 'role mailbox target probe uses unsupported atomics' >&2
	exit 1
fi
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -pthread \
	-fsanitize=address,undefined "$directory/pioneer_qwen35_dataflow_role_mailbox_test.cpp" \
	-o "$temporary/role-mailbox-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/role-mailbox-test"
# Coordinator simulation exercises concurrent role completion, queue audit,
# fault retention and missing receipts; the target probe binds only fake IO.
"$cxx" -std=c++17 -O2 -Wall -Wextra -Werror -ffreestanding \
	-fno-exceptions -fno-rtti -fno-stack-protector -fno-pic -fno-pie \
	-fno-builtin -ffp-contract=off -ffunction-sections -fdata-sections \
	-fno-unwind-tables -fno-asynchronous-unwind-tables -msmall-data-limit=0 \
	-mno-relax -march=rv32imf_zicsr -mabi=ilp32 \
	-I"$root/tools/pioneer" -I"$sdk/headers" -idirafter "$sdk/headers/posix" \
	-c "$directory/pioneer_qwen35_dataflow_role_coordinator_target_probe.cpp" \
	-o "$temporary/role-coordinator.o"
if [ -n "$("$nm" -u "$temporary/role-coordinator.o")" ]; then
	echo 'role coordinator target probe has unresolved imports' >&2
	exit 1
fi
"$objdump" -d "$temporary/role-coordinator.o" > "$temporary/role-coordinator.disassembly"
if grep -E '[[:space:]](amo[a-z]+|lr\.w|sc\.w)[[:space:]]' "$temporary/role-coordinator.disassembly" >/dev/null; then
	echo 'role coordinator target probe uses unsupported atomics' >&2
	exit 1
fi
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -pthread \
	-fsanitize=address,undefined "$directory/pioneer_qwen35_dataflow_role_coordinator_test.cpp" \
	-o "$temporary/role-coordinator-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/role-coordinator-test"
sh "$directory/run_pioneer_qwen35_activation_pipeline_tests.sh"
"$cxx" -std=c++17 -Os -Wall -Wextra -Werror -ffreestanding -fno-exceptions -fno-rtti \
    -I"$root/tools/pioneer" -I"$sdk/headers" -idirafter "$sdk/headers/posix" \
    -c "$directory/pioneer_qwen35_activation_share_plan_target_probe.cpp" -o "$temporary/activation-share-host.o"
"$cxx" -std=gnu++17 -O2 -Wall -Wextra -Werror -fno-exceptions -fno-rtti -ffreestanding -fstack-usage \
    -mcmodel=medany -fpic -D_KERNEL_MODE -DPIONEER_QWEN35_RECOVERY_COMPILE_ONLY \
    -DPIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS -DPIONEER_QWEN35_ACTIVATION_SHARING_WITNESS \
    -DPIONEER_QWEN35_PAIRED_RECURRENCE -DPIONEER_QWEN35_FUSED_RECURRENCE \
    -I"$sdk/headers" -idirafter "$sdk/headers/posix" \
    -c "$directory/pioneer_qwen35_dataflow_stream_recorder_kernel_probe.cpp" -o "$temporary/activation-share-recorder-host.o"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -pthread \
	-fsanitize=address,undefined "$directory/pioneer_qwen35_dataflow_program_test.cpp" \
	-o "$temporary/dataflow-program-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/dataflow-program-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -ffp-contract=off \
	-fsanitize=address,undefined "$directory/pioneer_qwen35_dataflow_delta_test.cpp" \
	-o "$temporary/dataflow-delta-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/dataflow-delta-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -pthread \
	-fsanitize=address,undefined "$directory/pioneer_qwen35_dataflow_projection_qualification_test.cpp" \
	-o "$temporary/dataflow-projection-qualification-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/dataflow-projection-qualification-test"
echo 'PASS queued role program, including inactive reader shards; SIMULATED ONLY'
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror \
    -fsanitize=address,undefined "$directory/pioneer_qwen35_activation_sums_test.cpp" \
    -o "$temporary/activation-sums-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/activation-sums-test"
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
# Link the actual graph executor to the new shared-role compute path. The
# bridge is compile-only; it does not install role images or own hardware.
role_flags='-std=c++17 -Os -Wall -Wextra -Werror -ffreestanding -fno-exceptions -fno-rtti -fno-stack-protector -fno-pic -fno-pie -fno-builtin -ffp-contract=off -ffunction-sections -fdata-sections -fno-unwind-tables -fno-asynchronous-unwind-tables -fstack-usage -msmall-data-limit=0 -mno-relax -march=rv32imf_zicsr -mabi=ilp32'
if "$cxx" $role_flags -DPIONEER_QWEN35_DEVICE_DISPATCH -DPIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS \
	-I"$root/tools/pioneer" -I"$sdk/headers" -idirafter "$sdk/headers/posix" \
	-c "$root/tools/pioneer/PioneerQwen35DataflowServiceImage.cpp" \
	-o "$temporary/invalid-role-service.o" >"$temporary/invalid-role-service.log" 2>&1; then
	echo 'shared-role graph bridge compiled without the recovery-only guard' >&2
	exit 1
fi
"$cxx" $role_flags -DPIONEER_QWEN35_DEVICE_DISPATCH -DPIONEER_QWEN35_RECOVERY_COMPILE_ONLY \
	-DPIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS -DPIONEER_QWEN35_PAIRED_RECURRENCE -I"$root/tools/pioneer" -I"$sdk/headers" \
	-idirafter "$sdk/headers/posix" -c "$root/tools/pioneer/PioneerQwen35DataflowServiceImage.cpp" \
	-o "$temporary/role-graph-service.o"
"$cxx" $role_flags -DPIONEER_QWEN35_DEVICE_DISPATCH -DPIONEER_QWEN35_RECOVERY_COMPILE_ONLY \
	-DPIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS -DPIONEER_QWEN35_PAIRED_RECURRENCE \
	-I"$root/tools/pioneer" -I"$sdk/headers" -idirafter "$sdk/headers/posix" \
	-c "$root/tools/pioneer/PioneerQwen35DataflowSharedRoles.cpp" -o "$temporary/role-graph-compute.o"
mkdir "$temporary/role-graph"
for piece in stream barrier; do
	"$cxx" $role_flags -O2 -DPIONEER_QWEN35_DEVICE_DISPATCH -DPIONEER_QWEN35_RECOVERY_COMPILE_ONLY \
		-DPIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS -DPIONEER_QWEN35_PAIRED_RECURRENCE -I"$root/tools/pioneer" -I"$sdk/headers" \
		-idirafter "$sdk/headers/posix" -c "$directory/pioneer_qwen35_dataflow_device_${piece}_target_probe.cpp" \
		-o "$temporary/role-graph/device-${piece}.o"
done
"$cxx" $role_flags -O2 -DPIONEER_QWEN35_DEVICE_DISPATCH -DPIONEER_QWEN35_RECOVERY_COMPILE_ONLY \
	-DPIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS -DPIONEER_QWEN35_PAIRED_RECURRENCE -I"$root/tools/pioneer" -I"$sdk/headers" \
	-idirafter "$sdk/headers/posix" -c "$root/tools/pioneer/PioneerQwen35DataflowDeviceDispatch.cpp" \
	-o "$temporary/role-graph/device-graph.o"
"$ld" -m elf32lriscv --no-relax --gc-sections -u PioneerQwen35DeviceGraphRun \
	-T "$root/tools/pioneer/blackhole_qwen35_dataflow_dispatch_witness.ld" \
	"$temporary/graph-service/start.o" "$temporary/role-graph-service.o" "$temporary/graph-service/memory.o" \
	"$temporary/role-graph/device-stream.o" "$temporary/role-graph/device-barrier.o" \
	"$temporary/role-graph/device-graph.o" "$temporary/role-graph-compute.o" \
	-o "$temporary/role-graph-witness.elf"
if [ -n "$("$nm" -u "$temporary/role-graph-witness.elf")" ]; then
	echo 'shared-role graph witness has unresolved imports' >&2
	exit 1
fi
"$objdump" -d "$temporary/role-graph-witness.elf" > "$temporary/role-graph-witness.disassembly"
if ! grep -E 'jalr.*<qwen35_shared_compute_coordinate>' "$temporary/role-graph-witness.disassembly" >/dev/null; then
	echo 'graph witness does not call the shared-role compute coordinator' >&2
	exit 1
fi
if ! grep -E 'jalr.*<qwen35_resident_stream_cache_bind>' "$temporary/role-graph-witness.disassembly" >/dev/null; then
	echo 'shared-role graph witness does not bind the resident stream cache' >&2
	exit 1
fi
"$size" -A "$temporary/role-graph-witness.elf"
python3 - "$temporary/role-graph-service.su" "$temporary/role-graph-compute.su" \
	"$temporary/role-graph/device-graph.su" <<'PYROLESTACK'
from pathlib import Path
import sys
entries=[]
for name in sys.argv[1:]:
    for line in Path(name).read_text().splitlines():
        parts=line.rsplit('\t',2)
        if len(parts)==3 and parts[1].isdigit():
            entries.append((parts[0],int(parts[1]),parts[2]))
markers=('qwen35_dataflow_service_main()', 'PioneerQwen35DataflowBlackholeWorkerService()',
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
        raise SystemExit('missing or unbounded shared-role graph frame: '+marker)
    total+=max(frame for frame,_ in matches)
if total>8192:
    raise SystemExit(f'shared-role graph stack witness {total} exceeds 8192 bytes')
print(f'PASS shared-role graph stack witness {total}/8192; not whole-program proof')
PYROLESTACK
echo 'PASS actual graph executor links shared-role projection coordinator; NOT LOADABLE OR RUNNING'
if PIONEER_QWEN35_SHARED_ROLE_GRAPH_COMPILE_ONLY=1 \
	sh "$root/tools/pioneer/build_qwen35_dataflow_service_image.sh" "$build" "$temporary/invalid-shared-graph" >/dev/null 2>&1; then
	echo 'shared role graph compiled without the recovery-only guard' >&2
	exit 1
fi
PIONEER_QWEN35_SHARED_ROLE_GRAPH_COMPILE_ONLY=1 PIONEER_QWEN35_RECOVERY_COMPILE_ONLY=1 \
	PIONEER_QWEN35_DEVICE_DISPATCH_COMPILE_ONLY=1 \
	sh "$root/tools/pioneer/build_qwen35_dataflow_service_image.sh" "$build" "$temporary/shared-graph-service"
sh "$root/tools/pioneer/build_qwen35_resident_service_witness.sh" \
    "$build" "$temporary/shared-graph-service" "$temporary/resident-graph-service"
PIONEER_QWEN35_TREE_SETTLEMENT_WITNESS=1 sh "$root/tools/pioneer/build_qwen35_resident_service_witness.sh" \
    "$build" "$temporary/shared-graph-service" "$temporary/resident-tree-graph-service"
for image in "$temporary/shared-role-images/reader.disassembly" \
	"$temporary/shared-graph-service/disassembly.txt"; do
	if ! grep -E '# abe40([[:space:]]|$)' "$image" >/dev/null; then
		echo 'exported shared reader/graph is missing activation reuse metadata' >&2
		exit 1
	fi
done
echo 'PASS exported reader and graph use the shared activation reuse address; OFFLINE ONLY'
python3 "$directory/pioneer_qwen35_dataflow_tensix_source_order_test.py" \
	"$temporary/shared-graph-service/disassembly.txt"
python3 "$directory/pioneer_qwen35_dataflow_noc_fifo_image_test.py" \
	"$temporary/shared-role-images/reader.disassembly" \
	"$temporary/shared-role-images/writer.disassembly" \
	"$temporary/shared-graph-service/disassembly.txt"
for section in boot text dispatch; do
	"$objcopy" -O binary -j ".$section" "$temporary/role-graph-witness.elf" "$temporary/expected-role-graph-$section.bin"
	cmp "$temporary/expected-role-graph-$section.bin" "$temporary/shared-graph-service/$section.bin"
done
echo 'PASS reproducible shared-role graph image export; OFFLINE ONLY'
awk '/PIONEER_Q35_SHARED_AUTO_ARM_BEGIN/ {copying=1;next} /PIONEER_Q35_SHARED_AUTO_ARM_END/ {found=1;exit} copying {print} END {if(!found)exit 1}' \
	"$root/src/add-ons/kernel/drivers/misc/PioneerQwen35DataflowSharedRoleBinding.inc" > "$temporary/pioneer_q35_shared_auto_arm.inc"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
	-I"$temporary" "$directory/pioneer_qwen35_shared_auto_arm_test.cpp" -o "$temporary/shared-auto-arm-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/shared-auto-arm-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
	-I"$directory/role_haiku_stubs" -I"$temporary/shared-role-images" -I"$temporary/shared-graph-service" \
	"$directory/pioneer_qwen35_dataflow_shared_role_binding_test.cpp" -o "$temporary/shared-role-binding-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/shared-role-binding-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
	-DTEST_SHARED_POST_RESET -I"$directory/role_haiku_stubs" -I"$temporary/shared-role-images" -I"$temporary/shared-graph-service" \
	"$directory/pioneer_qwen35_dataflow_shared_role_binding_test.cpp" -o "$temporary/shared-role-reset-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/shared-role-reset-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
	-DTEST_SHARED_POST_RESET -DTEST_SHARED_FABRIC_VIEWS -DPIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS \
	-I"$directory/role_haiku_stubs" -I"$temporary/shared-role-images" -I"$temporary/shared-graph-service" \
	-I"$temporary/local-restore-images" -I"$temporary/role-service-probe-images" \
	"$directory/pioneer_qwen35_dataflow_shared_role_binding_test.cpp" -o "$temporary/fabric-role-binding-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/fabric-role-binding-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
	-DTEST_SHARED_POST_RESET -DPIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS \
	-I"$directory/role_haiku_stubs" -I"$temporary/shared-role-images" -I"$temporary/shared-graph-service" \
	-I"$temporary/local-restore-images" -I"$temporary/role-service-probe-images" "$directory/pioneer_qwen35_dataflow_shared_role_binding_test.cpp" \
	-o "$temporary/core-local-role-binding-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/core-local-role-binding-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
	-DTEST_SHARED_POST_RESET -DTEST_PRESERVATION_ROLLBACK -DPIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS \
	-I"$directory/role_haiku_stubs" -I"$temporary/shared-role-images" -I"$temporary/shared-graph-service" \
	-I"$temporary/local-restore-images" -I"$temporary/role-service-probe-images" \
	"$directory/pioneer_qwen35_dataflow_shared_role_binding_test.cpp" -o "$temporary/preservation-rollback-binding-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/preservation-rollback-binding-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
	-I"$directory/role_haiku_stubs" -I"$temporary/shared-role-images" -I"$temporary/shared-graph-service" \
	-I"$temporary/local-restore-images" -I"$temporary/role-service-probe-images" \
	"$directory/pioneer_qwen35_faulted_stage_service_integration_test.cpp" -o "$temporary/faulted-stage-service-integration-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/faulted-stage-service-integration-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
	-DTEST_SHARED_POST_RESET -DPIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS \
	-I"$directory/role_haiku_stubs" -I"$temporary/shared-role-images" -I"$temporary/shared-graph-service" \
	"$directory/pioneer_qwen35_dataflow_shared_role_binding_test.cpp" -o "$temporary/exclusive-role-binding-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/exclusive-role-binding-test"
awk '/PIONEER_Q35_ROLE_STAGE_END_GUARD_BEGIN/ {copying=1;next} /PIONEER_Q35_ROLE_STAGE_END_GUARD_END/ {found=1;exit} copying {print} END {if(!found)exit 1}' \
    "$root/src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp" > "$temporary/pioneer_q35_role_stage_end_guard.inc"
awk '/PIONEER_Q35_AUTO_RESTORE_SELECT_BEGIN/ {copying=1;next} /PIONEER_Q35_AUTO_RESTORE_SELECT_END/ {found=1;exit} copying {print} END {if(!found)exit 1}' \
    "$root/src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp" > "$temporary/pioneer_q35_auto_restore_select.inc"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
    -I"$temporary" "$directory/pioneer_qwen35_auto_restore_install_test.cpp" -o "$temporary/auto-restore-install-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/auto-restore-install-test"
awk '/PIONEER_Q35_INSTALL_QUALIFICATION_GUARD_BEGIN/ {copying=1;next} /PIONEER_Q35_INSTALL_QUALIFICATION_GUARD_END/ {found=1;exit} copying {print} END {if(!found)exit 1}' \
    "$root/src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp" > "$temporary/pioneer_q35_install_qualification_guard.inc"
awk '/PIONEER_Q35_INSTALL_ROUTE_BEGIN/ {copying=1;next} /PIONEER_Q35_INSTALL_ROUTE_END/ {found=1;exit} copying {print} END {if(!found)exit 1}' \
    "$root/src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp" > "$temporary/pioneer_q35_install_route.inc"
for resident_install in 0 1; do
    resident_flag=
    [ "$resident_install" = 0 ] || resident_flag=-DPIONEER_QWEN35_RESIDENT_SERVICE_WITNESS
    ${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
        $resident_flag -I"$temporary" "$directory/pioneer_qwen35_qualification_install_route_test.cpp" \
        -o "$temporary/qualification-install-route-$resident_install"
    ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/qualification-install-route-$resident_install"
done
awk '/PIONEER_Q35_QUALIFICATION_INSTALL_FAILURE_BEGIN/ {copying=1;next} /PIONEER_Q35_QUALIFICATION_INSTALL_FAILURE_END/ {found=1;exit} copying {print} END {if(!found)exit 1}' \
    "$root/src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp" > "$temporary/pioneer_q35_qualification_install_failure.inc"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
    -I"$temporary" "$directory/pioneer_qwen35_qualification_unstaged_abort_test.cpp" \
    -o "$temporary/qualification-unstaged-abort-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/qualification-unstaged-abort-test"
awk '/PIONEER_Q35_INSTALL_CLEANUP_BEGIN/ {copying=1;next} /PIONEER_Q35_INSTALL_CLEANUP_END/ {found=1;exit} copying {print} END {if(!found)exit 1}' \
    "$root/src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp" > "$temporary/pioneer_q35_install_cleanup.inc"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
    -I"$temporary" "$directory/pioneer_qwen35_install_cleanup_test.cpp" -o "$temporary/install-cleanup-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/install-cleanup-test"
awk '/PIONEER_Q35_DATAFLOW_CLOSE_BEGIN/ {copying=1;next} /PIONEER_Q35_DATAFLOW_CLOSE_END/ {found=1;exit} copying {print} END {if(!found)exit 1}' \
    "$root/src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp" > "$temporary/pioneer_q35_dataflow_close.inc"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
    -I"$temporary" "$directory/pioneer_qwen35_closed_service_cleanup_test.cpp" -o "$temporary/closed-service-cleanup-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/closed-service-cleanup-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
    -DTEST_RESIDENT_DRIVER_CLOSE -I"$temporary" "$directory/pioneer_qwen35_closed_service_cleanup_test.cpp" \
    -o "$temporary/resident-closed-service-cleanup-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/resident-closed-service-cleanup-test"
awk '/PIONEER_Q35_PROJECTION_OWNER_END_GUARD_BEGIN/ {copying=1;next} /PIONEER_Q35_PROJECTION_OWNER_END_GUARD_END/ {found=1;exit} copying {print} END {if(!found)exit 1}' \
    "$root/src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp" > "$temporary/pioneer_q35_projection_owner_end_guard.inc"
awk '/PIONEER_Q35_PROJECTION_OWNER_UNSAFE_GUARD_BEGIN/ {copying=1;next} /PIONEER_Q35_PROJECTION_OWNER_UNSAFE_GUARD_END/ {found=1;exit} copying {print} END {if(!found)exit 1}' \
    "$root/src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp" > "$temporary/pioneer_q35_projection_owner_unsafe_guard.inc"
awk '/PIONEER_Q35_PROJECTION_SERVICE_UNSAFE_GUARD_BEGIN/ {copying=1;next} /PIONEER_Q35_PROJECTION_SERVICE_UNSAFE_GUARD_END/ {found=1;exit} copying {print} END {if(!found)exit 1}' \
    "$root/src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp" > "$temporary/pioneer_q35_projection_service_unsafe_guard.inc"
for snapshot_hold in 0 1; do
    hold_flag=
    [ "$snapshot_hold" = 0 ] || hold_flag=-DTEST_SNAPSHOT_SCRATCH_HOLD
    ${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
        $hold_flag -I"$temporary" "$directory/pioneer_qwen35_projection_service_unsafe_guard_test.cpp" \
        -o "$temporary/projection-service-unsafe-$snapshot_hold"
    ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/projection-service-unsafe-$snapshot_hold"
done
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
    -DTEST_RESIDENT_CLOSE_SCOPE -DTEST_SNAPSHOT_SCRATCH_HOLD -I"$temporary" \
    "$directory/pioneer_qwen35_projection_service_unsafe_guard_test.cpp" -o "$temporary/resident-service-unsafe"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/resident-service-unsafe"
for resident_capture in 0 1; do
    resident_flag=
    [ "$resident_capture" = 0 ] || resident_flag=-DTEST_RESIDENT_CLOSE_SCOPE
    ${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
        -DTEST_SNAPSHOT_SCRATCH_HOLD -DTEST_QUALIFICATION_CAPTURE $resident_flag -I"$temporary" \
        "$directory/pioneer_qwen35_projection_service_unsafe_guard_test.cpp" -o "$temporary/capture-service-unsafe-$resident_capture"
    ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/capture-service-unsafe-$resident_capture"
done
awk '/PIONEER_Q35_PROJECTION_OWNER_RESET_GUARD_BEGIN/ {copying=1;next} /PIONEER_Q35_PROJECTION_OWNER_RESET_GUARD_END/ {found=1;exit} copying {print} END {if(!found)exit 1}' \
    "$root/src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp" > "$temporary/pioneer_q35_projection_owner_reset_guard.inc"
awk '/PIONEER_Q35_PROJECTION_INTERNAL_SCOPE_BEGIN/ {copying=1;next} /PIONEER_Q35_PROJECTION_INTERNAL_SCOPE_END/ {found=1;exit} copying {print} END {if(!found)exit 1}' \
    "$root/src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp" > "$temporary/pioneer_q35_projection_internal_scope.inc"
awk '/PIONEER_Q35_PROJECTION_ROUTE_PREPARE_BEGIN/ {copying=1;next} /PIONEER_Q35_PROJECTION_ROUTE_PREPARE_END/ {found=1;exit} copying {print} END {if(!found)exit 1}' \
    "$root/src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp" > "$temporary/pioneer_q35_projection_route_prepare.inc"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
    -I"$temporary" "$directory/pioneer_qwen35_projection_route_prepare_test.cpp" -o "$temporary/projection-route-prepare-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/projection-route-prepare-test"
awk '/PIONEER_Q35_PROJECTION_SCRATCH_OPERATION_BEGIN/ {copying=1;next} /PIONEER_Q35_PROJECTION_SCRATCH_OPERATION_END/ {found=1;exit} copying {print} END {if(!found)exit 1}' \
    "$root/src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp" > "$temporary/pioneer_q35_projection_scratch_operation.inc"
awk '/PIONEER_Q35_PROJECTION_ABANDON_BEGIN/ {copying=1;next} /PIONEER_Q35_PROJECTION_ABANDON_END/ {found=1;exit} copying {print} END {if(!found)exit 1}' \
    "$root/src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp" > "$temporary/pioneer_q35_projection_abandon.inc"
awk '/PIONEER_Q35_PROJECTION_RUN_BEGIN/ {copying=1;next} /PIONEER_Q35_PROJECTION_RUN_END/ {found=1;exit} copying {print} END {if(!found)exit 1}' \
    "$root/src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp" > "$temporary/pioneer_q35_projection_run.inc"
awk '/PIONEER_Q35_PROJECTION_CLOSE_BEGIN/ {copying=1;next} /PIONEER_Q35_PROJECTION_CLOSE_END/ {found=1;exit} copying {print} END {if(!found)exit 1}' \
    "$root/src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp" > "$temporary/pioneer_q35_projection_close.inc"
awk '/PIONEER_Q35_PROJECTION_FINISH_BEGIN/ {copying=1;next} /PIONEER_Q35_PROJECTION_FINISH_END/ {found=1;exit} copying {print} END {if(!found)exit 1}' \
    "$root/src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp" > "$temporary/pioneer_q35_projection_finish.inc"
for scope_test in RESUME PREPARE; do
    awk -v begin="PIONEER_Q35_SCOPE_${scope_test}_BEGIN" -v end="PIONEER_Q35_SCOPE_${scope_test}_END" \
        'index($0,begin) {copying=1;next} index($0,end) {found=1;exit} copying {print} END {if(!found)exit 1}' \
        "$root/src/add-ons/kernel/drivers/misc/PioneerQwen35ProjectionQualificationScope.inc" \
        > "$temporary/pioneer_q35_scope_$(printf '%s' "$scope_test" | tr '[:upper:]' '[:lower:]').inc"
done
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
    -I"$temporary" "$directory/pioneer_qwen35_scope_retirement_test.cpp" -o "$temporary/scope-retirement-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/scope-retirement-test"
awk '/PIONEER_Q35_PROJECTION_TRANSACTION_BEGIN/ {copying=1;next} /PIONEER_Q35_PROJECTION_TRANSACTION_END/ {found=1;exit} copying {print} END {if(!found)exit 1}' \
    "$root/src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp" > "$temporary/pioneer_q35_projection_transaction.inc"
awk '/PIONEER_Q35_SCOPE_RESET_CONTROL_BEGIN/ {copying=1;next} /PIONEER_Q35_SCOPE_RESET_CONTROL_END/ {found=1;exit} copying {print} END {if(!found)exit 1}' \
    "$root/src/add-ons/kernel/drivers/misc/PioneerQwen35DataflowRoleHaikuScopeWitness.inc" > "$temporary/pioneer_q35_scope_reset_control.inc"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
    -I"$temporary" "$directory/pioneer_qwen35_shared_reset_control_test.cpp" -o "$temporary/shared-reset-control-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/shared-reset-control-test"
awk '/PIONEER_Q35_FAULT_CORE_SETTLED_BEGIN/ {copying=1;next} /PIONEER_Q35_FAULT_CORE_SETTLED_END/ {found=1;exit} copying {print} END {if(!found)exit 1}' \
    "$root/src/add-ons/kernel/drivers/misc/PioneerQwen35DataflowSharedRoleBinding.inc" > "$temporary/pioneer_q35_fault_core_settled.inc"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
    -I"$temporary" "$directory/pioneer_qwen35_fault_role_image_test.cpp" -o "$temporary/fault-role-image-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/fault-role-image-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
    -I"$temporary" "$directory/pioneer_qwen35_projection_transaction_test.cpp" -o "$temporary/projection-transaction-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/projection-transaction-test"
awk '/PIONEER_Q35_PROJECTION_CONTROL_BEGIN/ {copying=1;next} /PIONEER_Q35_PROJECTION_CONTROL_END/ {found=1;exit} copying {print} END {if(!found)exit 1}' \
	"$root/src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp" > "$temporary/pioneer_q35_projection_control.inc"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
	-I"$temporary" "$directory/pioneer_qwen35_projection_control_test.cpp" -o "$temporary/projection-control-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/projection-control-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
    -I"$temporary" "$directory/pioneer_qwen35_projection_finish_context_test.cpp" -o "$temporary/projection-finish-context-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/projection-finish-context-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
    -I"$temporary" "$directory/pioneer_qwen35_projection_owner_end_guard_test.cpp" -o "$temporary/projection-owner-end-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/projection-owner-end-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
    "$directory/pioneer_qwen35_unmodified_reset_recovery_test.cpp" -o "$temporary/unmodified-reset-recovery-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/unmodified-reset-recovery-test"
awk '/PIONEER_Q35_PROJECTION_RESET_FINISH_BEGIN/ {copying=1;next} /PIONEER_Q35_PROJECTION_RESET_FINISH_END/ {found=1;exit} copying {print} END {if(!found)exit 1}' \
    "$root/src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp" > "$temporary/pioneer_q35_projection_reset_finish.inc"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
    -I"$temporary" "$directory/pioneer_qwen35_unmodified_reset_finish_test.cpp" -o "$temporary/unmodified-reset-finish-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/unmodified-reset-finish-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
    "$directory/pioneer_qwen35_projection_reset_restore_test.cpp" -o "$temporary/projection-reset-restore-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/projection-reset-restore-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
    "$directory/pioneer_qwen35_fresh_recovery_session_test.cpp" -o "$temporary/fresh-recovery-session-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/fresh-recovery-session-test"
for test in guarded_mmio_begin guarded_baseline guarded_host_page guarded_role_select guarded_mmio_end; do
    ${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
        "$directory/pioneer_qwen35_${test}_test.cpp" -o "$temporary/$test"
    ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/$test"
done
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
    -I"$directory/role_haiku_stubs" "$directory/pioneer_qwen35_guarded_role_fleet_test.cpp" \
    -o "$temporary/guarded-role-fleet-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/guarded-role-fleet-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
    -I"$directory/role_haiku_stubs" "$directory/pioneer_qwen35_guarded_role_image_test.cpp" \
    -o "$temporary/guarded-role-image-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/guarded-role-image-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
    "$directory/pioneer_qwen35_fabric_core_sequence_test.cpp" -o "$temporary/fabric-core-sequence-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/fabric-core-sequence-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/fabric-core-sequence-test" all
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
    "$directory/pioneer_qwen35_guarded_host_audits_test.cpp" -o "$temporary/guarded-host-audits-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/guarded-host-audits-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
    "$directory/pioneer_qwen35_guarded_lease_end_test.cpp" -o "$temporary/guarded-lease-end-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/guarded-lease-end-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
    "$directory/pioneer_qwen35_guarded_store_page_test.cpp" -o "$temporary/guarded-store-page-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/guarded-store-page-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
    -I"$temporary" "$directory/pioneer_qwen35_modified_reset_retirement_test.cpp" -o "$temporary/modified-reset-retirement-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/modified-reset-retirement-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
    "$directory/pioneer_qwen35_modified_recovery_transaction_test.cpp" -o "$temporary/modified-recovery-transaction-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/modified-recovery-transaction-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
    "$directory/pioneer_qwen35_postreset_l2_audit_test.cpp" -o "$temporary/postreset-l2-audit-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/postreset-l2-audit-test"
if ${CXX:-c++} -x c++ -std=c++17 -E -DPIONEER_QWEN35_RECOVERY_COMPILE_ONLY \
    -DPIONEER_QWEN35_DEVICE_DISPATCH -DPIONEER_QWEN35_RECOVERY_RUN_ENABLE \
    -DPIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS -DPIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS \
    -DPIONEER_QWEN35_RECOVERY_QUALIFY \
    "$root/src/add-ons/kernel/drivers/misc/PioneerQwen35ModifiedRecoveryTransaction.inc" \
    >"$temporary/modified-recovery-guard.out" 2>"$temporary/modified-recovery-guard.err"; then
    echo 'loadable qualification accepted modified recovery transaction witness' >&2
    exit 1
fi
grep -q 'Modified Q35 recovery transaction requires the non-loadable snapshot witness' "$temporary/modified-recovery-guard.err"
awk '/PIONEER_Q35_FRESH_RECOVERY_FINISH_GUARD_BEGIN/ {copying=1;next} /PIONEER_Q35_FRESH_RECOVERY_FINISH_GUARD_END/ {found=1;exit} copying {print} END {if(!found)exit 1}' \
    "$root/src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp" > "$temporary/pioneer_q35_fresh_recovery_finish_guard.inc"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
    -I"$temporary" "$directory/pioneer_qwen35_fresh_recovery_finish_guard_test.cpp" -o "$temporary/fresh-recovery-finish-guard-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/fresh-recovery-finish-guard-test"
if ${CXX:-c++} -x c++ -std=c++17 -E -DPIONEER_QWEN35_RECOVERY_COMPILE_ONLY \
    -DPIONEER_QWEN35_DEVICE_DISPATCH -DPIONEER_QWEN35_RECOVERY_RUN_ENABLE \
    -DPIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS -DPIONEER_QWEN35_RECOVERY_QUALIFY \
    "$root/src/add-ons/kernel/drivers/misc/PioneerQwen35FreshRecoverySession.inc" \
    >"$temporary/fresh-session-guard.out" 2>"$temporary/fresh-session-guard.err"; then
    echo 'loadable qualification accepted fresh recovery session witness' >&2
    exit 1
fi
grep -q 'Fresh Q35 reset transfer session is restricted to the non-loadable snapshot witness' "$temporary/fresh-session-guard.err"
if ${CXX:-c++} -x c++ -std=c++17 -E -DPIONEER_QWEN35_RECOVERY_COMPILE_ONLY \
    -DPIONEER_QWEN35_DEVICE_DISPATCH -DPIONEER_QWEN35_RECOVERY_RUN_ENABLE \
    -DPIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS -DPIONEER_QWEN35_FRESH_RECOVERY_TEST -D_KERNEL_MODE \
    "$root/src/add-ons/kernel/drivers/misc/PioneerQwen35FreshRecoverySession.inc" \
    >"$temporary/fresh-session-test-guard.out" 2>"$temporary/fresh-session-test-guard.err"; then
    echo 'kernel build accepted simulated fresh-session disposal' >&2
    exit 1
fi
grep -q 'Simulated fresh-session disposal is not permitted in a kernel build' "$temporary/fresh-session-test-guard.err"
if ${CXX:-c++} -x c++ -std=c++17 -E -DPIONEER_QWEN35_RECOVERY_COMPILE_ONLY \
    -DPIONEER_QWEN35_DEVICE_DISPATCH -DPIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS \
    -DPIONEER_QWEN35_RECOVERY_QUALIFY \
    "$root/src/add-ons/kernel/drivers/misc/PioneerQwen35ProjectionResetRecovery.h" \
    >"$temporary/reset-restore-guard.out" 2>"$temporary/reset-restore-guard.err"; then
    echo 'loadable qualification accepted reset scratch restore witness' >&2
    exit 1
fi
if ! grep -q 'Projection reset restore is restricted to the non-loadable snapshot witness' "$temporary/reset-restore-guard.err"; then
    echo 'reset scratch restore rejection did not come from its qualification guard' >&2
    exit 1
fi
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
    -I"$temporary" "$directory/pioneer_qwen35_projection_scratch_context_test.cpp" -o "$temporary/projection-scratch-context-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/projection-scratch-context-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
    -I"$temporary" "$directory/pioneer_qwen35_projection_haiku_scope_test.cpp" -o "$temporary/projection-haiku-scope-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/projection-haiku-scope-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
    -I"$temporary" "$directory/pioneer_qwen35_qualification_install_observation_test.cpp" \
    -o "$temporary/qualification-install-observation-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/qualification-install-observation-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
    -I"$directory/role_haiku_stubs" -I"$temporary/shared-role-images" -I"$temporary/shared-graph-service" -I"$temporary" \
    "$directory/pioneer_qwen35_role_stage_control_test.cpp" -o "$temporary/role-stage-control-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/role-stage-control-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
    -I"$directory/role_haiku_stubs" -I"$temporary/shared-role-images" -I"$temporary/shared-graph-service" -I"$temporary" \
    "$directory/pioneer_qwen35_role_prestack_control_test.cpp" -o "$temporary/role-prestack-control-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/role-prestack-control-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
    -I"$directory/role_haiku_stubs" -I"$temporary/shared-role-images" -I"$temporary/shared-graph-service" \
    -I"$temporary/local-restore-probe-images" -I"$temporary" \
    "$directory/pioneer_qwen35_role_local_restore_control_test.cpp" -o "$temporary/role-local-restore-control-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/role-local-restore-control-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
    -I"$directory/role_haiku_stubs" -I"$temporary/shared-role-images" -I"$temporary/shared-graph-service" \
    -I"$temporary/local-restore-probe-images" -I"$temporary/role-service-probe-images" -I"$temporary" \
    "$directory/pioneer_qwen35_role_service_control_test.cpp" -o "$temporary/role-service-control-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/role-service-control-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
    -I"$directory/role_haiku_stubs" -I"$temporary/shared-role-images" -I"$temporary/shared-graph-service" \
    -I"$temporary/local-restore-probe-images" -I"$temporary/role-service-probe-images" -I"$temporary" \
    "$directory/pioneer_qwen35_role_fleet_service_control_test.cpp" -o "$temporary/role-fleet-service-control-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/role-fleet-service-control-test"
if ${CXX:-c++} -x c++ -std=c++17 -E -D_KERNEL_MODE -DPIONEER_QWEN35_ROLE_FLEET_SERVICE_QUALIFY \
    -DPIONEER_QWEN35_ROLE_SERVICE_QUALIFY -DPIONEER_QWEN35_ROLE_PRESTACK_QUALIFY \
    -DPIONEER_QWEN35_ROLE_STAGE_QUALIFY -I"$directory/role_haiku_stubs" \
    "$root/src/add-ons/kernel/drivers/misc/PioneerQwen35RolePrestackProbe.h" \
    >"$temporary/fleet-service-guard.out" 2>"$temporary/fleet-service-guard.err"; then
    echo 'unqualified fleet service accepted a kernel build' >&2
    exit 1
fi
if ! grep -q 'Fleet service startup requires its restricted qualification build' "$temporary/fleet-service-guard.err"; then
    echo 'fleet service kernel rejection did not come from its qualification guard' >&2
    exit 1
fi
if ${CXX:-c++} -x c++ -std=c++17 -E -DPIONEER_QWEN35_RECOVERY_COMPILE_ONLY \
    -DPIONEER_QWEN35_DEVICE_DISPATCH -DPIONEER_QWEN35_RECOVERY_QUALIFY \
    -I"$directory/role_haiku_stubs" "$root/src/add-ons/kernel/drivers/misc/PioneerQwen35ProjectionScratchCompileWitness.inc" \
    >"$temporary/projection-witness-guard.out" 2>"$temporary/projection-witness-guard.err"; then
    echo 'loadable qualification accepted the projection compile witness' >&2
    exit 1
fi
if ! grep -q 'Projection scratch compile witness must not be included' "$temporary/projection-witness-guard.err"; then
    echo 'projection witness rejection did not come from its qualification guard' >&2
    exit 1
fi
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
    -I"$directory/role_haiku_stubs" -I"$temporary/shared-role-images" -I"$temporary/shared-graph-service" \
    -I"$temporary/local-restore-probe-images" -I"$temporary/role-service-probe-images" -I"$temporary" \
    "$directory/pioneer_qwen35_role_fault_control_test.cpp" -o "$temporary/role-fault-control-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/role-fault-control-test"
if ${CXX:-c++} -std=c++17 -E -D_KERNEL_MODE -DPIONEER_QWEN35_ROLE_FAULT_QUALIFY \
    -DPIONEER_QWEN35_ROLE_SERVICE_QUALIFY -DPIONEER_QWEN35_ROLE_PRESTACK_QUALIFY \
    -DPIONEER_QWEN35_ROLE_STAGE_QUALIFY -I"$root/src/add-ons/kernel/drivers/misc" -x c++ - \
    >"$temporary/role-fault-kernel.i" 2>"$temporary/role-fault-kernel.log" <<'EOF'
#include "PioneerQwen35RolePrestackProbe.h"
EOF
then
    echo 'offline fault qualification unexpectedly admitted kernel compilation' >&2
    exit 1
fi
if ! grep -q 'Fault qualification requires its separately tagged resident service build' "$temporary/role-fault-kernel.log"; then
    cat "$temporary/role-fault-kernel.log" >&2
    exit 1
fi
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
	-fsanitize=address,undefined "$directory/pioneer_qwen35_dataflow_noc_read_pipeline_test.cpp" \
	-o "$temporary/noc-read-pipeline-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/noc-read-pipeline-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror \
	-fsanitize=address,undefined "$directory/pioneer_qwen35_dataflow_noc1_audit_test.cpp" \
	-o "$temporary/noc1-audit-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/noc1-audit-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror \
	-fsanitize=address,undefined "$directory/pioneer_qwen35_dataflow_device_barrier_test.cpp" \
	-o "$temporary/device-barrier-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/device-barrier-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror \
	-fsanitize=address,undefined "$directory/pioneer_qwen35_dataflow_shared_barrier_test.cpp" \
	-o "$temporary/shared-barrier-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/shared-barrier-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror \
	-fsanitize=address,undefined "$directory/pioneer_qwen35_dataflow_device_stream_test.cpp" \
	-o "$temporary/device-stream-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/device-stream-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror \
	-fsanitize=address,undefined "$directory/pioneer_qwen35_dataflow_device_graph_test.cpp" \
	-o "$temporary/device-graph-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/device-graph-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror \
	-DPIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS -fsanitize=address,undefined \
	"$directory/pioneer_qwen35_dataflow_device_graph_test.cpp" -o "$temporary/shared-device-graph-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/shared-device-graph-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror \
	-fsanitize=address,undefined "$directory/pioneer_qwen35_dataflow_stream_recorder_test.cpp" \
	-o "$temporary/stream-recorder-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/stream-recorder-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -Wno-misleading-indentation \
    -fsanitize=address,undefined "$directory/pioneer_qwen35_projection_pair_test.cpp" \
    -o "$temporary/projection-pair-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/projection-pair-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -Wno-misleading-indentation \
    -fsanitize=address,undefined "$directory/pioneer_qwen35_projection_pair_graph_test.cpp" \
    -o "$temporary/projection-pair-graph-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/projection-pair-graph-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror \
	-fsanitize=address,undefined "$directory/pioneer_qwen35_stream_bank_sink_test.cpp" \
	-o "$temporary/stream-bank-sink-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/stream-bank-sink-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror \
	-fsanitize=address,undefined "$directory/pioneer_qwen35_projection_scratch_test.cpp" \
	-o "$temporary/projection-scratch-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/projection-scratch-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror \
	-fsanitize=address,undefined "$directory/pioneer_qwen35_stream_recording_integration_test.cpp" \
	-o "$temporary/stream-recording-integration-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/stream-recording-integration-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
    -DPIONEER_QWEN35_RECOVERY_COMPILE_ONLY -DPIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS \
    -DPIONEER_QWEN35_ACTIVATION_SHARING_WITNESS "$directory/pioneer_qwen35_stream_recording_integration_test.cpp" \
    -o "$temporary/sharing-stream-recording-integration-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/sharing-stream-recording-integration-test"
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
	-fsanitize=address,undefined "$directory/pioneer_qwen35_resident_dispatch_test.cpp" \
	-o "$temporary/resident-dispatch-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/resident-dispatch-test"
python3 -B "$directory/pioneer_qwen35_resident_service_header_test.py"
python3 -B "$directory/pioneer_qwen35_qualification_service_header_test.py"
python3 -B "$directory/pioneer_qwen35_selected_service_image_test.py"
python3 -B "$directory/pioneer_qwen35_resident_graph_owner_test.py"
python3 -B "$directory/pioneer_qwen35_resident_model_transaction_test.py"
python3 -B "$directory/pioneer_qwen35_resident_graph_session_test.py"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
	"$directory/pioneer_qwen35_rms_save_graph_test.cpp" -o "$temporary/fused-rms-save-graph-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/fused-rms-save-graph-test"
python3 -B "$directory/pioneer_qwen35_resident_service_ownership_test.py"
python3 -B "$directory/pioneer_qwen35_resident_close_test.py"
python3 -B "$directory/pioneer_qwen35_resident_owned_runner_test.py"
python3 -B "$directory/pioneer_qwen35_resident_driver_owner_test.py"
python3 -B "$directory/pioneer_qwen35_resident_driver_close_fault_test.py"
python3 -B "$directory/pioneer_qwen35_resident_driver_cache_test.py"
python3 -B "$directory/pioneer_qwen35_resident_driver_recovery_test.py"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
	"$directory/pioneer_qwen35_resident_fleet_audit_test.cpp" -o "$temporary/resident-fleet-audit-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/resident-fleet-audit-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
	-DPIONEER_QWEN35_PAIRED_RECURRENCE "$directory/pioneer_qwen35_resident_fleet_audit_test.cpp" \
	-o "$temporary/resident-fleet-audit-paired-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/resident-fleet-audit-paired-test"
python3 -B "$directory/pioneer_qwen35_resident_host_fleet_audit_test.py"
for mode in baseline tree; do
    version_flags=
    [ "$mode" = baseline ] || version_flags=-DPIONEER_QWEN35_TREE_SETTLEMENT
    ${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined $version_flags \
        "$directory/pioneer_qwen35_resident_candidate_version_test.cpp" -o "$temporary/resident-candidate-$mode-test"
    ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/resident-candidate-$mode-test"
done
python3 -B "$directory/pioneer_qwen35_resident_tree_audit_test.py"
python3 -B "$directory/pioneer_qwen35_resident_tree_host_test.py"
python3 -B "$directory/pioneer_qwen35_resident_compact_host_test.py"
python3 -B "$directory/pioneer_qwen35_resident_candidate_poll_test.py"
python3 -B "$directory/pioneer_qwen35_resident_fleet_audit_pci_test.py"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
	"$directory/pioneer_qwen35_resident_sender_receipt_test.cpp" -o "$temporary/resident-sender-receipt-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/resident-sender-receipt-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
	"$directory/pioneer_qwen35_resident_sender_pci_range_test.cpp" -o "$temporary/resident-sender-range-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/resident-sender-range-test"
python3 -B "$directory/pioneer_qwen35_resident_sender_fleet_test.py"
"$cxx" -std=c++17 -Os -Wall -Wextra -Werror -ffreestanding -fno-exceptions -fno-rtti \
	-fno-stack-protector -fno-pic -fno-pie -fno-builtin -ffp-contract=off -ffunction-sections -fdata-sections \
	-fno-unwind-tables -fno-asynchronous-unwind-tables -msmall-data-limit=0 -mno-relax -march=rv32imf_zicsr -mabi=ilp32 \
	-I"$sdk/headers" -idirafter "$sdk/headers/posix" -c "$directory/pioneer_qwen35_resident_sender_receipt_target_probe.cpp" \
	-o "$temporary/resident-sender-receipt-target.o"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
	-DPIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS -DPIONEER_QWEN35_PAIRED_RECURRENCE \
	"$directory/pioneer_qwen35_resident_dispatch_test.cpp" -o "$temporary/resident-dispatch-paired-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/resident-dispatch-paired-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
	"$directory/pioneer_qwen35_resident_lifecycle_test.cpp" -o "$temporary/resident-lifecycle-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/resident-lifecycle-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
	-DPIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS -DPIONEER_QWEN35_PAIRED_RECURRENCE \
	"$directory/pioneer_qwen35_resident_lifecycle_test.cpp" -o "$temporary/resident-lifecycle-paired-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/resident-lifecycle-paired-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
	"$directory/pioneer_qwen35_resident_worker_test.cpp" -o "$temporary/resident-worker-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/resident-worker-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
	"$directory/pioneer_qwen35_resident_host_pci_test.cpp" -o "$temporary/resident-host-pci-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/resident-host-pci-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
	"$directory/pioneer_qwen35_resident_template_installer_test.cpp" -o "$temporary/resident-template-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/resident-template-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
	"$directory/pioneer_qwen35_resident_template_service_hook_test.cpp" -o "$temporary/resident-template-hook-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/resident-template-hook-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
	"$directory/pioneer_qwen35_resident_template_service_integration_test.cpp" -o "$temporary/resident-template-integration-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/resident-template-integration-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
	"$directory/pioneer_qwen35_resident_service_test.cpp" -o "$temporary/resident-service-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/resident-service-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
	-DPIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS -DPIONEER_QWEN35_PAIRED_RECURRENCE \
	"$directory/pioneer_qwen35_resident_service_test.cpp" -o "$temporary/resident-service-paired-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/resident-service-paired-test"
for variant in ordinary paired; do
    tree_flags=
    [ "$variant" = ordinary ] || tree_flags=-DPIONEER_QWEN35_PAIRED_RECURRENCE
    ${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
        -DPIONEER_QWEN35_TREE_SETTLEMENT -DPIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS $tree_flags \
        "$directory/pioneer_qwen35_resident_service_test.cpp" -o "$temporary/resident-tree-$variant-test"
    ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/resident-tree-$variant-test"
done
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
    "$directory/pioneer_qwen35_tree_receipt_bounds_test.cpp" -o "$temporary/resident-tree-bounds-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/resident-tree-bounds-test"
python3 -B "$directory/pioneer_qwen35_tree_build_guard_test.py"
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
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
	-DPIONEER_QWEN35_RECOVERY_COMPILE_ONLY -DPIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS \
	-DPIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS "$directory/pioneer_qwen35_graph_service_owner_test.cpp" \
	-o "$temporary/snapshot-graph-service-owner-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/snapshot-graph-service-owner-test"
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
	-fsanitize=address,undefined "$directory/pioneer_qwen35_dataflow_shared_service_owner_test.cpp" \
	-o "$temporary/shared-service-owner-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/shared-service-owner-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -pthread \
	-fsanitize=address,undefined "$directory/pioneer_qwen35_dataflow_projection_service_integration_test.cpp" \
	-o "$temporary/projection-service-integration-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/projection-service-integration-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -pthread \
	-fsanitize=address,undefined "$directory/pioneer_qwen35_projection_qualification_service_test.cpp" \
	-o "$temporary/projection-qualification-service-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/projection-qualification-service-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -pthread -fsanitize=address,undefined \
	"$directory/pioneer_qwen35_projection_route_integration_test.cpp" -o "$temporary/projection-route-integration-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/projection-route-integration-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -pthread \
	-fsanitize=address,undefined -I"$temporary" "$directory/pioneer_qwen35_projection_scratch_service_integration_test.cpp" \
	-o "$temporary/projection-scratch-service-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/projection-scratch-service-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -pthread -fsanitize=address,undefined \
	-DPIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS -I"$temporary" \
	"$directory/pioneer_qwen35_projection_scratch_service_integration_test.cpp" \
	-o "$temporary/projection-scratch-held-service-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/projection-scratch-held-service-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
	"$directory/pioneer_qwen35_scratch_hold_service_test.cpp" -o "$temporary/scratch-hold-service-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/scratch-hold-service-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror \
	-fsanitize=address,undefined "$directory/pioneer_qwen35_dataflow_shared_service_recovery_test.cpp" \
	-o "$temporary/shared-service-recovery-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/shared-service-recovery-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
	-DPIONEER_QWEN35_EXPORTED_SHARED_IMAGES -I"$temporary/shared-role-images" \
	"$directory/pioneer_qwen35_dataflow_shared_service_recovery_test.cpp" \
	-o "$temporary/shared-exported-images-recovery-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/shared-exported-images-recovery-test"
echo 'PASS exported reader/writer images staged and restored on all 120 simulated tiles; NO HARDWARE'
shared_owner_flags='-std=gnu++17 -O2 -Wall -Wextra -Werror -fno-exceptions -fno-rtti -ffreestanding -fstack-usage -mcmodel=medany -fpic -D_KERNEL_MODE -DPIONEER_QWEN35_DEVICE_DISPATCH -DPIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS'
for probe in model session owned_runner fleet_audit tree_audit close; do
	"$cxx" $shared_owner_flags -DPIONEER_QWEN35_RECOVERY_COMPILE_ONLY -DPIONEER_QWEN35_PAIRED_RECURRENCE \
		-DPIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS -DPIONEER_QWEN35_RESIDENT_SERVICE_WITNESS \
		-I"$sdk/headers" -I"$sdk/headers/os" -I"$sdk/headers/os/support" -idirafter "$sdk/headers/posix" \
		-c "$directory/pioneer_qwen35_resident_${probe}_target_probe.cpp" -o "$temporary/resident-${probe}-target.o"
done
echo 'PASS resident session and model host transport Haiku SDK compile; NO LIVE DRIVER BINDING'
if "$cxx" $shared_owner_flags -I"$sdk/headers" -I"$sdk/headers/os" -I"$sdk/headers/os/support" -idirafter "$sdk/headers/posix" \
	-c "$directory/pioneer_qwen35_dataflow_shared_service_kernel_probe.cpp" \
	-o "$temporary/invalid-shared-service-kernel.o" >"$temporary/invalid-shared-service-kernel.log" 2>&1; then
	echo 'shared service owner compiled without the recovery-only guard' >&2
	exit 1
fi
"$cxx" $shared_owner_flags -DPIONEER_QWEN35_RECOVERY_COMPILE_ONLY \
	-I"$sdk/headers" -I"$sdk/headers/os" -I"$sdk/headers/os/support" -idirafter "$sdk/headers/posix" \
	-c "$directory/pioneer_qwen35_dataflow_shared_service_kernel_probe.cpp" \
	-o "$temporary/shared-service-kernel.o"
awk -F '\t' 'NF==3 {if($2+0>2048||$3!="static")bad=1;seen=1} END{exit !(seen&&!bad)}' \
	"$temporary/shared-service-kernel.su"
echo 'PASS shared service lifecycle Haiku kernel SDK compile; NO LIVE DRIVER BINDING'
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror \
	-fsanitize=address,undefined -DPIONEER_QWEN35_DEVICE_DISPATCH \
	"$directory/pioneer_qwen35_dataflow_pci_memory_test.cpp" -o "$temporary/device-dispatch-pci-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/device-dispatch-pci-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
	-DPIONEER_QWEN35_DEVICE_DISPATCH -DPIONEER_QWEN35_RECOVERY_COMPILE_ONLY \
	-DPIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS \
	"$directory/pioneer_qwen35_dataflow_pci_memory_test.cpp" -o "$temporary/shared-role-pci-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/shared-role-pci-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
	-DPIONEER_QWEN35_DEVICE_DISPATCH "$directory/pioneer_qwen35_compact_service_owner_test.cpp" \
	-o "$temporary/compact-service-owner-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/compact-service-owner-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
	-DPIONEER_QWEN35_DEVICE_DISPATCH -DPIONEER_QWEN35_RECOVERY_COMPILE_ONLY \
	-DPIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS "$directory/pioneer_qwen35_compact_service_owner_test.cpp" \
	-o "$temporary/compact-shared-service-owner-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/compact-shared-service-owner-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
	-DPIONEER_QWEN35_RECOVERY_COMPILE_ONLY -DPIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS \
	"$directory/pioneer_qwen35_graph_descriptor_update_test.cpp" -o "$temporary/shared-role-descriptor-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/shared-role-descriptor-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
	"$directory/pioneer_qwen35_span_qualification_test.cpp" -o "$temporary/span-qualification-client-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/span-qualification-client-test"
awk '/PIONEER_Q35_PANEL_MAPPING_TEST_BEGIN/ {copying=1;next} /PIONEER_Q35_PANEL_MAPPING_TEST_END/ {found=1;exit} copying {print} END {if(!found)exit 1}' \
	"$root/src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp" > "$temporary/pioneer_q35_panel_mapping.inc"
awk '/PIONEER_Q35_BANK_PREALLOC_TEST_BEGIN/ {copying=1;next} /PIONEER_Q35_BANK_PREALLOC_TEST_END/ {found=1;exit} copying {print} END {if(!found)exit 1}' \
	"$root/src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp" > "$temporary/pioneer_q35_bank_prealloc.inc"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
	-I"$temporary" "$directory/pioneer_qwen35_span_panel_mapping_test.cpp" -o "$temporary/span-panel-mapping-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/span-panel-mapping-test"
awk '/PIONEER_Q35_MODEL_PRELOAD_SESSION_BEGIN/ {copying=1;next} /PIONEER_Q35_MODEL_PRELOAD_SESSION_END/ {found=1;exit} copying {print} END {if(!found)exit 1}' \
	"$directory/../pioneer_qwen35_dataflow.cpp" > "$temporary/pioneer_q35_model_preload_session.inc"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
	-I"$temporary" "$directory/pioneer_qwen35_model_preload_session_test.cpp" -o "$temporary/model-preload-session-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/model-preload-session-test"
awk '/PIONEER_Q35_BEGIN_CLIENT_BEGIN/ {copying=1;next} /PIONEER_Q35_BEGIN_CLIENT_END/ {found=1;exit} copying {print} END {if(!found)exit 1}' \
	"$directory/../pioneer_qwen35_dataflow.cpp" > "$temporary/pioneer_q35_begin_client.inc"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
	-I"$directory/cli-stubs" -I"$temporary" "$directory/pioneer_qwen35_preload_begin_test.cpp" -o "$temporary/preload-begin-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/preload-begin-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
	-I"$directory/cli-stubs" "$directory/pioneer_qwen35_span_exclusive_client_test.cpp" -o "$temporary/span-exclusive-client-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/span-exclusive-client-test"
python3 "$directory/pioneer_qwen35_span_qualification_guard_test.py" "$build"
sh "$directory/run_pioneer_qwen35_span_image_tests.sh" "$build"
echo 'PASS: Qwen35 dataflow roles compile as RV32IMF without A atomics or runtime helpers'
