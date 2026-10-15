#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-brisc-policy.XXXXXXXX")
trap 'rm -f "$temporary/test" "$temporary/lock-test" "$temporary/pioneer_lock.inc"; rmdir "$temporary"' EXIT HUP INT TERM
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_brisc_probe_test.cpp" -o "$temporary/test"
"$temporary/test"
awk '
 /PIONEER_PROBE_LOCK_BEGIN/ {copying=1; next}
 /PIONEER_PROBE_LOCK_END/ {found=1; exit}
 copying {print}
 END {if(!found) exit 1}
' "$directory/../../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp" > "$temporary/pioneer_lock.inc"
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined \
 -I "$temporary" "$directory/pioneer_brisc_lock_test.cpp" -o "$temporary/lock-test"
"$temporary/lock-test"
awk '
 /acquire_sem\(sLock\)/ {acquires++}
 /status = AcquireProbeLock\(\)/ {gated++}
 END {if(acquires!=1 || gated!=23) exit 1}
' "$directory/../../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp"
printf 'PASS: fixed BRISC/TRISC0 scalar policies; dropped writes, timeout, unsafe-entry/stop and rollback gates\n'
