#!/bin/sh
# Separate test probes; never install or run the candidate.
set -eu
[ "$#" = 2 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
build=$(CDPATH= cd -- "$1" && pwd)
output=$2
case "$output" in /*) ;; *) exit 2;; esac
[ -d "$output" ] && [ ! -L "$output" ] || exit 1
for file in host-probe card-probe; do [ ! -e "$output/$file" ] && [ ! -L "$output/$file" ] || exit 1; done
worker=/boot/home/pioneer-fleet-reuse-20261113-a/worker
sdk=$build/objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop
release=$build/objects/haiku/riscv64/release
set -- -I"$sdk/headers" -idirafter "$sdk/headers/posix"
for include in "$sdk/headers/os" "$sdk"/headers/os/*; do [ ! -d "$include" ] || set -- "$@" -isystem "$include"; done
for library in "$build"/build_packages/gcc_syslibs-*/lib; do set -- "$@" -L"$library"; done
for mode in host card; do
 if [ "$mode" = host ]; then source=pioneer_assistant_qwen27_cache_load_smoke.cpp; else source=pioneer_assistant_qwen27_cache_inference_smoke.cpp; fi
 "$build/cross-tools-riscv64/bin/riscv64-unknown-haiku-g++" -std=c++17 -O2 -Wall -Wextra -Werror -fno-exceptions -fno-rtti \
  -DPIONEER_BATCHED_LOAD_TIMING=1 "-DASSISTANT_QWEN27_WORKER=\"$worker\"" "$@" \
  "$directory/tests/$source" "$directory/../../src/apps/assistant/P100AQwen27.cpp" \
  -B"$release/system/glue/" -L"$release/system/libroot" -L"$release/system/libnetwork" -lnetwork -o "$output/$mode-probe"
done
shasum -a 256 "$output/host-probe" "$output/card-probe"
