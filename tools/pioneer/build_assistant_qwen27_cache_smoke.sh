#!/bin/sh
# Cross-build isolated LOAD/UNLOAD probe; no install/run/model/card access.
set -eu
[ "$#" = 2 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
build=$(CDPATH= cd -- "$1" && pwd)
output=$2
case "$output" in /*) ;; *) exit 2;; esac
[ ! -e "$output" ] && [ ! -L "$output" ] || exit 1
worker=/boot/home/pioneer-lossless-cache-20261103-a/HaikuAssistantQwen27-cached-quiet-model-progress
sdk=$build/objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop
release=$build/objects/haiku/riscv64/release
set -- -I"$sdk/headers" -idirafter "$sdk/headers/posix"
for include in "$sdk/headers/os" "$sdk"/headers/os/*; do [ ! -d "$include" ] || set -- "$@" -isystem "$include"; done
for library in "$build"/build_packages/gcc_syslibs-*/lib; do set -- "$@" -L"$library"; done
"$build/cross-tools-riscv64/bin/riscv64-unknown-haiku-g++" -std=c++17 -O2 -Wall -Wextra -Werror -fno-exceptions -fno-rtti \
 "-DASSISTANT_QWEN27_WORKER=\"$worker\"" "$@" \
 "$directory/tests/pioneer_assistant_qwen27_cache_load_smoke.cpp" "$directory/../../src/apps/assistant/P100AQwen27.cpp" \
 -B"$release/system/glue/" -L"$release/system/libroot" -L"$release/system/libnetwork" -lnetwork -o "$output"
shasum -a 256 "$output"
