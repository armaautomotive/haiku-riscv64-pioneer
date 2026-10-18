#!/bin/sh
# Compile only,no install/device access.
set -eu
[ "$#" = 2 ] || exit 2
project=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
build=$(CDPATH= cd -- "$1" && pwd)
output=$2
case "$output" in /*) ;; *) exit 2;; esac
[ ! -e "$output" ] && [ ! -L "$output" ] || exit 1
case "${PIONEER_Q38_PANEL_CLIENT:-0}" in
 0) source=pioneer_q38_banks;;
 1) source=pioneer_q38_panel_probe;;
 2) source=pioneer_q38_stream_probe;;
 3) source=pioneer_q38_cohort_probe;;
 4) source=pioneer_q38_dispatch_probe;;
 *) exit 2;;
esac
case "${PIONEER_Q38_AFFINE_CLIENT:-0}" in
 0) extra=;;
 1) [ "$source" = pioneer_q38_panel_probe ] || exit 2; extra='-DTT_PIONEER_EXPERIMENTAL_Q38_AFFINE -ffp-contract=off';;
 *) exit 2;;
esac
case "${PIONEER_Q38_FIXTURE_CLIENT:-0}" in
 0) ;;
 1) [ "${PIONEER_Q38_AFFINE_CLIENT:-0}" = 1 ] || exit 2; extra="$extra -DPIONEER_Q38_PUBLIC_FIXTURES";;
 *) exit 2;;
esac
case "${PIONEER_Q38_BLOCK_CLIENT:-0}" in
 0) ;;
 1) [ "${PIONEER_Q38_AFFINE_CLIENT:-0}" = 1 ] || exit 2; extra="$extra -DTT_PIONEER_EXPERIMENTAL_Q38_BLOCK";;
 *) exit 2;;
esac
if [ "$source" = pioneer_q38_stream_probe ] || [ "$source" = pioneer_q38_cohort_probe ] || [ "$source" = pioneer_q38_dispatch_probe ]; then extra="$extra -ffp-contract=off"; fi
case "${PIONEER_Q38_COHORT_SEALED:-0}" in
 0) ;;
 1) [ "$source" = pioneer_q38_cohort_probe ] || exit 2; extra="$extra -DTT_PIONEER_EXPERIMENTAL_Q38_COHORT_SEALED";;
 *) exit 2;;
esac
case "${PIONEER_Q38_COHORT_PAYLOAD:-0}" in
 0) ;;
 1) [ "$source" = pioneer_q38_cohort_probe ] && [ "${PIONEER_Q38_COHORT_SEALED:-0}" = 1 ] || exit 2; extra="$extra -DTT_PIONEER_EXPERIMENTAL_Q38_COHORT_PAYLOAD";;
 *) exit 2;;
esac
case "${PIONEER_Q38_ROWS_CLIENT:-0}" in
 0) ;;
 1) [ "$source" = pioneer_q38_stream_probe ] || exit 2; extra="$extra -DTT_PIONEER_EXPERIMENTAL_Q38_ROWS";;
 *) exit 2;;
esac
case "${PIONEER_Q38_STREAM_DIAGNOSTICS:-0}" in
 0) ;;
 1) [ "$source" = pioneer_q38_stream_probe ] || exit 2; extra="$extra -DPIONEER_Q38_STREAM_DIAGNOSTICS";;
 *) exit 2;;
esac
case "${PIONEER_Q38_PRIMITIVE_TIMING:-0}" in
 0) ;;
 1) [ "${PIONEER_Q38_ROWS_CLIENT:-0}" = 1 ] || exit 2; extra="$extra -DPIONEER_Q38_PRIMITIVE_TIMING";;
 *) exit 2;;
esac
sdk=$build/objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop
release=$build/objects/haiku/riscv64/release
cxx=$build/cross-tools-riscv64/bin/riscv64-unknown-haiku-g++
set -- -I"$sdk/headers" -idirafter "$sdk/headers/posix"
for include in "$sdk/headers/os" "$sdk"/headers/os/*; do
 [ ! -d "$include" ] || set -- "$@" -isystem "$include"
done
for library in "$build"/build_packages/gcc_syslibs-*/lib; do set -- "$@" -L"$library"; done
umask 077
"$cxx" -std=c++17 -O2 -Wall -Wextra -Werror -fno-exceptions -fno-rtti $extra "$@" \
 -B"$release/system/glue/" -L"$release/system/libroot" \
 "$project/tools/pioneer/$source.cpp" -o "$output"
shasum -a 256 "$output"
