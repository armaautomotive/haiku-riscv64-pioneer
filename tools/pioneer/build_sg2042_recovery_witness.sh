#!/bin/sh
# Offline cross-build only; the witness rejects controller initialization and
# its default recovery module refuses requests. A second non-loadable build
# type-checks the controller gateway. A distinct qualification candidate may
# initialize but exposes only a fixed no-op lease probe. Never installs or
# touches hardware.
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
for mode in control witness gateway qualification retained; do
 mkdir -m 700 "$output/$mode"
 flag=
 [ "$mode" = control ] || flag=-DPIONEER_SG2042_RECOVERY_WITNESS
 [ "$mode" != gateway ] || flag="$flag -DPIONEER_SG2042_RECOVERY_GATEWAY_WITNESS"
 [ "$mode" != qualification ] || flag="$flag -DPIONEER_SG2042_RECOVERY_QUALIFICATION"
 [ "$mode" != retained ] || flag="$flag -DPIONEER_SG2042_RETAINED_LEASE_SERVICE"
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
for invalid in gateway-with-qualification qualification-without-witness retained-without-witness retained-with-qualification retained-with-gateway retained-with-witness; do
 case "$invalid" in
  gateway-with-qualification) flags='-DPIONEER_SG2042_RECOVERY_WITNESS -DPIONEER_SG2042_RECOVERY_GATEWAY_WITNESS -DPIONEER_SG2042_RECOVERY_QUALIFICATION';;
  qualification-without-witness) flags='-DPIONEER_SG2042_RECOVERY_QUALIFICATION';;
  retained-without-witness) flags='-DPIONEER_SG2042_RETAINED_LEASE_SERVICE';;
  retained-with-qualification) flags='-DPIONEER_SG2042_RECOVERY_WITNESS -DPIONEER_SG2042_RETAINED_LEASE_SERVICE -DPIONEER_SG2042_RECOVERY_QUALIFICATION';;
  retained-with-gateway) flags='-DPIONEER_SG2042_RECOVERY_WITNESS -DPIONEER_SG2042_RETAINED_LEASE_SERVICE -DPIONEER_SG2042_RECOVERY_GATEWAY_WITNESS';;
  retained-with-witness) flags='-DPIONEER_SG2042_RECOVERY_WITNESS -DPIONEER_SG2042_RETAINED_LEASE_SERVICE -DPIONEER_SG2042_RETAINED_LEASE_WITNESS';;
 esac
 # Intentional word splitting: flags is one of the two fixed literals above.
 if "$cxx" -std=gnu++17 -fno-exceptions -ffreestanding -mcmodel=medany -fpic -D_KERNEL_MODE \
   $flags "$@" -fsyntax-only "$project/src/add-ons/kernel/busses/pci/ecam/kernel_interface.cpp" > "$output/$invalid.log" 2>&1; then
  echo "unsafe qualification flags accepted: $invalid" >&2; exit 1
 fi
 case "$invalid" in
  gateway-with-qualification|qualification-without-witness) grep -q 'Qualification requires the witness and excludes the general recovery gateway' "$output/$invalid.log";;
  *) grep -q 'Retained lease requires the non-loadable recovery configuration' "$output/$invalid.log";;
 esac
done
if "$nm" -C "$output/control/ecam" | grep -q 'gPioneerRecovery'; then
 echo 'unexpected recovery module in control' >&2; exit 1
fi
"$nm" -C "$output/witness/ecam" | grep -q 'gPioneerRecovery'
"$nm" -C "$output/gateway/ecam" | grep -q 'gPioneerRecovery'
"$nm" -C "$output/qualification/ecam" | grep -q 'gPioneerLeaseProbe'
"$nm" -C "$output/qualification/ecam" | grep -q 'gPioneerLeaseContention'
"$nm" -C "$output/retained/ecam" | grep -q 'gPioneerRetainedLease'
if "$nm" -C "$output/control/ecam" "$output/witness/ecam" "$output/gateway/ecam" "$output/qualification/ecam" | grep -q 'gPioneerRetainedLease'; then
 echo 'unexpected retained lease module in another build' >&2; exit 1
fi
if "$nm" -C "$output/control/ecam" "$output/witness/ecam" "$output/gateway/ecam" | grep -Eq 'gPioneerLease(Probe|Contention)'; then
 echo 'unexpected qualification probe in another build' >&2; exit 1
fi
"$objdump" -Cd "$output/witness/ECAMPCIController.o" > "$output/witness/disassembly.txt"
sed -n '/<ECAMPCIController::InitDriver(/,/^$/p' "$output/witness/disassembly.txt" > "$output/witness/init.txt"
grep -q 'lui.*a0,0x80000' "$output/witness/init.txt"
grep -q 'add.*a0,a0,15' "$output/witness/init.txt"
"$objdump" -Cd "$output/witness/kernel_interface.o" > "$output/witness/interface.txt"
sed -n '/<gPioneerRecovery.*_FUN/,/^$/p' "$output/witness/interface.txt" > "$output/witness/callbacks.txt"
[ "$(grep -c 'lui.*a0,0x80000' "$output/witness/callbacks.txt")" = 1 ]
[ "$(grep -c 'add.*a0,a0,15' "$output/witness/callbacks.txt")" = 1 ]
"$objdump" -Cd "$output/gateway/ECAMPCIController.o" > "$output/gateway/disassembly.txt"
sed -n '/<ECAMPCIController::InitDriver(/,/^$/p' "$output/gateway/disassembly.txt" > "$output/gateway/init.txt"
grep -q 'lui.*a0,0x80000' "$output/gateway/init.txt"
grep -q 'add.*a0,a0,15' "$output/gateway/init.txt"
"$objdump" -Cd "$output/retained/ECAMPCIController.o" > "$output/retained/disassembly.txt"
sed -n '/<ECAMPCIController::InitDriver(/,/^$/p' "$output/retained/disassembly.txt" > "$output/retained/init.txt"
grep -q 'lui.*a0,0x80000' "$output/retained/init.txt"
grep -q 'add.*a0,a0,15' "$output/retained/init.txt"
"$nm" -C "$output/gateway/kernel_interface.o" | grep -q 'ECAMPCIController::RunPioneerRecovery'
"$objdump" -Cd "$output/qualification/kernel_interface.o" > "$output/qualification/interface.txt"
sed -n '/<gPioneerRecovery.*_FUN/,/^$/p' "$output/qualification/interface.txt" > "$output/qualification/refusal.txt"
[ "$(grep -c 'lui.*a0,0x80000' "$output/qualification/refusal.txt")" = 1 ]
[ "$(grep -c 'add.*a0,a0,15' "$output/qualification/refusal.txt")" = 1 ]
"$nm" -C "$output/qualification/kernel_interface.o" | grep -q 'ECAMPCIController::RunPioneerRecovery'
shasum -a 256 "$output/control/ecam" "$output/witness/ecam" "$output/gateway/ecam" "$output/qualification/ecam" "$output/retained/ecam"
echo 'PASS SG2042 non-loadable witnesses and offline fixed-probe qualification candidate'
