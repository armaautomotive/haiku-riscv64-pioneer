#!/bin/sh
set -eu
[ "$#" = 1 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-span-service.XXXXXXXX")
source=$directory/../../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp
for pair in PROBE_LOCK:pioneer_lock Q38_RETAIN_MEMORY:pioneer_retain_memory Q38_SPAN_CONTROL:pioneer_span_control; do
 marker=PIONEER_${pair%%:*}; name=${pair#*:}
 awk -v begin="${marker}_BEGIN" -v end="${marker}_END" '
  index($0,begin) {copying=1;next} index($0,end) {found=1;exit} copying {print} END {if(!found)exit 1}
 ' "$source" > "$temporary/$name.inc"
done
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DTT_PIONEER_EXPERIMENTAL_Q38_BANKS -I"$temporary" "$directory/pioneer_q38_span_service_test.cpp" -o "$temporary/service"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/service" "$1"
for kind in session public_client; do
 ${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
  -DPIONEER_Q38_SPAN_PRELOAD -DTT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_VECTOR \
  "$directory/pioneer_q38_span_${kind}_test.cpp" -o "$temporary/$kind"
 ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/$kind"
done
printf 'Offline native span adapter evidence retained at %s\n' "$temporary"
