#!/bin/sh
# Exercise the real optional device-manager service with simulated nodes.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-retained-service.XXXXXXXX")
trap 'rm -rf "$temporary"' EXIT
source=$directory/../../../src/system/kernel/device_manager/device_manager.cpp
for pair in INIT_LIFETIME:init_lifetime RETAINED_PIN_SERVICE:retained_pin_service; do
 section=${pair%%:*}; name=${pair#*:}
 awk -v begin="PIONEER_DM_${section}_BEGIN" -v end="PIONEER_DM_${section}_END" \
  'index($0,begin) {copying=1;next} index($0,end) {found=1;exit} copying {print} END {if(!found)exit 1}' \
  "$source" > "$temporary/pioneer_dm_$name.inc"
done
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -pthread \
 -I"$temporary" "$directory/pioneer_retained_pin_service_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test"
