#!/bin/sh
# Standalone, default-off RTL concurrency comparison. NO sync/install/card work.
# Uses the existing SDK/kernel/compat archive; never traverses llama sources.
set -eu
[ "$#" = 2 ] || exit 2
project=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
build=$(CDPATH= cd -- "$1" && pwd)
output=$2
case "$output" in /*) ;; *) exit 2;; esac
[ ! -e "$output" ] && [ ! -L "$output" ] || exit 1
sdk=$build/objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop
release=$build/objects/haiku/riscv64/release
tools=$build/cross-tools-riscv64/bin/riscv64-unknown-haiku
driver=$project/src/add-ons/kernel/drivers/network/ether/rtl8125
archive=$release/libs/compat/freebsd_network/libfreebsd_network.a
test -f "$archive" && test -f "$release/system/kernel/kernel.so"
set -- -isystem "$project/src/libs/compat/openbsd_network/compat" \
 -isystem "$project/src/libs/compat/freebsd_network/compat" -I"$driver" \
 -isystem "$project/headers/private/net" -isystem "$project/headers/private/system" \
 -isystem "$project/headers/private/system/arch/riscv64" \
 -isystem "$project/headers/private/kernel" -isystem "$project/headers/private/kernel/arch/riscv64" \
 -isystem "$project/headers/private/kernel/arch/generic" -isystem "$project/headers/private" \
 -isystem "$project/headers/private/drivers" -isystem "$project/headers/private/shared" \
 -I"$project/build/config_headers" \
 -I"$build/objects/haiku/riscv64/release/system/kernel" \
 -I"$build/objects/haiku/riscv64/debug/system/kernel" \
 -I"$sdk/headers" -idirafter "$sdk/headers/posix"
for include in "$sdk/headers/os" "$sdk"/headers/os/*; do
 [ ! -d "$include" ] || set -- "$@" -isystem "$include"
done
umask 077
mkdir -m 700 "$output"
begin=$("$tools-g++" -print-file-name=crtbeginS.o)
end=$("$tools-g++" -print-file-name=crtendS.o)
libgcc=$("$tools-g++" -print-file-name=libgcc.a)
for profile in serialized msi-control; do
 extra=
 [ "$profile" != serialized ] || extra=-DHAIKU_PIONEER_RTL_SERIALIZE=1
 for source in if_rge glue; do
  case "$source" in if_rge) input=$driver/dev/pci/if_rge.c;; glue) input=$driver/glue.c;; esac
  "$tools-gcc" -std=gnu11 -O2 -Wall -Wextra -Wno-unused -Wno-sign-compare \
   -Werror=implicit-function-declaration -ffreestanding -fno-strict-aliasing \
   -fno-delete-null-pointer-checks -fpic -mcmodel=medany \
   -D_KERNEL=1 -D_KERNEL_MODE -D_XOPEN_SOURCE -DHAIKU_PIONEER_RTL_MSI_ONLY=1 \
   -DHAIKU_PIONEER_RTL_DIAGNOSTICS=1 $extra "$@" \
   -c "$input" -o "$output/$source-$profile.o"
 done
 "$tools-g++" -shared -nostdlib -Wl,--no-undefined,-soname=rtl8125 \
  "$begin" "$release/system/glue/haiku_version_glue.o" \
  "$output/if_rge-$profile.o" "$output/glue-$profile.o" "$archive" \
  "$release/system/kernel/kernel.so" "$libgcc" "$end" -o "$output/rtl8125-$profile"
 shasum -a 256 "$output/rtl8125-$profile"
done
echo OFFLINE_RTL_CANDIDATE_AND_CONTROL_BUILT_NOT_INSTALLED_OR_RUNTIME_VALIDATED
