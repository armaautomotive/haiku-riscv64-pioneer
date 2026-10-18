#!/bin/sh
# Pristine pinned ggml base only;never reads/builds dirty llama implementation.
set -eu
[ "$#" = 0 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
project=$(CDPATH= cd -- "$directory/../../.." && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-ggml.XXXXXXXX")
git -C "$project/src/libs/llama.cpp" archive c060ca974c773c7c3d17fd1b66dc9d312bc292c0 ggml LICENSE | tar -x -C "$temporary"
source=$temporary/ggml
set -- -O1 -g -fsanitize=address,undefined -D_DARWIN_C_SOURCE -DGGML_SCHED_MAX_COPIES=4 \
 '-DGGML_VERSION="0.21.0"' '-DGGML_COMMIT="c060ca974c77"' -I"$source/include" -I"$source/src"
for name in ggml ggml-alloc ggml-quants; do
 if [ "$name" = ggml ]; then
  ${CC:-cc} -std=c11 "$@" -fno-sanitize=pointer-overflow -c "$source/src/$name.c" -o "$temporary/$name.c.o"
 else
  ${CC:-cc} -std=c11 "$@" -c "$source/src/$name.c" -o "$temporary/$name.c.o"
 fi
done
for name in ggml ggml-backend ggml-backend-meta ggml-threading ggml-opt gguf ggml-backend-dl ggml-backend-reg; do
 ${CXX:-c++} -std=c++17 "$@" -c "$source/src/$name.cpp" -o "$temporary/$name.cpp.o"
done
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror "$@" \
 "$directory/../PioneerQ38GgmlBackend.cpp" "$directory/pioneer_q38_ggml_backend_test.cpp" \
 "$temporary"/*.o -o "$temporary/test"
for fault in 0 1 2 3 4; do ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" "$fault"; done
printf 'Pristine pinned ggml bridge test build retained at %s;NO hardware was accessed\n' "$temporary"
