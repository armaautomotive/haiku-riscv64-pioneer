#!/bin/sh
# Standalone DATA transfer helper only;no llama source,driver or GPU access.
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
set -- -I"$sdk/headers" -idirafter "$sdk/headers/posix"
for include in "$sdk/headers/os" "$sdk"/headers/os/*; do
 [ ! -d "$include" ] || set -- "$@" -isystem "$include"
done
for library in "$build"/build_packages/gcc_syslibs-*/lib; do set -- "$@" -L"$library"; done
umask 077
"$cxx" -std=c++11 -O2 -Wall -Wextra -Werror -fno-exceptions -fno-rtti "$@" \
 -B"$release/system/glue/" -L"$release/system/libroot" \
 "$project/tools/pioneer/pioneer_model_assemble.cpp" -o "$output"
shasum -a 256 "$output"
