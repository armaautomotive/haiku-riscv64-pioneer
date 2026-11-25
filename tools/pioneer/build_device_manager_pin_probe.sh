#!/bin/sh
# Cross-build only: this does not install, boot, access a card or load a driver.
set -eu
[ "$#" = 2 ] || exit 2
build=$(CDPATH= cd -- "$1" && pwd)
output=$2
case "$output" in /*) ;; *) exit 2;; esac
[ ! -e "$output" ] && [ ! -L "$output" ] || exit 2
project=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
sdk=$build/objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop
release=$build/objects/haiku/riscv64/release
cxx=$build/cross-tools-riscv64/bin/riscv64-unknown-haiku-g++
[ -x "$cxx" ] && [ -f "$release/system/kernel/kernel.so" ] && [ -f "$release/system/libroot/libroot.so" ] || exit 2
umask 077
mkdir -m 700 "$output"
set -- -I"$sdk/headers" -idirafter "$sdk/headers/posix"
for include in "$sdk/headers/os" "$sdk"/headers/os/*; do
 [ ! -d "$include" ] || set -- "$@" -isystem "$include"
done
"$cxx" -std=gnu++17 -O2 -Wall -Wextra -Werror -fno-exceptions -ffreestanding \
 -fno-use-cxa-atexit -mcmodel=medany -fpic -D_KERNEL_MODE "$@" \
 -c "$project/src/add-ons/kernel/drivers/misc/pioneer_device_manager_pin_probe.cpp" -o "$output/probe.o"
"$cxx" -shared -nostdlib -Wl,--no-undefined,-soname=pioneer_device_manager_pin_probe \
 -o "$output/pioneer_device_manager_pin_probe" "$output/probe.o" "$release/system/kernel/kernel.so"
for lib in "$build"/build_packages/gcc_syslibs-*/lib; do
 [ ! -d "$lib" ] || set -- "$@" -L"$lib"
done
"$cxx" -std=gnu++17 -O2 -Wall -Wextra -Werror "$@" -B"$release/system/glue/" \
 -L"$release/system/libroot" "$project/tools/pioneer/pioneer_device_manager_pin_probe.cpp" \
 -o "$output/pin_probe_client"
shasum -a 256 "$output/pioneer_device_manager_pin_probe" "$output/pin_probe_client"
echo 'PASS cross-build; not installed, booted, loaded or run'
