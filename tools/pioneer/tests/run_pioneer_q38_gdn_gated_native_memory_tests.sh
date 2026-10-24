#!/bin/sh
# Exact new DATA footprint+ACTUAL memory owner,NO hardware/compute authority.
set -eu
[ "$#" = 3 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
output=$(mktemp -d /private/tmp/pioneer-q38-gated-native-memory.XXXXXXXX)
flags='-DPIONEER_Q38_PROJECTION_RESULT_STASH=1 -DPIONEER_Q38_GDN_GATED_NATIVE=1 -DPIONEER_Q38_GDN_RECURRENT_NATIVE=1'
for unit in projection_graph_catalog projection_memory; do
 "${CXX:-c++}" -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined $flags \
  "$directory/pioneer_q38_${unit}_test.cpp" -o "$output/$unit"
done
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$output/projection_graph_catalog" "$1" "$2" "$3"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$output/projection_memory"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$output/projection_memory" --faults
