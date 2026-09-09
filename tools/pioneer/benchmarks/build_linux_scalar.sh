#!/bin/sh
set -eu
comparison=${1:?usage: build_linux_scalar.sh STAGING_DIRECTORY}
cd "$comparison"
test "$(sha256sum llama-c060ca-linux-baseline.tar.gz | awk '{print $1}')" = ef1286b5bc643e2394957f655f05be468bbf505306cb5a9e2508ac2746957528
test "$(sha256sum Qwen3-0.6B-Q8_0.gguf.part | awk '{print $1}')" = 9465e63a22add5354d9bb4b99e90117043c7124007664907259bd16d043bb031
test ! -e source
mkdir source tmp
tar -xzf llama-c060ca-linux-baseline.tar.gz -C source
export TMPDIR="$comparison/tmp"
cmake -S source -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=/usr/bin/gcc \
    -DCMAKE_CXX_COMPILER=/usr/bin/g++ \
    -DCMAKE_PROJECT_INCLUDE="$comparison/linux-scalar-generic.cmake" \
    '-DCMAKE_C_FLAGS=-march=rv64gc -mabi=lp64d' \
    '-DCMAKE_CXX_FLAGS=-march=rv64gc -mabi=lp64d' \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DBUILD_SHARED_LIBS=ON \
    -DGGML_CCACHE=OFF -DGGML_NATIVE=OFF -DGGML_OPENMP=OFF \
    -DGGML_RVV=OFF -DGGML_RV_ZFH=OFF -DGGML_RV_ZVFH=OFF \
    -DGGML_RV_ZICBOP=OFF -DGGML_RV_ZIHINTPAUSE=OFF -DGGML_XTHEADVECTOR=OFF \
    -DLLAMA_OPENSSL=OFF -DLLAMA_BUILD_TESTS=OFF -DLLAMA_BUILD_EXAMPLES=OFF \
    -DLLAMA_BUILD_SERVER=OFF -DLLAMA_BUILD_APP=OFF -DLLAMA_BUILD_UI=OFF \
    -DLLAMA_BUILD_COMMIT=c060ca974c77 -DLLAMA_BUILD_NUMBER=0
cmake --build build --target llama-bench llama-completion --parallel 32
