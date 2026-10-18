#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-panel-policy.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_q38_panel_probe_test.cpp" -o "$temporary/test"
UBSAN_OPTIONS=halt_on_error=1 "$temporary/test"
source=$directory/../../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp
awk '/PIONEER_PROBE_LOCK_BEGIN/ {copying=1;next} /PIONEER_PROBE_LOCK_END/ {found=1;exit} copying {print} END {if(!found)exit 1}' "$source" > "$temporary/pioneer_lock.inc"
awk '/PIONEER_Q38_PANEL_CONTROL_BEGIN/ {copying=1;next} /PIONEER_Q38_PANEL_CONTROL_END/ {found=1;exit} copying {print} END {if(!found)exit 1}' "$source" > "$temporary/pioneer_q38_panel_control.inc"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 -I"$temporary" "$directory/pioneer_q38_panel_control_test.cpp" -o "$temporary/control"
UBSAN_OPTIONS=halt_on_error=1 "$temporary/control"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DTT_PIONEER_EXPERIMENTAL_Q38_AFFINE "$directory/pioneer_q38_panel_probe_test.cpp" -o "$temporary/affine"
UBSAN_OPTIONS=halt_on_error=1 "$temporary/affine"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DTT_PIONEER_EXPERIMENTAL_Q38_AFFINE -I"$temporary" "$directory/pioneer_q38_panel_control_test.cpp" -o "$temporary/affine-control"
UBSAN_OPTIONS=halt_on_error=1 "$temporary/affine-control"
printf 'Offline local-panel policy evidence retained at %s\n' "$temporary"
