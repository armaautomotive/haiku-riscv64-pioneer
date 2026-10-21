#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-sentinel-tag.XXXXXXXX")
for variant in sentinel host batch4 raw frontend raw-cache prefetch command peer-phase q4-nibble;do
extra=;[ "$variant" != host ] || extra=-DPIONEER_Q38_HOST_PHASE_REFERENCE_CLIENT
[ "$variant" != batch4 ] || extra=-DPIONEER_Q38_BATCH4_REFERENCE_CLIENT
[ "$variant" != raw ] || extra=-DPIONEER_Q38_BATCH_RAW_REFERENCE_CLIENT
[ "$variant" != frontend ] || extra=-DPIONEER_Q38_FRONTEND_REFERENCE_CLIENT
[ "$variant" != raw-cache ] || extra=-DPIONEER_Q38_RAW_CACHE_REFERENCE_CLIENT
[ "$variant" != prefetch ] || extra=-DPIONEER_Q38_PREFETCH_REFERENCE_CLIENT
[ "$variant" != command ] || extra=-DPIONEER_Q38_COMMAND_REFERENCE_CLIENT
[ "$variant" != peer-phase ] || extra=-DPIONEER_Q38_PEER_PHASE_REFERENCE_CLIENT
[ "$variant" != q4-nibble ] || extra=-DPIONEER_Q38_Q4_NIBBLE_REFERENCE_CLIENT
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined $extra \
 "$directory/pioneer_q38_sentinel_debug_tag_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test"
done
printf 'Offline sentinel-root debug-tag evidence retained at %s\n' "$temporary"
