#!/bin/sh
# Offline driver45 candidate and default43 control. NEVER syncs,installs,
# reloads or touches a card. The control is NOT the qualified native binary.
# Uses ONLY driver sources/local headers plus existing packaged SDK/kernel.
set -eu
[ "$#" = 2 ] || exit 2
project=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
build=$(CDPATH= cd -- "$1" && pwd)
output=$2
case "$output" in /*) ;; *) exit 2;; esac
[ ! -e "$output" ] && [ ! -L "$output" ] || exit 1
sdk=$build/objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop
release=$build/objects/haiku/riscv64/release
cxx=$build/cross-tools-riscv64/bin/riscv64-unknown-haiku-g++
kernel=$release/system/kernel/kernel.so
test -f "$kernel"
case "${PIONEER_Q38_PANEL_DRIVER:-0}" in
 0) extra=; variant=45;;
 1) extra=-DTT_PIONEER_EXPERIMENTAL_Q38_PANEL; variant=46;;
 2) extra='-DTT_PIONEER_EXPERIMENTAL_Q38_PANEL -DTT_PIONEER_EXPERIMENTAL_Q38_AFFINE'; variant=47;;
 3) extra='-DTT_PIONEER_EXPERIMENTAL_Q38_PANEL -DTT_PIONEER_EXPERIMENTAL_Q38_AFFINE -DTT_PIONEER_EXPERIMENTAL_Q38_BLOCK'; variant=48;;
 4) extra='-DTT_PIONEER_EXPERIMENTAL_Q38_PANEL -DTT_PIONEER_EXPERIMENTAL_Q38_AFFINE -DTT_PIONEER_EXPERIMENTAL_Q38_BLOCK -DTT_PIONEER_EXPERIMENTAL_Q38_STREAM'; variant=49;;
 *) exit 2;;
esac
set -- -isystem "$project/headers/private/kernel" -isystem "$project/headers/private/kernel/arch/riscv64" \
 -isystem "$project/headers/private/kernel/arch/generic" -isystem "$project/headers/private" \
 -isystem "$project/headers/private/drivers" -isystem "$project/headers/private/system" \
 -isystem "$project/headers/private/system/arch/riscv64" \
 -isystem "$project/build/config_headers" -isystem "$project/headers/private/shared" \
 -I"$build/objects/haiku/riscv64/release/system/kernel" \
 -I"$build/objects/haiku/riscv64/debug/system/kernel" \
 -I"$sdk/headers" -idirafter "$sdk/headers/posix"
for include in "$sdk/headers/os" "$sdk"/headers/os/*; do
 [ ! -d "$include" ] || set -- "$@" -isystem "$include"
done
umask 077
mkdir -m 700 "$output"
for source in PioneerPciProfile tenstorrent_pci_probe; do
 "$cxx" -std=gnu++17 -O2 -Wall -Wextra -Werror -fno-exceptions -fno-rtti \
  -ffreestanding -finline -fno-semantic-interposition -fno-tree-vectorize -fno-use-cxa-atexit \
  -fno-strict-aliasing -fno-delete-null-pointer-checks -mcmodel=medany -fpic \
  -D_KERNEL_MODE -DTT_PIONEER_EXPERIMENTAL_Q38_BANKS $extra "$@" \
  -c "$project/src/add-ons/kernel/drivers/misc/$source.cpp" -o "$output/$source.o"
done
begin=$("$cxx" -print-file-name=crtbeginS.o)
end=$("$cxx" -print-file-name=crtendS.o)
libgcc=$("$cxx" -print-file-name=libgcc.a)
"$cxx" -shared -nostdlib -Wl,--no-undefined,-soname=tenstorrent_pci_probe \
 "$begin" "$release/system/glue/haiku_version_glue.o" \
 "$output/tenstorrent_pci_probe.o" "$output/PioneerPciProfile.o" "$kernel" "$libgcc" "$end" \
 -o "$output/tenstorrent_pci_probe-driver$variant"
shasum -a 256 "$output/tenstorrent_pci_probe-driver$variant"
# Compile/link the default (feature disabled) branch too. This catches any
# accidental new ABI dependency in the preserved ordinary driver build.
"$cxx" -std=gnu++17 -O2 -Wall -Wextra -Werror -fno-exceptions -fno-rtti \
 -ffreestanding -finline -fno-semantic-interposition -fno-tree-vectorize -fno-use-cxa-atexit \
 -fno-strict-aliasing -fno-delete-null-pointer-checks -mcmodel=medany -fpic \
 -D_KERNEL_MODE "$@" -c "$project/src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp" \
 -o "$output/tenstorrent_pci_probe-control.o"
"$cxx" -shared -nostdlib -Wl,--no-undefined,-soname=tenstorrent_pci_probe \
 "$begin" "$release/system/glue/haiku_version_glue.o" \
 "$output/tenstorrent_pci_probe-control.o" "$output/PioneerPciProfile.o" "$kernel" "$libgcc" "$end" \
 -o "$output/tenstorrent_pci_probe-default43-control"
shasum -a 256 "$output/tenstorrent_pci_probe-default43-control"
