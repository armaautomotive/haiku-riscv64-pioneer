#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-dm-register.XXXXXXXX")
trap 'rm -rf "$temporary"' EXIT
source=$directory/../../../src/system/kernel/device_manager/device_manager.cpp
awk '/PIONEER_DM_REGISTER_NODE_BEGIN/ {copying=1;next} /PIONEER_DM_REGISTER_NODE_END/ {found=1;exit} copying {print} END {if(!found)exit 1}' \
 "$source" > "$temporary/pioneer_dm_register_node.inc"
${CXX:-c++} -std=c++20 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -pthread \
 -I"$temporary" "$directory/pioneer_device_manager_register_race_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test"
python3 -B - "$directory/../../../src/add-ons/kernel/bus_managers/pci/pci.cpp" "$temporary/pioneer_pci_reserve_device.inc" <<'PY'
from pathlib import Path
import sys
text = Path(sys.argv[1]).read_text()
start = text.index('status_t\npci_reserve_device(')
end = text.index('\n\n\nstatus_t\npci_unreserve_device(', start)
Path(sys.argv[2]).write_text(text[start:end] + '\n')
PY
${CXX:-c++} -std=gnu++20 -O1 -g -Wall -Wextra -Werror -Wno-missing-field-initializers \
 -fsanitize=address,undefined -pthread -I"$temporary" \
 "$directory/pioneer_pci_reservation_race_test.cpp" -o "$temporary/pci-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/pci-test"
awk '/PIONEER_DM_FIND_CHILD_BEGIN/ {copying=1;next} /PIONEER_DM_FIND_CHILD_END/ {found=1;exit} copying {print} END {if(!found)exit 1}' \
 "$source" > "$temporary/pioneer_dm_find_child.inc"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -pthread \
 -I"$temporary" "$directory/pioneer_device_manager_find_child_test.cpp" -o "$temporary/find-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/find-test"
