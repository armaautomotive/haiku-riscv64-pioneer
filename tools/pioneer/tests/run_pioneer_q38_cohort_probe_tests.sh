#!/bin/sh
set -eu
[ "$#" = 0 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-cohort-policy.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_q38_cohort_probe_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test"
source=$directory/../../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp
for section in PROBE_LOCK Q38_COHORT_PREPARE Q38_COHORT_CONTROL Q38_PANEL_CONTROL Q38_STREAM_CONTROL; do
 marker=PIONEER_$section
 case "$section" in PROBE_LOCK) name=pioneer_lock;; *) name=$(printf '%s' "$marker" | tr '[:upper:]' '[:lower:]');; esac
 awk -v begin="${marker}_BEGIN" -v end="${marker}_END" '
  index($0,begin) {copying=1;next} index($0,end) {found=1;exit} copying {print} END {if(!found)exit 1}
 ' "$source" > "$temporary/$name.inc"
done
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 -I"$temporary" "$directory/pioneer_q38_cohort_control_test.cpp" -o "$temporary/control"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/control"
for sourceName in probe control; do
 ${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
  -DTT_PIONEER_EXPERIMENTAL_Q38_COHORT_SEALED -I"$temporary" "$directory/pioneer_q38_cohort_${sourceName}_test.cpp" -o "$temporary/sealed-$sourceName"
 ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/sealed-$sourceName"
done
for sourceName in probe control; do
 ${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
  -DTT_PIONEER_EXPERIMENTAL_Q38_COHORT_SEALED -DTT_PIONEER_EXPERIMENTAL_Q38_COHORT_PAYLOAD \
  -I"$temporary" "$directory/pioneer_q38_cohort_${sourceName}_test.cpp" -o "$temporary/payload-$sourceName"
 ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/payload-$sourceName"
done
printf 'Offline cohort policy evidence retained at %s\n' "$temporary"
