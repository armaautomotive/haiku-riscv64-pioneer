#!/bin/sh
# Validate the isolated sharded worker, full-graph recording, and compile guards.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/q35-dense-shards.XXXXXXXX")
trap 'rm -rf "$temporary"' EXIT
for name in dense_attention_shards dense_attention_shards_graph dense_attention_math dense_attention_worker; do
 ${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
  "$directory/pioneer_qwen35_dataflow_${name}_test.cpp" -o "$temporary/$name"
 ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/$name"
done
header=$directory/../PioneerQwen35DataflowDenseAttentionShardProgram.h
printf '#include "%s"\n' "$header" > "$temporary/guard.cpp"
for flags in '' '-DPIONEER_QWEN35_DENSE_SHARD_COMPILE_ONLY' '-DPIONEER_QWEN35_RECOVERY_COMPILE_ONLY' \
 '-DPIONEER_QWEN35_DENSE_SHARD_COMPILE_ONLY -DPIONEER_QWEN35_RECOVERY_COMPILE_ONLY -DPIONEER_QWEN35_RECOVERY_QUALIFY'; do
 # Deliberate word splitting provides separate compiler arguments.
 if ${CXX:-c++} -std=c++17 $flags -fsyntax-only "$temporary/guard.cpp" > /dev/null 2>&1; then
  echo 'FAIL dense shard build guard' >&2;exit 1
 fi
done
echo 'PASS dense shard compile guards reject unguarded and qualification builds'
