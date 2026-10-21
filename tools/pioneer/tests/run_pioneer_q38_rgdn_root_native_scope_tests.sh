#!/bin/sh
# Actual extracted native gate/physical audit,NO native operation.
set -eu
[ "$#" = 1 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-rgdn-native-scope.XXXXXXXX")
source=$directory/../../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp
for entry in Q38_RGDN_ROOT_NATIVE_SCOPE:root_scope Q38_NONROOT_NATIVE_OWNER:owner_scope Q38_RGDN_ROOT_NATIVE_HELD:root_held; do
 marker=PIONEER_${entry%:*}; output=${entry##*:}
 awk -v begin="${marker}_BEGIN" -v end="${marker}_END" '
  index($0,begin) {copying=1;next} index($0,end) {found=1;exit}
  copying {print} END {if(!found)exit 1}' "$source" > "$temporary/$output.inc"
done
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DPIONEER_Q38_GDN_MUTABLE_STATE -DPIONEER_Q38_RGDN_ROOT_QUALIFICATION -DPIONEER_Q38_RESIDENT_GDN_ROOT_DIAGNOSTIC \
 "$directory/pioneer_q38_rgdn_root_native_scope_test.cpp" -I"$temporary" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" "$1"
printf 'Offline native resident-root scope evidence retained at %s;NO native selection\n' "$temporary"
