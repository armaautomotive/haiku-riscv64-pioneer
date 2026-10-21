#!/bin/sh
# ACTUAL extracted lock+call guard+native scope,synthetic CPU fabric only.
set -eu
[ "$#" = 0 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-gdn-call-scope.XXXXXXXX")
source=$directory/../../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp
for entry in PROBE_LOCK:native_lock Q38_GDN_CALL_SCOPE:call_scope Q38_STATE_ZERO_NATIVE_SCOPE:native_scope Q38_GDN_BIND_OWNER:bind_owner; do
 marker=PIONEER_${entry%:*}; output=${entry##*:}
 awk -v begin="${marker}_BEGIN" -v end="${marker}_END" '
  index($0,begin) {copying=1;next} index($0,end) {found=1;exit}
  copying {print} END {if(!found)exit 1}' "$source" > "$temporary/$output.inc"
done
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DPIONEER_Q38_GDN_MUTABLE_STATE -DTT_PIONEER_EXPERIMENTAL_Q38_BANKS -DTT_PIONEER_DRIVER_RELOAD \
 -I"$temporary" "$directory/pioneer_q38_gdn_call_scope_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test"
printf 'Offline private GDN call-scope evidence retained at %s\n' "$temporary"
