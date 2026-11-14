#!/bin/sh
# Only pristine pinned GGML arithmetic is built; dirty llama files are untouched.
set -eu
[ "$#" = 0 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
project=$(CDPATH= cd -- "$directory/../../.." && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-flash-next.XXXXXXXX")
git -C "$project/src/libs/llama.cpp" archive c060ca974c773c7c3d17fd1b66dc9d312bc292c0 ggml LICENSE | tar -x -C "$temporary"
source=$temporary/ggml
set -- -O1 -g -ffp-contract=off -fsanitize=address,undefined -D_DARWIN_C_SOURCE -DGGML_SCHED_MAX_COPIES=4 \
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
 "$directory/pioneer_flash_next_offload_test.cpp" "$temporary"/*.o -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test"
PYTHONDONTWRITEBYTECODE=1 python3 "$directory/pioneer_flash_next_preflight_test.py"
printf 'Offline Flash-Next test build retained at %s; no device access\n' "$temporary"
