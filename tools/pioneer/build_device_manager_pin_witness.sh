#!/bin/sh
# Compile the actual kernel device-manager and module units with and without
# the experimental pin service. This does not link, install or boot a kernel.
set -eu
[ "$#" = 2 ] || exit 2
build=$(CDPATH= cd -- "$1" && pwd)
output=$2
[ ! -e "$output" ] && [ ! -L "$output" ] || exit 2
mkdir -m 700 "$output"
output=$(CDPATH= cd -- "$output" && pwd)
project=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
sdk=$build/objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop
release=$build/objects/haiku/riscv64/release
cxx=$build/cross-tools-riscv64/bin/riscv64-unknown-haiku-g++
nm=$build/cross-tools-riscv64/bin/riscv64-unknown-haiku-nm
ld=$build/cross-tools-riscv64/bin/riscv64-unknown-haiku-ld
[ -x "$cxx" ] && [ -f "$release/system/kernel/kernel.so" ] || exit 2
set -- -isystem "$sdk/headers/os/drivers" \
 -isystem "$project/headers/private/kernel" -isystem "$project/headers/private/kernel/arch/riscv64" \
 -isystem "$project/headers/private/kernel/arch/generic" -isystem "$project/headers/private/kernel/boot/platform/efi" \
 -isystem "$project/headers/private" -isystem "$project/headers/private/drivers" \
 -isystem "$project/headers/private/system" -isystem "$project/headers/private/system/arch/riscv64" \
 -isystem "$project/build/config_headers" -isystem "$project/headers/private/shared" \
 -I"$project/src/system/kernel" -I"$project/src/system/kernel/device_manager" -I"$release/system/kernel" \
 -I"$sdk/headers" -idirafter "$sdk/headers/posix"
for include in "$sdk/headers/os" "$sdk"/headers/os/*; do
 [ ! -d "$include" ] || set -- "$@" -isystem "$include"
done
for mode in control witness; do
 flag=
 [ "$mode" != witness ] || flag=-DPIONEER_DEVICE_MANAGER_PIN_WITNESS
 for unit in device_manager/device_manager module; do
  "$cxx" -std=gnu++17 -O2 -Wall -Wextra -Werror -Wno-unused-parameter -Wno-missing-field-initializers \
   -fno-exceptions -ffreestanding -fno-use-cxa-atexit -fno-strict-aliasing \
   -mcmodel=medany -fpic -D_KERNEL_MODE $flag "$@" \
   -c "$project/src/system/kernel/$unit.cpp" -o "$output/$mode-$(basename "$unit").o"
 done
done
if "$nm" "$output/control-device_manager.o" "$output/control-module.o" | grep -q 'gPioneerDeviceManagerPinModule'; then
 echo 'unexpected pin service in default kernel objects' >&2; exit 1
fi
"$nm" "$output/witness-device_manager.o" | grep -q ' [BD] gPioneerDeviceManagerPinModule$'
"$nm" "$output/witness-module.o" | grep -q ' U gPioneerDeviceManagerPinModule$'
"$ld" -r "$output/witness-device_manager.o" "$output/witness-module.o" -o "$output/witness-combined.o"
"$nm" "$output/witness-combined.o" | grep -q ' [BD] gPioneerDeviceManagerPinModule$'
shasum -a 256 "$output"/*.o
echo 'PASS actual device-manager pin kernel objects; NO KERNEL LINK OR HARDWARE'
