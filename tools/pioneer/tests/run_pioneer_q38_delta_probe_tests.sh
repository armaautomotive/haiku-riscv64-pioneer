#!/bin/sh
set -eu
[ "$#" = 1 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-delta-probe.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_q38_delta_probe_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" "$1"
source=$directory/../../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp
for section in PROBE_LOCK Q38_DELTA_CONTROL Q38_DISPATCH_RETAIN Q38_DISPATCH_COOKIE_RELEASE; do
 marker=PIONEER_$section
 case "$section" in PROBE_LOCK) name=pioneer_lock;; *) name=$(printf '%s' "$marker" | tr '[:upper:]' '[:lower:]');; esac
 awk -v begin="${marker}_BEGIN" -v end="${marker}_END" '
  index($0,begin) {copying=1;next} index($0,end) {found=1;exit} copying {print} END {if(!found)exit 1}
 ' "$source" > "$temporary/$name.inc"
done
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 -I"$temporary" "$directory/pioneer_q38_delta_control_test.cpp" -o "$temporary/control"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/control" "$1"
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 -I"$temporary" -I"$directory/cli-stubs" "$directory/pioneer_q38_delta_client_test.cpp" -o "$temporary/client"
if ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/client" "$1" > "$temporary/client.log" 2>&1; then
 tail -n 1 "$temporary/client.log"
else
 tail -n 40 "$temporary/client.log"; exit 1
fi
printf 'Offline LOCAL vector lease evidence retained at %s\n' "$temporary"
