#!/bin/sh
# Offline cross-build only; the witness rejects controller initialization and
# its recovery module refuses requests. Never installs or touches hardware.
set -eu
[ "$#" = 2 ] || exit 2
build=$(CDPATH= cd -- "$1" && pwd)
output=$2
test ! -e "$output" && test ! -L "$output"
mkdir -m 700 "$output"
output=$(CDPATH= cd -- "$output" && pwd)
project=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
sdk=$build/objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop
release=$build/objects/haiku/riscv64/release
cxx=$build/cross-tools-riscv64/bin/riscv64-unknown-haiku-g++
nm=$build/cross-tools-riscv64/bin/riscv64-unknown-haiku-nm
objdump=$build/cross-tools-riscv64/bin/riscv64-unknown-haiku-objdump
test -x "$cxx" && test -f "$release/system/kernel/kernel.so"
# SDK precedes the source tree because some hosts use case-insensitive file
# systems: ACPI.h and the kernel's distinct acpi.h must remain distinguishable.
set -- -isystem "$sdk/headers/os/drivers" \
 -isystem "$project/headers/private/kernel" -isystem "$project/headers/private/kernel/arch/riscv64" \
 -isystem "$project/headers/private/kernel/arch/generic" -isystem "$project/headers/private/kernel/boot/platform/efi" \
 -isystem "$project/headers/private" -isystem "$project/headers/private/drivers" \
 -isystem "$project/headers/private/system" -isystem "$project/headers/private/system/arch/riscv64" \
 -isystem "$project/build/config_headers" -isystem "$project/headers/private/shared" \
 -I"$project/src/system/kernel/arch/generic" -I"$release/system/kernel" \
 -I"$sdk/headers" -idirafter "$sdk/headers/posix"
for include in "$sdk/headers/os" "$sdk"/headers/os/*; do
 [ ! -d "$include" ] || set -- "$@" -isystem "$include"
done
for mode in control witness; do
 mkdir -m 700 "$output/$mode"
 flag=
 [ "$mode" != witness ] || flag=-DPIONEER_SG2042_RECOVERY_WITNESS
 for source in ECAMPCIController ECAMPCIControllerFDT ECAMPCIControllerACPI kernel_interface; do
  "$cxx" -std=gnu++17 -O2 -Wall -Wextra -Werror -Wno-unused-parameter -Wno-missing-field-initializers \
   -fno-exceptions -fno-rtti -ffreestanding -fno-use-cxa-atexit -fno-strict-aliasing \
   -mcmodel=medany -fpic -D_KERNEL_MODE $flag "$@" \
   -c "$project/src/add-ons/kernel/busses/pci/ecam/$source.cpp" -o "$output/$mode/$source.o"
 done
 "$cxx" -shared -nostdlib -Wl,--no-undefined,-soname=ecam \
  "$("$cxx" -print-file-name=crtbeginS.o)" "$release/system/glue/haiku_version_glue.o" \
  "$output/$mode/ECAMPCIController.o" "$output/$mode/ECAMPCIControllerFDT.o" \
  "$output/$mode/ECAMPCIControllerACPI.o" "$output/$mode/kernel_interface.o" \
  "$release/system/kernel/kernel.so" "$("$cxx" -print-file-name=libgcc.a)" \
  "$("$cxx" -print-file-name=crtendS.o)" -o "$output/$mode/ecam"
done
if "$nm" -C "$output/control/ecam" | grep -q 'gPioneerRecovery'; then
 echo 'unexpected recovery module in control' >&2; exit 1
fi
"$nm" -C "$output/witness/ecam" | grep -q 'gPioneerRecovery'
"$objdump" -Cd "$output/witness/ECAMPCIController.o" > "$output/witness/disassembly.txt"
sed -n '/<ECAMPCIController::InitDriver(/,/^$/p' "$output/witness/disassembly.txt" > "$output/witness/init.txt"
grep -q 'lui.*a0,0x80000' "$output/witness/init.txt"
grep -q 'add.*a0,a0,15' "$output/witness/init.txt"
"$objdump" -Cd "$output/witness/kernel_interface.o" > "$output/witness/interface.txt"
sed -n '/<gPioneerRecovery.*_FUN/,/^$/p' "$output/witness/interface.txt" > "$output/witness/callbacks.txt"
[ "$(grep -c 'lui.*a0,0x80000' "$output/witness/callbacks.txt")" = 1 ]
[ "$(grep -c 'add.*a0,a0,15' "$output/witness/callbacks.txt")" = 1 ]
shasum -a 256 "$output/control/ecam" "$output/witness/ecam"
echo 'PASS SG2042 recovery witness rejects controller initialization and the service call; NOT LOADABLE'
