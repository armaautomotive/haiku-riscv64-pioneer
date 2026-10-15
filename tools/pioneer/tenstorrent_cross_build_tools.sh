#!/bin/sh
# Build userspace diagnostics only; never connect to or operate a card.
set -eu
fail() { printf 'ERROR: %s\n' "$1" >&2; exit 1; }
[ "$#" = 2 ] || fail 'Usage: tenstorrent_cross_build_tools.sh CONFIGURED_BUILD FRESH_ABSOLUTE_OUTPUT'
tt_project=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
tt_build=$(CDPATH= cd -- "$1" && pwd)
tt_output=$2
case "$tt_output" in /*) ;; *) fail 'Output must be absolute';; esac
[ ! -e "$tt_output" ] && [ ! -L "$tt_output" ] || fail 'Output already exists'
tt_sdk=$tt_build/objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop/headers
tt_cc=$tt_build/cross-tools-riscv64/bin/riscv64-unknown-haiku-gcc
tt_glue=$tt_build/objects/haiku/riscv64/release/system/glue
tt_libroot=$tt_build/objects/haiku/riscv64/release/system/libroot
[ -x "$tt_cc" ] && [ "$("$tt_cc" -dumpmachine)" = riscv64-unknown-haiku ] || fail 'Expected configured RISC-V Haiku compiler'
[ -d "$tt_sdk" ] && [ -f "$tt_libroot/libroot.so" ] || fail 'Build SDK headers and libroot first'
tt_begin=$("$tt_cc" -print-file-name=crtbeginS.o)
tt_end=$("$tt_cc" -print-file-name=crtendS.o)
for tt_input in "$tt_begin" "$tt_end" "$tt_glue/arch/riscv64/crti.o" \
 "$tt_glue/arch/riscv64/crtn.o" "$tt_glue/start_dyn.o" "$tt_glue/init_term_dyn.o"; do
 [ -f "$tt_input" ] || fail 'Missing configured-build startup object'
done
umask 077
mkdir -m 700 "$tt_output"
for tt_name in tenstorrent_haiku_diag tenstorrent_haiku_bar4_probe \
 tenstorrent_haiku_pioneer_profile tenstorrent_haiku_mmio_probe tenstorrent_haiku_arc_probe \
 tenstorrent_haiku_telemetry_probe tenstorrent_haiku_tensix_probe tenstorrent_haiku_l1_probe tenstorrent_haiku_brisc_probe tenstorrent_haiku_dram_probe tenstorrent_haiku_dram_samples tenstorrent_haiku_matmul_probe; do
 case "$tt_name" in
  tenstorrent_haiku_diag) tt_language=c; tt_standard=c11; tt_extension=c; set -- ;;
  *) tt_language=c++; tt_standard=c++11; tt_extension=cpp; set -- -fno-exceptions -fno-rtti ;;
 esac
 "$tt_cc" -x "$tt_language" -std="$tt_standard" -O2 -Wall -Wextra -Werror "$@" \
  -isystem "$tt_sdk/os" -isystem "$tt_sdk/os/kernel" -isystem "$tt_sdk/os/drivers" \
  -isystem "$tt_sdk/os/support" -isystem "$tt_sdk/config" -isystem "$tt_sdk/posix" -isystem "$tt_sdk" \
  -c "$tt_project/tools/pioneer/$tt_name.$tt_extension" -o "$tt_output/$tt_name.o"
 "$tt_cc" -nostartfiles -Wl,-e,_start \
  "$tt_glue/arch/riscv64/crti.o" "$tt_begin" "$tt_glue/start_dyn.o" "$tt_glue/init_term_dyn.o" \
  "$tt_output/$tt_name.o" -L "$tt_libroot" -lroot "$tt_end" "$tt_glue/arch/riscv64/crtn.o" \
  -o "$tt_output/$tt_name"
done
if command -v sha256sum >/dev/null 2>&1; then
 tt_hash() { sha256sum "$@"; }
else
 tt_hash() { shasum -a 256 "$@"; }
fi
(
 cd "$tt_output"
 tt_hash tenstorrent_haiku_diag tenstorrent_haiku_bar4_probe \
  tenstorrent_haiku_pioneer_profile tenstorrent_haiku_mmio_probe tenstorrent_haiku_arc_probe \
  tenstorrent_haiku_telemetry_probe tenstorrent_haiku_tensix_probe tenstorrent_haiku_l1_probe tenstorrent_haiku_brisc_probe tenstorrent_haiku_dram_probe tenstorrent_haiku_dram_samples tenstorrent_haiku_matmul_probe > artifacts.sha256
)
printf 'Built twelve diagnostics in %s; no device was accessed\n' "$tt_output"
