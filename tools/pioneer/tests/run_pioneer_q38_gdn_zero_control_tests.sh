#!/bin/sh
# Actual control entry with explicitly mocked initializer,NO native IO.
set -eu
[ "$#" = 0 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-gdn-zero-control.XXXXXXXX")
source=$directory/../../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp
for entry in PROBE_LOCK:native_lock Q38_GDN_CALL_SCOPE:call_scope Q38_STATE_ZERO_NATIVE_SCOPE:native_scope Q38_GDN_BIND_OWNER:bind_owner Q38_GDN_ZERO_CONTROL:zero_control; do
 marker=PIONEER_${entry%:*}; output=${entry##*:}
 awk -v begin="${marker}_BEGIN" -v end="${marker}_END" '
  index($0,begin) {copying=1;next} index($0,end) {found=1;exit}
  copying {print} END {if(!found)exit 1}' "$source" > "$temporary/$output.inc"
done
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DPIONEER_Q38_GDN_MUTABLE_STATE -DTT_PIONEER_EXPERIMENTAL_Q38_BANKS -DTT_PIONEER_DRIVER_RELOAD \
 -I"$temporary" "$directory/pioneer_q38_gdn_zero_control_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DPIONEER_Q38_GDN_MUTABLE_STATE -DTT_PIONEER_EXPERIMENTAL_Q38_BANKS -DTT_PIONEER_DRIVER_RELOAD \
 -I"$temporary" "$directory/pioneer_q38_gdn_zero_client_test.cpp" -o "$temporary/client"
if ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/client" > "$temporary/client.log" 2>&1; then
 tail -n 1 "$temporary/client.log"
else tail -n 35 "$temporary/client.log"; exit 1; fi
printf 'Offline fixed-zero control evidence retained at %s\n' "$temporary"
