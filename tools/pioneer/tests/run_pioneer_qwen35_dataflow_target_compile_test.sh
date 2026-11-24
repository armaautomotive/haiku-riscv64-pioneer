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
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -pthread \
	-fsanitize=address,undefined "$directory/pioneer_qwen35_dataflow_program_test.cpp" \
	-o "$temporary/dataflow-program-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/dataflow-program-test"
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
"$cxx" $role_flags -I"$root/tools/pioneer" -I"$sdk/headers" -idirafter "$sdk/headers/posix" \
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
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
	-I"$directory/role_haiku_stubs" -I"$temporary/shared-role-images" -I"$temporary/shared-graph-service" \
	"$directory/pioneer_qwen35_dataflow_shared_role_binding_test.cpp" -o "$temporary/shared-role-binding-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/shared-role-binding-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
	-DTEST_SHARED_POST_RESET -I"$directory/role_haiku_stubs" -I"$temporary/shared-role-images" -I"$temporary/shared-graph-service" \
	"$directory/pioneer_qwen35_dataflow_shared_role_binding_test.cpp" -o "$temporary/shared-role-reset-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/shared-role-reset-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
	-DTEST_SHARED_POST_RESET -DPIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS \
	-I"$directory/role_haiku_stubs" -I"$temporary/shared-role-images" -I"$temporary/shared-graph-service" \
	-I"$temporary/local-restore-images" -I"$temporary/role-service-probe-images" "$directory/pioneer_qwen35_dataflow_shared_role_binding_test.cpp" \
	-o "$temporary/core-local-role-binding-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/core-local-role-binding-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
	-DTEST_SHARED_POST_RESET -DPIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS \
	-I"$directory/role_haiku_stubs" -I"$temporary/shared-role-images" -I"$temporary/shared-graph-service" \
	"$directory/pioneer_qwen35_dataflow_shared_role_binding_test.cpp" -o "$temporary/exclusive-role-binding-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/exclusive-role-binding-test"
awk '/PIONEER_Q35_ROLE_STAGE_END_GUARD_BEGIN/ {copying=1;next} /PIONEER_Q35_ROLE_STAGE_END_GUARD_END/ {found=1;exit} copying {print} END {if(!found)exit 1}' \
    "$root/src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp" > "$temporary/pioneer_q35_role_stage_end_guard.inc"
awk '/PIONEER_Q35_PROJECTION_OWNER_END_GUARD_BEGIN/ {copying=1;next} /PIONEER_Q35_PROJECTION_OWNER_END_GUARD_END/ {found=1;exit} copying {print} END {if(!found)exit 1}' \
    "$root/src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp" > "$temporary/pioneer_q35_projection_owner_end_guard.inc"
awk '/PIONEER_Q35_PROJECTION_OWNER_UNSAFE_GUARD_BEGIN/ {copying=1;next} /PIONEER_Q35_PROJECTION_OWNER_UNSAFE_GUARD_END/ {found=1;exit} copying {print} END {if(!found)exit 1}' \
    "$root/src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp" > "$temporary/pioneer_q35_projection_owner_unsafe_guard.inc"
awk '/PIONEER_Q35_PROJECTION_OWNER_RESET_GUARD_BEGIN/ {copying=1;next} /PIONEER_Q35_PROJECTION_OWNER_RESET_GUARD_END/ {found=1;exit} copying {print} END {if(!found)exit 1}' \
    "$root/src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp" > "$temporary/pioneer_q35_projection_owner_reset_guard.inc"
awk '/PIONEER_Q35_PROJECTION_INTERNAL_SCOPE_BEGIN/ {copying=1;next} /PIONEER_Q35_PROJECTION_INTERNAL_SCOPE_END/ {found=1;exit} copying {print} END {if(!found)exit 1}' \
    "$root/src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp" > "$temporary/pioneer_q35_projection_internal_scope.inc"
awk '/PIONEER_Q35_PROJECTION_SCRATCH_OPERATION_BEGIN/ {copying=1;next} /PIONEER_Q35_PROJECTION_SCRATCH_OPERATION_END/ {found=1;exit} copying {print} END {if(!found)exit 1}' \
    "$root/src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp" > "$temporary/pioneer_q35_projection_scratch_operation.inc"
awk '/PIONEER_Q35_PROJECTION_ABANDON_BEGIN/ {copying=1;next} /PIONEER_Q35_PROJECTION_ABANDON_END/ {found=1;exit} copying {print} END {if(!found)exit 1}' \
    "$root/src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp" > "$temporary/pioneer_q35_projection_abandon.inc"
awk '/PIONEER_Q35_PROJECTION_RUN_BEGIN/ {copying=1;next} /PIONEER_Q35_PROJECTION_RUN_END/ {found=1;exit} copying {print} END {if(!found)exit 1}' \
    "$root/src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp" > "$temporary/pioneer_q35_projection_run.inc"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
    -I"$temporary" "$directory/pioneer_qwen35_projection_owner_end_guard_test.cpp" -o "$temporary/projection-owner-end-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/projection-owner-end-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
    -I"$temporary" "$directory/pioneer_qwen35_projection_scratch_context_test.cpp" -o "$temporary/projection-scratch-context-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/projection-scratch-context-test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
    -I"$temporary" "$directory/pioneer_qwen35_projection_haiku_scope_test.cpp" -o "$temporary/projection-haiku-scope-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/projection-haiku-scope-test"
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
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -pthread \
	-fsanitize=address,undefined -I"$temporary" "$directory/pioneer_qwen35_projection_scratch_service_integration_test.cpp" \
	-o "$temporary/projection-scratch-service-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/projection-scratch-service-test"
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
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
	-I"$directory/cli-stubs" "$directory/pioneer_qwen35_span_exclusive_client_test.cpp" -o "$temporary/span-exclusive-client-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/span-exclusive-client-test"
python3 "$directory/pioneer_qwen35_span_qualification_guard_test.py" "$build"
sh "$directory/run_pioneer_qwen35_span_image_tests.sh" "$build"
echo 'PASS: Qwen35 dataflow roles compile as RV32IMF without A atomics or runtime helpers'
