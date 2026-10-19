#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-p100a-reload.XXXXXXXX")
source=$directory/../../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp
for pair in DRIVER_RELOAD_STATE:reload_state DRIVER_RELOAD_CONTROL:reload_control PROBE_LOCK:lock; do
 section=${pair%%:*}; name=${pair#*:}
 awk -v begin="PIONEER_${section}_BEGIN" -v end="PIONEER_${section}_END" '
  index($0,begin) {copying=1;next} index($0,end) {found=1;exit} copying {print} END {if(!found)exit 1}
 ' "$source" > "$temporary/pioneer_$name.inc"
done
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 -I"$directory/p100a_reload_stubs" -I"$temporary" "$directory/pioneer_p100a_reload_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test"
printf 'Offline reload guard evidence retained at %s\n' "$temporary"
