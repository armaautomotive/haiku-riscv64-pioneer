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
if [ -n "${ASSISTANT_LLAMA_LIB_DIR:-}" ]; then
 headers=$(mktemp -d /private/tmp/assistant27-pool-headers.XXXXXXXX)
 git -C "$project/src/libs/llama.cpp" archive c060ca974c773c7c3d17fd1b66dc9d312bc292c0 ggml/include | tar -x -C "$headers"
 # Same three ABI-pinned ggml libraries used by the installed worker.
 for spec in libggml.so.0.21.0:a3b81c133641bbdacde8dd1d0e0ffa7a0a1ec0c33e95c426df1db59735379c16 libggml-base.so.0.21.0:9cb45a36558bd78aa4d4b7ed5385cbf9df32de208f9e6c9b6d064e8527dca389 libggml-cpu.so.0.21.0:69e9ec2c43a123afbb256c3feb6313cdcebdf1b2a87278ac3cdac8a1a65273d2; do
  actual=$(shasum -a 256 "$ASSISTANT_LLAMA_LIB_DIR/${spec%%:*}"); [ "${actual%% *}" = "${spec#*:}" ] || exit 1
 done
 "$cxx" -std=c++11 -O1 -Wall -Wextra -Werror "$@" -B"$release/system/glue/" \
  -L"$release/system/libroot" -L"$ASSISTANT_LLAMA_LIB_DIR" -Wl,-rpath-link,"$ASSISTANT_LLAMA_LIB_DIR" -I"$headers/ggml/include" \
  "$project/tools/pioneer/tests/assistant_qwen27_cpu_pool_test.cpp" \
  -l:libggml.so.0.21.0 -l:libggml-base.so.0.21.0 -l:libggml-cpu.so.0.21.0 -o "${output}CpuPoolTest"
 shasum -a 256 "${output}CpuPoolTest"
fi
