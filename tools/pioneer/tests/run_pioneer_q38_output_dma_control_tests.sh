#!/bin/sh
set -eu
[ "$#" = 1 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-output-dma-control.XXXXXXXX")
source=$directory/../../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp
for section in PROBE_LOCK Q38_DISPATCH_PREPARE Q38_DISPATCH_CONTROL Q38_DISPATCH_RETAIN Q38_DISPATCH_COOKIE_RELEASE; do
 marker=PIONEER_$section
 case "$section" in PROBE_LOCK) name=pioneer_lock;; *) name=$(printf '%s' "$marker" | tr '[:upper:]' '[:lower:]');; esac
 awk -v begin="${marker}_BEGIN" -v end="${marker}_END" '
  index($0,begin) {copying=1;next} index($0,end) {found=1;exit} copying {print} END {if(!found)exit 1}
 ' "$source" > "$temporary/$name.inc"
done
for test in output_dma_control work_run; do
 ${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DTT_PIONEER_EXPERIMENTAL_Q38_DISPATCH -DTT_PIONEER_EXPERIMENTAL_Q38_COHORT_PAYLOAD -I"$temporary" \
 "$directory/pioneer_q38_${test}_test.cpp" -o "$temporary/$test"
 ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/$test" "$1"
done
printf 'Offline actual Work DMA controller evidence retained at %s\n' "$temporary"
