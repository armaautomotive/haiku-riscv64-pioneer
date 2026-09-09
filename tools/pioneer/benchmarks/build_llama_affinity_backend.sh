#!/bin/sh
set -eu
repo=$(CDPATH= cd -- "$(dirname "$0")/../../.." && pwd)
out=${1:?usage: build_llama_affinity_backend.sh OBJECT_DIRECTORY [HAIKU_BUILD_DIRECTORY]}
build=${2:-/Volumes/HaikuBuildLocal/generated.riscv64}
sdk=$build/objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop
cross=$build/cross-tools-riscv64/bin/riscv64-unknown-haiku
src=$repo/src/libs/llama.cpp
git -C "$src" show HEAD:ggml/src/ggml-cpu/ggml-cpu.c > "$out/baseline.c"
for variant in baseline affinity; do
    mkdir -p "$out/$variant"
    input=$out/baseline.c
    if [ "$variant" = affinity ]; then input=$src/ggml/src/ggml-cpu/ggml-cpu.c; fi
    "$cross-gcc" -O3 -DNDEBUG -std=gnu11 -fPIC -Wall -Wextra -Wno-unused-function -Werror=implicit-function-declaration \
        -DGGML_BACKEND_BUILD -DGGML_BACKEND_SHARED -DGGML_SCHED_MAX_COPIES=4 -DGGML_SHARED \
        -DGGML_USE_CPU_REPACK -DGGML_USE_LLAMAFILE -D_XOPEN_SOURCE=600 -Dggml_cpu_EXPORTS -DGGML_CPU_GENERIC \
        -I"$src/ggml" -I"$src/ggml/src" -I"$src/ggml/src/ggml-cpu" -I"$src/ggml/include" \
        -I"$sdk/headers/os/kernel" -I"$sdk/headers/os/support" -I"$sdk/headers/os/storage" -I"$sdk/headers/os" \
        -I"$sdk/headers" -isystem "$sdk/headers/posix" -c "$input" -o "$out/$variant/ggml-cpu.c.o"
    "$cross-gcc" -shared -Wl,-soname,libggml-cpu.so.0 \
        -B"$build/objects/haiku/riscv64/release/system/glue/" \
        -L"$build/objects/haiku/riscv64/release/system/libroot" \
        "$out/$variant/ggml-cpu.c.o" \
        "$out/native-objects/ggml-cpu.cpp.o" "$out/native-objects/repack.cpp.o" \
        "$out/native-objects/hbm.cpp.o" "$out/native-objects/quants.c.o" \
        "$out/native-objects/traits.cpp.o" "$out/native-objects/amx/amx.cpp.o" \
        "$out/native-objects/amx/mmq.cpp.o" "$out/native-objects/binary-ops.cpp.o" \
        "$out/native-objects/unary-ops.cpp.o" "$out/native-objects/vec.cpp.o" \
        "$out/native-objects/ops.cpp.o" "$out/native-objects/llamafile/sgemm.cpp.o" \
        "$out/libggml-base.so.0.21.0" \
        "$build/build_packages/gcc_syslibs-13.2.0_2023_08_10-2-riscv64/lib/libstdc++.so.6.0.32" \
        -o "$out/$variant/libggml-cpu.so.0"
done
