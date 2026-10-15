#!/bin/sh
# Standalone cross-build outside the user-modified llama.cpp checkout.
set -eu
[ "$#" = 3 ] || { echo 'Usage: build_qwen_resident.sh CONFIGURED_BUILD MATCHING_NATIVE_LIBS FRESH_OUTPUT' >&2; exit 2; }
project=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
build=$(CDPATH= cd -- "$1" && pwd)
libraries=$(CDPATH= cd -- "$2" && pwd)
output=$3
case "$output" in /*) ;; *) echo 'Output must be absolute' >&2; exit 1;; esac
[ ! -e "$output" ] && [ ! -L "$output" ] || { echo 'Output already exists' >&2; exit 1; }
source=$project/src/libs/llama.cpp
[ "$(git -C "$source" rev-parse HEAD)" = c060ca974c773c7c3d17fd1b66dc9d312bc292c0 ] || { echo 'Expected pinned llama revision' >&2; exit 1; }
git -C "$source" diff --quiet HEAD -- include/llama.h ggml/include ggml/src/ggml-backend-impl.h
# These are the matching runtime binaries used by the independent native
# CPU baseline. A different build needs a deliberate ABI/baseline recheck.
for name in libllama.so.0.2.0 libggml.so.0.21.0 libggml-base.so.0.21.0 libggml-cpu.so.0.21.0; do
 case "$name" in
  libllama.so.0.2.0) expected=0ae23a2c211af9fbafb89c3d93bdd136a4bdd574baeeaf2d622f582efcf426af;;
  libggml.so.0.21.0) expected=a3b81c133641bbdacde8dd1d0e0ffa7a0a1ec0c33e95c426df1db59735379c16;;
  libggml-base.so.0.21.0) expected=9cb45a36558bd78aa4d4b7ed5385cbf9df32de208f9e6c9b6d064e8527dca389;;
  libggml-cpu.so.0.21.0) expected=69e9ec2c43a123afbb256c3feb6313cdcebdf1b2a87278ac3cdac8a1a65273d2;;
 esac
 actual=$(shasum -a 256 "$libraries/$name"); actual=${actual%% *}
 [ "$actual" = "$expected" ] || { echo "Matching pinned library required: $name" >&2; exit 1; }
done
sdk=$build/objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop
release=$build/objects/haiku/riscv64/release
cxx=$build/cross-tools-riscv64/bin/riscv64-unknown-haiku-g++
set -- -I"$sdk/headers" -idirafter "$sdk/headers/posix"
for include in "$sdk/headers/os" "$sdk"/headers/os/*; do
 [ ! -d "$include" ] || set -- "$@" -isystem "$include"
done
for lib in "$build"/build_packages/gcc_syslibs-*/lib; do set -- "$@" -L"$lib"; done
umask 077
mkdir -m 700 "$output"
"$cxx" -std=c++11 -O2 -Wall -Wextra -Werror -fno-exceptions -fno-rtti "$@" \
 -I"$source/include" -I"$source/ggml/include" -I"$source/ggml/src" \
 -B"$release/system/glue/" -L"$release/system/libroot" -L"$libraries" -Wl,-rpath-link,"$libraries" \
 "$project/tools/pioneer/PioneerGgmlBackend.cpp" "$project/tools/pioneer/tenstorrent_haiku_qwen_resident.cpp" \
 -l:libllama.so.0.2.0 -l:libggml.so.0 -l:libggml-base.so.0 \
 -o "$output/tenstorrent_haiku_qwen_resident"
shasum -a 256 "$output/tenstorrent_haiku_qwen_resident"
printf 'Built standalone experimental Qwen resident test; no device or Assistant was accessed\n'
