#!/bin/sh
# Fixed synthetic DEBUG probe only; cross-build, never install or run.
set -eu
[ "$#" = 2 ] || exit 2
project=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
build=$(CDPATH= cd -- "$1" && pwd)
output=$2
case "$output" in /*) ;; *) exit 2;; esac
[ ! -e "$output" ] && [ ! -L "$output" ] || exit 1
sdk=$build/objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop
release=$build/objects/haiku/riscv64/release
set -- -I"$sdk/headers" -idirafter "$sdk/headers/posix"
for include in "$sdk/headers/os" "$sdk"/headers/os/*; do [ ! -d "$include" ] || set -- "$@" -isystem "$include"; done
for library in "$build"/build_packages/gcc_syslibs-*/lib; do set -- "$@" -L"$library"; done
umask 077
"$build/cross-tools-riscv64/bin/riscv64-unknown-haiku-g++" -std=c++17 -O2 -ffp-contract=off -Wall -Wextra -Werror \
 -fno-exceptions -fno-rtti -DTT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_VECTOR "$@" \
 -B"$release/system/glue/" -L"$release/system/libroot" "$project/tools/pioneer/pioneer_q38_work_batch_probe.cpp" -o "$output"
shasum -a 256 "$output"
