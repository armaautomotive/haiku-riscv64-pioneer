#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-resident-banks.XXXXXXXX")
trap 'rm -f "$temporary/test" "$temporary/control" "$temporary/pioneer_lock.inc" "$temporary/pioneer_q38_bank_control.inc"; rmdir "$temporary"' EXIT HUP INT TERM
${CXX:-c++} -std=c++17 -O1 -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_resident_banks_test.cpp" -o "$temporary/test"
"$temporary/test"
source=$directory/../../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp
awk '/PIONEER_PROBE_LOCK_BEGIN/ {copying=1;next} /PIONEER_PROBE_LOCK_END/ {found=1;exit} copying {print} END {if(!found)exit 1}' "$source" > "$temporary/pioneer_lock.inc"
awk '/PIONEER_Q38_BANK_CONTROL_BEGIN/ {copying=1;next} /PIONEER_Q38_BANK_CONTROL_END/ {found=1;exit} copying {print} END {if(!found)exit 1}' "$source" > "$temporary/pioneer_q38_bank_control.inc"
${CXX:-c++} -std=c++17 -O1 -Wall -Wextra -Werror -fsanitize=address,undefined \
 -I "$temporary" "$directory/pioneer_q38_bank_control_test.cpp" -o "$temporary/control"
"$temporary/control"
printf 'PASS: seven-bank private page/bulk route authority,actual control handler,all-bank ownership/bounds,independent PIO roundtrips,ALL21 page+42 bulk unknown stages quarantine EVERY bank\n'
