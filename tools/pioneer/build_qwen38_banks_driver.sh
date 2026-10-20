#!/bin/sh
# Offline driver45 candidate and default43 control. NEVER syncs,installs,
# reloads or touches a card. The control is NOT the qualified native binary.
# Uses ONLY driver sources/local headers plus existing packaged SDK/kernel.
set -eu
case "${PIONEER_Q38_NORMAL_CLOCK:-0}" in
 0|1) ;;
 *) exit 2;;
esac
case "${PIONEER_Q38_GDN_MUTABLE_STATE:-0}" in
 0) ;;
 *) printf '%s\n' 'Mutable GDN ownership/zero image has NO qualified native controller; refusing driver build.' >&2; exit 2;;
esac
case "${PIONEER_Q38_OUTPUT_TRANSFER_ROOT:-0}" in
 0) ;;
 *) printf '%s\n' 'Output-root prototype has NO qualified kernel/host-owner integration; refusing driver build.' >&2; exit 2;;
esac
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
 5) extra='-DTT_PIONEER_EXPERIMENTAL_Q38_PANEL -DTT_PIONEER_EXPERIMENTAL_Q38_AFFINE -DTT_PIONEER_EXPERIMENTAL_Q38_BLOCK -DTT_PIONEER_EXPERIMENTAL_Q38_STREAM -DTT_PIONEER_EXPERIMENTAL_Q38_ROWS'; variant=50;;
 6) extra='-DTT_PIONEER_EXPERIMENTAL_Q38_PANEL -DTT_PIONEER_EXPERIMENTAL_Q38_AFFINE -DTT_PIONEER_EXPERIMENTAL_Q38_BLOCK -DTT_PIONEER_EXPERIMENTAL_Q38_STREAM -DTT_PIONEER_EXPERIMENTAL_Q38_ROWS -DTT_PIONEER_EXPERIMENTAL_Q38_COHORT'; variant=51;;
 7) extra='-DTT_PIONEER_EXPERIMENTAL_Q38_PANEL -DTT_PIONEER_EXPERIMENTAL_Q38_AFFINE -DTT_PIONEER_EXPERIMENTAL_Q38_BLOCK -DTT_PIONEER_EXPERIMENTAL_Q38_STREAM -DTT_PIONEER_EXPERIMENTAL_Q38_ROWS -DTT_PIONEER_EXPERIMENTAL_Q38_COHORT -DTT_PIONEER_EXPERIMENTAL_Q38_COHORT_SEALED'; variant=52;;
 8) extra='-DTT_PIONEER_EXPERIMENTAL_Q38_PANEL -DTT_PIONEER_EXPERIMENTAL_Q38_AFFINE -DTT_PIONEER_EXPERIMENTAL_Q38_BLOCK -DTT_PIONEER_EXPERIMENTAL_Q38_STREAM -DTT_PIONEER_EXPERIMENTAL_Q38_ROWS -DTT_PIONEER_EXPERIMENTAL_Q38_COHORT -DTT_PIONEER_EXPERIMENTAL_Q38_COHORT_SEALED -DTT_PIONEER_EXPERIMENTAL_Q38_COHORT_PAYLOAD'; variant=53;;
 9) extra='-DTT_PIONEER_EXPERIMENTAL_Q38_PANEL -DTT_PIONEER_EXPERIMENTAL_Q38_AFFINE -DTT_PIONEER_EXPERIMENTAL_Q38_BLOCK -DTT_PIONEER_EXPERIMENTAL_Q38_STREAM -DTT_PIONEER_EXPERIMENTAL_Q38_ROWS -DTT_PIONEER_EXPERIMENTAL_Q38_COHORT -DTT_PIONEER_EXPERIMENTAL_Q38_COHORT_SEALED -DTT_PIONEER_EXPERIMENTAL_Q38_COHORT_PAYLOAD -DTT_PIONEER_EXPERIMENTAL_Q38_DISPATCH'; variant=54;;
 10|11|12|13|14) extra='-DTT_PIONEER_EXPERIMENTAL_Q38_PANEL -DTT_PIONEER_EXPERIMENTAL_Q38_AFFINE -DTT_PIONEER_EXPERIMENTAL_Q38_BLOCK -DTT_PIONEER_EXPERIMENTAL_Q38_STREAM -DTT_PIONEER_EXPERIMENTAL_Q38_ROWS -DTT_PIONEER_EXPERIMENTAL_Q38_COHORT -DTT_PIONEER_EXPERIMENTAL_Q38_COHORT_SEALED -DTT_PIONEER_EXPERIMENTAL_Q38_COHORT_PAYLOAD -DTT_PIONEER_EXPERIMENTAL_Q38_DISPATCH -DTT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_VECTOR'; variant=55
  if [ "$PIONEER_Q38_PANEL_DRIVER" = 11 ]; then extra="$extra -DTT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_BFLOAT"; variant=56; fi
  if [ "$PIONEER_Q38_PANEL_DRIVER" -ge 12 ]; then extra="$extra -DTT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_FAULT_RETURN"; variant=57; fi
  if [ "$PIONEER_Q38_PANEL_DRIVER" -ge 13 ]; then extra="$extra -DTT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_TRANSFER_DIAGNOSTIC"; variant=58; fi
  if [ "$PIONEER_Q38_PANEL_DRIVER" = 14 ]; then extra="$extra -DTT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_PEER_PROGRESS"; variant=59; fi;;
 *) exit 2;;
esac
if [ "${PIONEER_DRIVER_RELOAD:-0}" = 1 ]; then
 case "$variant" in 54|55|56|57|58|59) ;; *) exit 2;; esac
 case "${PIONEER_DRIVER_BUILD_TAG:-}" in ''|*[!0-9a-f]* ) exit 2;; esac
 [ "${#PIONEER_DRIVER_BUILD_TAG}" = 16 ] && [ "$PIONEER_DRIVER_BUILD_TAG" != 0000000000000000 ] || exit 2
 extra="$extra -DTT_PIONEER_DRIVER_RELOAD -DPIONEER_DRIVER_BUILD_TAG=0x${PIONEER_DRIVER_BUILD_TAG}ULL"
fi
case "${PIONEER_Q38_DISPATCH_PROFILE:-0}" in
 0) ;;
 1) [ "$variant" = 55 ] && [ "${PIONEER_DRIVER_RELOAD:-0}" = 1 ] || exit 2
    extra="$extra -DTT_PIONEER_Q38_DISPATCH_PROFILE";;
 *) exit 2;;
esac
case "${PIONEER_Q38_DISPATCH_BATCHED_PIO:-0}" in
 0) ;;
 1) [ "$variant" = 55 ] && [ "${PIONEER_DRIVER_RELOAD:-0}" = 1 ] || exit 2
    extra="$extra -DTT_PIONEER_Q38_DISPATCH_BATCHED_PIO";;
 *) exit 2;;
esac
case "${PIONEER_Q38_COMPACT_ACTIVATIONS:-0}" in
 0) ;;
 1) [ "$variant" = 55 ] && [ "${PIONEER_DRIVER_RELOAD:-0}" = 1 ] || exit 2
    extra="$extra -DPIONEER_Q38_COMPACT_ACTIVATIONS";;
 *) exit 2;;
esac
case "${PIONEER_Q38_REUSE_ACTIVATIONS:-0}" in
 0) ;;
 1) [ "${PIONEER_Q38_COMPACT_ACTIVATIONS:-0}" = 1 ] || exit 2
    extra="$extra -DPIONEER_Q38_REUSE_ACTIVATIONS";;
 *) exit 2;;
esac
case "${PIONEER_Q38_REUSE_CROSS_QUANT:-0}" in
 0) ;;
 1) [ "${PIONEER_Q38_REUSE_ACTIVATIONS:-0}" = 1 ] || exit 2
    extra="$extra -DPIONEER_Q38_REUSE_CROSS_QUANT";;
 *) exit 2;;
esac
case "${PIONEER_Q38_WORK_MODE:-0}" in
 0) ;;
 1) [ "${PIONEER_Q38_REUSE_ACTIVATIONS:-0}" = 1 ] || exit 2
    extra="$extra -DTT_PIONEER_Q38_WORK_MODE";;
 *) exit 2;;
esac
case "${PIONEER_Q38_WORK_POSTED_STAGE:-0}" in
 0) ;;
 1) [ "${PIONEER_Q38_WORK_MODE:-0}" = 1 ] || exit 2
    extra="$extra -DPIONEER_Q38_WORK_POSTED_STAGE";;
 *) exit 2;;
esac
case "${PIONEER_Q38_WORK_COHORT_CACHE:-0}" in
 0) ;;
 1) [ "${PIONEER_Q38_WORK_POSTED_STAGE:-0}" = 1 ] || exit 2
    extra="$extra -DPIONEER_Q38_WORK_COHORT_CACHE";;
 *) exit 2;;
esac
case "${PIONEER_Q38_Q4_TWO_PHASE:-0}" in
 0) ;;
 1) [ "${PIONEER_Q38_WORK_COHORT_CACHE:-0}" = 1 ] || exit 2
    extra="$extra -DPIONEER_Q38_Q4_TWO_PHASE";;
 *) exit 2;;
esac
case "${PIONEER_Q38_SPAN_PRELOAD:-0}" in
 0) ;;
 1) [ "${PIONEER_Q38_WORK_MODE:-0}" = 1 ] && [ "${PIONEER_Q38_Q4_TWO_PHASE:-0}" = 1 ] || exit 2
    extra="$extra -DPIONEER_Q38_SPAN_PRELOAD";;
 *) exit 2;;
esac
case "${PIONEER_Q38_SPAN_FAST:-0}" in
 0) ;;
 1) [ "${PIONEER_Q38_SPAN_PRELOAD:-0}" = 1 ] || exit 2
    extra="$extra -DPIONEER_Q38_SPAN_FAST";;
 *) exit 2;;
esac
case "${PIONEER_Q38_SPAN_REUSE_CODE:-0}" in
 0) ;;
 1) [ "${PIONEER_Q38_SPAN_FAST:-0}" = 1 ] || exit 2
    extra="$extra -DPIONEER_Q38_SPAN_REUSE_CODE";;
 *) exit 2;;
esac
case "${PIONEER_Q38_INT_SFPU:-0}" in
 0) ;;
 1) [ "$variant" = 55 ] && [ "${PIONEER_Q38_Q4_TWO_PHASE:-0}" = 1 ] || exit 2
    extra="$extra -DPIONEER_Q38_INT_SFPU";;
 *) exit 2;;
esac
case "${PIONEER_Q38_PIPELINED_MATRIX:-0}" in
 0) ;;
 1) [ "${PIONEER_Q38_INT_SFPU:-0}" = 1 ] || exit 2
    extra="$extra -DPIONEER_Q38_PIPELINED_MATRIX";;
 *) exit 2;;
esac
case "${PIONEER_Q38_STATIC_VECTOR:-0}" in
 0) ;;
 1) [ "${PIONEER_Q38_PIPELINED_MATRIX:-0}" = 1 ] || exit 2
    extra="$extra -DPIONEER_Q38_STATIC_VECTOR";;
 *) exit 2;;
esac
case "${PIONEER_Q38_CACHED_AFFINE:-0}" in
 0) ;;
 1) [ "${PIONEER_Q38_STATIC_VECTOR:-0}" = 1 ] || exit 2
    extra="$extra -DPIONEER_Q38_CACHED_AFFINE";;
 *) exit 2;;
esac
case "${PIONEER_Q38_REPLAY_OPERAND:-0}" in
 0) ;;
 1) [ "${PIONEER_Q38_CACHED_AFFINE:-0}" = 1 ] || exit 2
    extra="$extra -DPIONEER_Q38_REPLAY_OPERAND";;
 *) exit 2;;
esac
case "${PIONEER_Q38_WORK_SKIP_DOT_CHECK:-0}" in
 0) ;;
 1) [ "${PIONEER_Q38_REPLAY_OPERAND:-0}:${PIONEER_Q38_WORK_COHORT_CACHE:-0}:${PIONEER_Q38_WORK_MODE:-0}" = 1:1:1 ] || exit 2
    extra="$extra -DPIONEER_Q38_WORK_SKIP_DOT_CHECK";;
 *) exit 2;;
esac
case "${PIONEER_Q38_WORK_OUTPUT_DMA:-0}" in
 0) ;;
 1) [ "${PIONEER_Q38_WORK_SKIP_DOT_CHECK:-0}:${PIONEER_Q38_SPAN_FAST:-0}" = 1:1 ] || exit 2
    extra="$extra -DPIONEER_Q38_WORK_OUTPUT_DMA";;
 *) exit 2;;
esac
case "${PIONEER_Q38_QUEUED_MATRIX_ENTRY:-0}" in
 0) ;;
 1) [ "${PIONEER_Q38_WORK_OUTPUT_DMA:-0}" = 1 ] || exit 2
    extra="$extra -DPIONEER_Q38_QUEUED_MATRIX_ENTRY";;
 *) exit 2;;
esac
case "${PIONEER_Q38_WORK_INPUT_SUM_CACHE:-0}" in
 0) ;;
 1) [ "${PIONEER_Q38_QUEUED_MATRIX_ENTRY:-0}" = 1 ] || exit 2
    extra="$extra -DPIONEER_Q38_WORK_INPUT_SUM_CACHE";;
 *) exit 2;;
esac
case "${PIONEER_Q38_WORK_ZERO_MINIMUM:-0}" in
 0) ;;
 1) [ "${PIONEER_Q38_WORK_INPUT_SUM_CACHE:-0}" = 1 ] || exit 2
    extra="$extra -DPIONEER_Q38_WORK_ZERO_MINIMUM";;
 *) exit 2;;
esac
case "${PIONEER_Q38_NATIVE_OPERAND_STORE:-0}" in
 0) ;;
 1) [ "${PIONEER_Q38_WORK_ZERO_MINIMUM:-0}" = 1 ] || exit 2
    extra="$extra -DPIONEER_Q38_NATIVE_OPERAND_STORE";;
 *) exit 2;;
esac
[ "${PIONEER_Q38_WORK_DEFER_AFFINE:-0}" = 0 ] || {
 echo 'STOP deferred affine is OFFLINE ONLY;no qualified driver image selects it' >&2; exit 2;
}
case "${PIONEER_Q38_WORK_COMPACT_OUTPUT:-0}" in
 0) ;;
 1) [ "${PIONEER_Q38_WORK_SPARSE_EXPORT:-0}:${PIONEER_Q38_NATIVE_OPERAND_STORE:-0}:${PIONEER_Q38_WORK_OUTPUT_DMA:-0}" = 1:1:1 ] || exit 2
    extra="$extra -DPIONEER_Q38_WORK_COMPACT_OUTPUT";;
 *) exit 2;;
esac
case "${PIONEER_Q38_WORK_SPARSE_EXPORT:-0}" in
 0) ;;
 1) [ "${PIONEER_Q38_WORK_OUTPUT_DMA:-0}" = 1 ] || exit 2
    extra="$extra -DPIONEER_Q38_WORK_SPARSE_EXPORT";;
 *) exit 2;;
esac
case "${PIONEER_Q38_NORMAL_CLOCK:-0}" in
 0) ;;
 1) [ "$variant:${PIONEER_DRIVER_RELOAD:-0}:${PIONEER_Q38_WORK_SPARSE_EXPORT:-0}:${PIONEER_Q38_SPAN_PRELOAD:-0}" = 55:1:1:1 ] || exit 2
    extra="$extra -DPIONEER_Q38_NORMAL_CLOCK";;
 *) exit 2;;
esac
case "${PIONEER_Q38_LOCAL_DELTA_PROBE:-0}" in
 0) ;;
 1) [ "$variant:${PIONEER_DRIVER_RELOAD:-0}:${PIONEER_Q38_WORK_MODE:-0}" = 55:1:1 ] || exit 2
    extra="$extra -DPIONEER_Q38_LOCAL_DELTA_PROBE";;
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
 "$cxx" -std=gnu++17 -O2 -Wall -Wextra -Werror -fno-exceptions -fno-rtti -fstack-usage \
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
if [ "${PIONEER_DRIVER_RELOAD:-0}" = 1 ]; then
 "$cxx" -std=gnu++17 -O2 -Wall -Wextra -Werror -fno-exceptions -fno-rtti \
  -ffreestanding -fno-use-cxa-atexit -mcmodel=medany -fpic -D_KERNEL_MODE "$@" \
  -c "$project/src/add-ons/kernel/drivers/misc/pioneer_p100a_reload_guard.cpp" -o "$output/reload_guard.o"
 "$cxx" -shared -nostdlib -Wl,--no-undefined,-soname=pioneer_p100a_reload_guard \
  "$begin" "$release/system/glue/haiku_version_glue.o" "$output/reload_guard.o" "$kernel" "$libgcc" "$end" \
  -o "$output/pioneer_p100a_reload_guard"
 shasum -a 256 "$output/pioneer_p100a_reload_guard"
fi
# Compile/link the default (feature disabled) branch too. This catches any
# accidental new ABI dependency in the preserved ordinary driver build.
"$cxx" -std=gnu++17 -O2 -Wall -Wextra -Werror -fno-exceptions -fno-rtti -fstack-usage \
 -ffreestanding -finline -fno-semantic-interposition -fno-tree-vectorize -fno-use-cxa-atexit \
 -fno-strict-aliasing -fno-delete-null-pointer-checks -mcmodel=medany -fpic \
 -D_KERNEL_MODE "$@" -c "$project/src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp" \
 -o "$output/tenstorrent_pci_probe-control.o"
"$cxx" -shared -nostdlib -Wl,--no-undefined,-soname=tenstorrent_pci_probe \
 "$begin" "$release/system/glue/haiku_version_glue.o" \
 "$output/tenstorrent_pci_probe-control.o" "$output/PioneerPciProfile.o" "$kernel" "$libgcc" "$end" \
 -o "$output/tenstorrent_pci_probe-default43-control"
shasum -a 256 "$output/tenstorrent_pci_probe-default43-control"
