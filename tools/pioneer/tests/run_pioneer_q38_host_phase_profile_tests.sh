#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-host-phase-profile.XXXXXXXX")
source=$directory/../../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp
for section in ADD REPORT PUBLICATION;do
 lower=$(printf '%s' "$section" | tr '[:upper:]' '[:lower:]')
 awk -v begin="PIONEER_Q38_HOST_PHASE_${section}_BEGIN" -v end="PIONEER_Q38_HOST_PHASE_${section}_END" '
 index($0,begin){copying=1;next}index($0,end){found=1;exit}copying{print}END{if(!found)exit 1}' \
 "$source" > "$temporary/host_phase_$lower.inc"
done
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 -I"$temporary" "$directory/pioneer_q38_host_phase_profile_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test"
python3 "$directory/pioneer_q38_host_phase_build_gate_test.py"
printf 'Offline host phase diagnostics evidence retained at %s\n' "$temporary"
