#!/bin/sh
# Actual entry+controller+emitted image,simulated hardware fabric only.
set -eu
[ "$#" = 2 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-rgdn-root-control.XXXXXXXX")
source=$directory/../../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp
for entry in PROBE_LOCK:native_lock Q38_RETAIN_MEMORY:native_retain Q38_GDN_CALL_SCOPE:call_scope Q38_RGDN_ROOT_CONTROL:root_control; do
 marker=PIONEER_${entry%:*}; output=${entry##*:}
 awk -v begin="${marker}_BEGIN" -v end="${marker}_END" '
  index($0,begin) {copying=1;next} index($0,end) {found=1;exit}
  copying {print} END {if(!found)exit 1}' "$source" > "$temporary/$output.inc"
done
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DPIONEER_Q38_GDN_MUTABLE_STATE -DPIONEER_Q38_RESIDENT_GDN_ROOT_DIAGNOSTIC -DPIONEER_Q38_NORMAL_CLOCK \
 -DPIONEER_Q38_LOCAL_DELTA_GGML -DTT_PIONEER_DRIVER_RELOAD -DTT_PIONEER_EXPERIMENTAL_Q38_BANKS \
 "$directory/pioneer_q38_rgdn_root_control_test.cpp" -I"$temporary" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" "$1" "$2"
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DPIONEER_Q38_GDN_MUTABLE_STATE -DPIONEER_Q38_RESIDENT_GDN_ROOT_DIAGNOSTIC -DPIONEER_Q38_NORMAL_CLOCK \
 -DPIONEER_Q38_LOCAL_DELTA_GGML -DTT_PIONEER_DRIVER_RELOAD -DTT_PIONEER_EXPERIMENTAL_Q38_BANKS \
 "$directory/pioneer_q38_rgdn_root_client_test.cpp" -I"$temporary" -o "$temporary/client"
if ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/client" "$1" "$2" > "$temporary/client.log" 2>&1; then
 tail -n 1 "$temporary/client.log"
else tail -n 40 "$temporary/client.log"; exit 1; fi
printf 'Offline actual resident-root entry evidence retained at %s;NO native module/selection\n' "$temporary"
