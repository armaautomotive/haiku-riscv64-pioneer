#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-stream-policy.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_q38_stream_probe_test.cpp" -o "$temporary/test"
UBSAN_OPTIONS=halt_on_error=1 "$temporary/test"
source=$directory/../../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp
for section in LOCK Q38_PANEL_CONTROL Q38_STREAM_CONTROL; do
 case "$section" in LOCK) marker=PIONEER_PROBE_LOCK; name=pioneer_lock;; *) marker=PIONEER_$section; name=$(printf '%s' "$marker" | tr '[:upper:]' '[:lower:]');; esac
 awk -v begin="${marker}_BEGIN" -v end="${marker}_END" '
  index($0,begin) {copying=1;next} index($0,end) {found=1;exit} copying {print} END {if(!found)exit 1}
 ' "$source" > "$temporary/$name.inc"
done
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 -I"$temporary" "$directory/pioneer_q38_stream_control_test.cpp" -o "$temporary/control"
UBSAN_OPTIONS=halt_on_error=1 "$temporary/control"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DTT_PIONEER_EXPERIMENTAL_Q38_ROWS "$directory/pioneer_q38_stream_probe_test.cpp" -o "$temporary/rows"
UBSAN_OPTIONS=halt_on_error=1 "$temporary/rows"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DTT_PIONEER_EXPERIMENTAL_Q38_ROWS -I"$temporary" "$directory/pioneer_q38_stream_control_test.cpp" -o "$temporary/rows-control"
UBSAN_OPTIONS=halt_on_error=1 "$temporary/rows-control"
printf 'Offline fullK launch policy evidence retained at %s\n' "$temporary"
