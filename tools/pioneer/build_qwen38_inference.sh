#!/bin/sh
# Cross-link frozen ABI-matching runtime,NOT a build of dirty llama.cpp.
set -eu
[ "$#" = 3 ] || exit 2
project=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
build=$(CDPATH= cd -- "$1" && pwd)
libraries=$(CDPATH= cd -- "$2" && pwd)
output=$3
case "${PIONEER_Q38_DISPATCH_INFERENCE:-0}" in
 0) extra=;;
 1) extra=-DPIONEER_Q38_DISPATCH_INFERENCE;;
 *) exit 2;;
esac
case "${PIONEER_Q38_VECTOR_INFERENCE:-0}" in
 0) ;;
 1) [ "${PIONEER_Q38_DISPATCH_INFERENCE:-0}" = 1 ] || exit 2; extra="$extra -DTT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_VECTOR";;
 2) [ "${PIONEER_Q38_DISPATCH_INFERENCE:-0}" = 1 ] || exit 2; extra="$extra -DTT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_VECTOR -DTT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_BFLOAT";;
 *) exit 2;;
esac
case "${PIONEER_Q38_EXECUTION_MODE:-checked}" in
 checked) ;;
 work) [ "${PIONEER_Q38_VECTOR_INFERENCE:-0}" = 1 ] || exit 2; extra="$extra -DPIONEER_Q38_WORK_INFERENCE";;
 cpu-validated) [ "${PIONEER_Q38_VECTOR_INFERENCE:-0}" = 1 ] || exit 2; extra="$extra -DPIONEER_Q38_CPU_VALIDATION";;
 *) exit 2;;
esac
case "$output" in /*) ;; *) exit 2;; esac
case "${PIONEER_Q38_SPAN_PRELOAD:-0}" in
 0) ;;
 1) [ "${PIONEER_Q38_EXECUTION_MODE:-checked}" = work ] || exit 2
    extra="$extra -DPIONEER_Q38_SPAN_PRELOAD";;
 *) exit 2;;
esac
case "${PIONEER_Q38_SPAN_FAST:-0}" in
 0) ;;
 1) [ "${PIONEER_Q38_SPAN_PRELOAD:-0}" = 1 ] || exit 2
    extra="$extra -DPIONEER_Q38_SPAN_FAST";;
 *) exit 2;;
esac
case "${PIONEER_Q38_ORDERED_EXTENT_FAST:-0}" in
 0) ;;
 1) [ "${PIONEER_Q38_EXECUTION_MODE:-checked}:${PIONEER_Q38_SPAN_PRELOAD:-0}:${PIONEER_Q38_SPAN_FAST:-0}" = work:1:1 ] || exit 2
    extra="$extra -DPIONEER_Q38_ORDERED_EXTENT_FAST";;
 *) exit 2;;
esac
case "${PIONEER_Q38_ASSISTANT_WORKER:-0}" in
 0) worker_libraries=;;
 1) [ "${PIONEER_Q38_EXECUTION_MODE:-checked}:${PIONEER_Q38_SPAN_PRELOAD:-0}:${PIONEER_Q38_SPAN_FAST:-0}:${PIONEER_Q38_CPU_POOL_PROBE:-0}" = work:1:1:0 ] || exit 2
    worker_libraries=-lnetwork;;
 *) exit 2;;
esac
[ ! -e "$output" ] && [ ! -L "$output" ] || exit 1
for name in libllama.so.0.2.0 libggml.so.0.21.0 libggml-base.so.0.21.0 libggml-cpu.so.0.21.0; do
 case "$name" in
  libllama.so.0.2.0) expected=0ae23a2c211af9fbafb89c3d93bdd136a4bdd574baeeaf2d622f582efcf426af;;
  libggml.so.0.21.0) expected=a3b81c133641bbdacde8dd1d0e0ffa7a0a1ec0c33e95c426df1db59735379c16;;
  libggml-base.so.0.21.0) expected=9cb45a36558bd78aa4d4b7ed5385cbf9df32de208f9e6c9b6d064e8527dca389;;
  libggml-cpu.so.0.21.0) expected=69e9ec2c43a123afbb256c3feb6313cdcebdf1b2a87278ac3cdac8a1a65273d2;;
 esac
 actual=$(shasum -a 256 "$libraries/$name"); actual=${actual%% *}
 [ "$actual" = "$expected" ] || exit 1
done
umask 077
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-pinned-headers.XXXXXXXX")
git -C "$project/src/libs/llama.cpp" archive c060ca974c773c7c3d17fd1b66dc9d312bc292c0 include/llama.h ggml/include ggml/src/ggml-backend-impl.h | tar -x -C "$temporary"
sdk=$build/objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop
release=$build/objects/haiku/riscv64/release
cxx=$build/cross-tools-riscv64/bin/riscv64-unknown-haiku-g++
set -- -I"$sdk/headers" -idirafter "$sdk/headers/posix"
for include in "$sdk/headers/os" "$sdk"/headers/os/*; do [ ! -d "$include" ] || set -- "$@" -isystem "$include"; done
for library in "$build"/build_packages/gcc_syslibs-*/lib; do set -- "$@" -L"$library"; done
case "${PIONEER_Q38_CPU_POOL_PROBE:-0}" in
 0) program=$project/tools/pioneer/pioneer_q38_inference.cpp
    [ "${PIONEER_Q38_ASSISTANT_WORKER:-0}" = 0 ] || program=$project/tools/pioneer/pioneer_assistant_qwen27.cpp
    set -- "$@" "$project/tools/pioneer/PioneerQ38GgmlBackend.cpp" "$program";;
 1) set -- "$@" "$project/tools/pioneer/pioneer_q38_cpu_pool_probe.cpp";;
 *) exit 2;;
esac
"$cxx" -std=c++17 -O2 -ffp-contract=off -Wall -Wextra -Werror -fno-exceptions -fno-rtti \
 -DTT_PIONEER_EXPERIMENTAL_Q38_COHORT_SEALED $extra "$@" -I"$temporary/include" -I"$temporary/ggml/include" -I"$temporary/ggml/src" \
 -B"$release/system/glue/" -L"$release/system/libroot" -L"$release/system/libnetwork" -L"$libraries" -Wl,-rpath-link,"$libraries" \
 -l:libllama.so.0.2.0 -l:libggml.so.0.21.0 -l:libggml-base.so.0.21.0 -l:libggml-cpu.so.0.21.0 $worker_libraries -o "$output"
shasum -a 256 "$output"
