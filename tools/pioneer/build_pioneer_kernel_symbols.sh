#!/bin/sh
# Builds a read-only Haiku kernel-symbol inventory client; no target access.
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
[ -x "$cxx" ] && [ -f "$release/system/libroot/libroot.so" ] || exit 2
set -- -I"$sdk/headers" -idirafter "$sdk/headers/posix"
for include in "$sdk/headers/os" "$sdk"/headers/os/*; do
 [ ! -d "$include" ] || set -- "$@" -isystem "$include"
done
for lib in "$build"/build_packages/gcc_syslibs-*/lib; do
 [ ! -d "$lib" ] || set -- "$@" -L"$lib"
done
"$cxx" -std=c++17 -O2 -Wall -Wextra -Werror "$@" \
 -B"$release/system/glue/" -L"$release/system/libroot" \
 "$project/tools/pioneer/pioneer_kernel_symbols.cpp" -o "$output"
shasum -a 256 "$output"
echo 'PASS read-only Haiku kernel-symbol client cross-build; no target access'
