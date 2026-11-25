#!/bin/sh
# Tests the link pulse and actual recovery ordering against simulated PCI IO.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
root=$(CDPATH= cd -- "$directory/../../.." && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-link-reset.XXXXXXXX")
trap 'rm -rf "$temporary"' EXIT
source=$root/src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp
awk '/PIONEER_QWEN35_RECOVERY_RUN_BEGIN/ {copying=1;next} /PIONEER_QWEN35_RECOVERY_RUN_END/ {found=1;exit} copying {print} END {if(!found)exit 1}' \
 "$source" > "$temporary/pioneer_recovery_run.inc"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_pcie_link_reset_test.cpp" -o "$temporary/link"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/link"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_qwen35_link_pci_restore_guard_test.cpp" -o "$temporary/restore"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/restore"
for mode in asic firmware; do
 flag=
 [ "$mode" != firmware ] || flag=-DTEST_FIRMWARE_RESET
 ${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
  -DTEST_LINK_RESET $flag -I"$temporary" "$directory/pioneer_qwen35_recovery_run_test.cpp" -o "$temporary/$mode"
 ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/$mode"
done
