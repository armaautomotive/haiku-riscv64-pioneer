#!/bin/sh
# Pristine pinned ggml ONLY;never builds/edits dirty llama sources.
set -eu
[ "$#" -le 1 ] || exit 2
gdn_image=${1:-}
if [ -n "$gdn_image" ]; then
 actual=$(shasum -a 256 "$gdn_image/code.bin"); [ "${actual%% *}" = 0c61dc435a3c2e00c8ae8a11319a307c86707b410100f7fbc85d15f8587bfcbe ]
 actual=$(shasum -a 256 "$gdn_image/boot.bin"); [ "${actual%% *}" = e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b ]
fi
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
project=$(CDPATH= cd -- "$directory/../../.." && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-gdn-graph.XXXXXXXX")
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
 "$directory/pioneer_q38_gdn_graph_test.cpp" "$temporary"/*.o -o "$temporary/graph"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/graph"
if [ -n "$gdn_image" ]; then
 # Extract complete, unchanged upstream functions, not a rewritten reference.
 # Each selected multiline body ends with a column-zero closing brace.
 extract_function() {
  awk -v signature="$2" '
   !seen && $0 ~ signature { seen=1; print; if ($0 ~ /} *$/) { done=1; exit }; next }
   seen { print; if ($0 ~ /^}/) { done=1; exit } }
   END { if (!seen || !done) exit 1 }
  ' "$1"
 }
 {
  extract_function "$source/src/ggml-cpu/vec.cpp" '^void ggml_vec_dot_f32[(]'
  extract_function "$source/src/ggml-cpu/vec.h" '^inline static void ggml_vec_mul_f32 '
  extract_function "$source/src/ggml-cpu/vec.h" '^inline static void ggml_vec_mad_f32[(]'
  extract_function "$source/src/ggml-cpu/vec.h" '^inline static void ggml_vec_scale_f32[(]'
 } > "$temporary/pinned_gdn_vectors.inc"
 extract_function "$source/src/ggml-cpu/ops.cpp" '^static void ggml_compute_forward_gated_delta_net_one_chunk[(]' > "$temporary/pinned_gdn_chunk.inc"
 ${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -ffp-contract=off "$@" -I"$temporary" \
  "$directory/pioneer_q38_gdn_reference_test.cpp" "$temporary"/*.o -o "$temporary/reference"
 ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/reference" "$gdn_image"
fi
printf 'Pristine pinned GGML GDN graph evidence retained at %s;NO hardware accessed\n' "$temporary"
