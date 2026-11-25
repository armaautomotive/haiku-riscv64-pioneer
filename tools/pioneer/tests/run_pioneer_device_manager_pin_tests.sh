#!/bin/sh
# Exercise the actual device-manager pin and lifetime methods over simulated nodes.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
source=$directory/../../../src/system/kernel/device_manager/device_manager.cpp
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-dm-pin.XXXXXXXX")
trap 'rm -rf "$temporary"' EXIT
for pair in DRIVER_PIN:driver_pin INIT_LIFETIME:init_lifetime; do
 section=${pair%%:*}; name=${pair#*:}
 awk -v begin="PIONEER_DM_${section}_BEGIN" -v end="PIONEER_DM_${section}_END" \
  'index($0,begin) {copying=1;next} index($0,end) {found=1;exit} copying {print} END {if(!found)exit 1}' \
  "$source" > "$temporary/pioneer_dm_$name.inc"
done
for name in pin real_lifetime; do
 ${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -pthread \
  -I"$temporary" "$directory/pioneer_device_manager_${name}_test.cpp" -o "$temporary/$name"
 ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/$name"
done
