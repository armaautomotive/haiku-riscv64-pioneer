#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-ffn-repeat-loop.XXXXXXXX")
awk '/PIONEER_FFN_REPEAT_LOOP_BEGIN/ {copying=1;next} /PIONEER_FFN_REPEAT_LOOP_END/ {found=1;exit}
 copying {print} END {if(!found)exit 1}' "$directory/../pioneer_q38_inference.cpp" > "$temporary/ffn_repeat_loop.inc"
for outputs in 5120 10240; do
 ${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
  -DPIONEER_TEST_GRAPH_OUTPUTS="$outputs" -I"$temporary" "$directory/pioneer_q38_ffn_repeat_loop_test.cpp" -o "$temporary/test-$outputs"
 ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test-$outputs"
done
printf 'Offline ACTUAL FFN/Q8 repeat-loop evidence retained at %s\n' "$temporary"
