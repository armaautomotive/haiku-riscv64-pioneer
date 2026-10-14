#!/bin/sh
# Build a bounded userspace CPU smoke test only. Never reboot or touch a card.
set -eu
[ "$#" = 2 ] || [ "$#" = 3 ] || { echo 'Usage: pioneer_cross_build_smp_smoke.sh CONFIGURED_BUILD FRESH_ABSOLUTE_OUTPUT [--vm-shootdown]' >&2; exit 2; }
case "${3:-}" in
 '') smp_name=pioneer_smp_smoke;;
 --vm-shootdown) smp_name=pioneer_vm_shootdown_smoke;;
 *) exit 2;;
esac
smp_source=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
smp_build=$(CDPATH= cd -- "$1" && pwd)
smp_output=$2
case "$smp_output" in /*) ;; *) exit 2;; esac
[ ! -e "$smp_output" ] && [ ! -L "$smp_output" ]
smp_sdk=$smp_build/objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop/headers
smp_cc=$smp_build/cross-tools-riscv64/bin/riscv64-unknown-haiku-gcc
smp_glue=$smp_build/objects/haiku/riscv64/release/system/glue
smp_libroot=$smp_build/objects/haiku/riscv64/release/system/libroot
[ "$("$smp_cc" -dumpmachine)" = riscv64-unknown-haiku ]
smp_begin=$("$smp_cc" -print-file-name=crtbeginS.o)
smp_end=$("$smp_cc" -print-file-name=crtendS.o)
umask 077
mkdir -m 700 "$smp_output"
"$smp_cc" -std=c11 -O2 -Wall -Wextra -Werror \
 -isystem "$smp_sdk/os" -isystem "$smp_sdk/os/kernel" -isystem "$smp_sdk/os/drivers" \
 -isystem "$smp_sdk/os/support" -isystem "$smp_sdk/os/storage" \
 -isystem "$smp_sdk/config" -isystem "$smp_sdk/posix" -isystem "$smp_sdk" \
 -c "$smp_source/$smp_name.c" -o "$smp_output/$smp_name.o"
"$smp_cc" -nostartfiles -Wl,-e,_start \
 "$smp_glue/arch/riscv64/crti.o" "$smp_begin" "$smp_glue/start_dyn.o" "$smp_glue/init_term_dyn.o" \
 "$smp_output/$smp_name.o" -L "$smp_libroot" -lroot "$smp_end" "$smp_glue/arch/riscv64/crtn.o" \
 -o "$smp_output/$smp_name"
shasum -a 256 "$smp_output/$smp_name"
