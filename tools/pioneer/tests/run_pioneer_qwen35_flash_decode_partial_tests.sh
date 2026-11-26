#!/bin/sh
# Host numerical witness only; this never loads a driver or accesses a card.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/q35-flash-partial.XXXXXXXX")
trap 'rm -rf "$temporary"' EXIT
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_qwen35_dataflow_flash_decode_partial_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test"
header=$directory/../PioneerQwen35DataflowDenseAttentionPartial.h
printf '#include "%s"\n' "$header" > "$temporary/guard.cpp"
for flags in '' '-DPIONEER_QWEN35_FLASH_DECODE_COMPILE_ONLY' '-DPIONEER_QWEN35_RECOVERY_COMPILE_ONLY' \
 '-DPIONEER_QWEN35_FLASH_DECODE_COMPILE_ONLY -DPIONEER_QWEN35_RECOVERY_COMPILE_ONLY -DPIONEER_QWEN35_RECOVERY_QUALIFY'; do
 # Deliberate word splitting provides separate compiler arguments.
 if ${CXX:-c++} -std=c++17 $flags -fsyntax-only "$temporary/guard.cpp" > /dev/null 2>&1; then
  echo 'FAIL flash-decode build guard' >&2;exit 1
 fi
done
echo 'PASS flash-decode partials reject unguarded and qualification builds'
