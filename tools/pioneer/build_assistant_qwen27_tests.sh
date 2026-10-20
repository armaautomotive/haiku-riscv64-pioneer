#!/bin/sh
# Native Haiku socket/fork/exec regression; fake worker has NO accelerator code.
set -eu
[ "$#" = 3 ] || exit 2
project=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
build=$1
peer=$2
output=$3
case "$peer:$output" in /*:/*) ;; *) exit 2;; esac
sdk=$build/objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop
release=$build/objects/haiku/riscv64/release
cxx=$build/cross-tools-riscv64/bin/riscv64-unknown-haiku-g++
set -- -I"$sdk/headers" -idirafter "$sdk/headers/posix"
for include in "$sdk/headers/os" "$sdk"/headers/os/*; do [ ! -d "$include" ] || set -- "$@" -isystem "$include"; done
for library in "$build"/build_packages/gcc_syslibs-*/lib; do set -- "$@" -L"$library"; done
"$cxx" -std=c++11 -O1 -Wall -Wextra -Werror "$@" -B"$release/system/glue/" \
 -L"$release/system/libroot" -L"$release/system/libnetwork" \
 "$project/tools/pioneer/tests/assistant_qwen27_fake_worker.cpp" -lnetwork -o "${output}FakeWorker"
"$cxx" -std=c++11 -O1 -Wall -Wextra -Werror "$@" -B"$release/system/glue/" \
 -L"$release/system/libroot" -L"$release/system/libnetwork" -DASSISTANT_QWEN27_WORKER="\"$peer\"" \
 "$project/tools/pioneer/tests/assistant_qwen27_test.cpp" "$project/src/apps/assistant/P100AQwen27.cpp" -lnetwork -o "${output}Test"
shasum -a 256 "${output}FakeWorker" "${output}Test"
