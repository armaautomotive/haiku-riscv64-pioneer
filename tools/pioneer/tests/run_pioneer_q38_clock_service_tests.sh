#!/bin/sh
# Extract the ACTUAL native helper/virtual-view adapter; no hardware access.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
source=$directory/../../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-clock-service.XXXXXXXX")
for section in service io; do
 marker=$(printf 'PIONEER_Q38_CLOCK_%s' "$section" | tr '[:lower:]' '[:upper:]')
 awk -v begin="${marker}_BEGIN" -v end="${marker}_END" '
  index($0,begin) {copying=1;next} index($0,end) {found=1;exit} copying {print} END {if(!found)exit 1}
 ' "$source" > "$temporary/pioneer_q38_clock_$section.inc"
 ${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
  -I"$temporary" "$directory/pioneer_q38_clock_${section}_test.cpp" -o "$temporary/$section"
 UBSAN_OPTIONS=halt_on_error=1 "$temporary/$section"
done
# Verify the REAL native lifecycle call sites, not just the helper bodies.
# This is source-order regression evidence, never physical completion proof.
awk '
 /q38_dispatch_owned_restored:/ {held=NR}
 /FinishQ38NormalClock\(\*context,clean\)/ {finish=NR;finish_count++}
 /PioneerQ38DispatchMmioEnd\(context->io/ {pci=NR}
 /if \(ready && bankOnly\).*banks.Bind/ {bank=NR}
 /if \(ready && bankOnly\).*BeginQ38NormalClock/ {begin=NR;begin_count++}
 /if \(ready\).*PioneerPrepareHostPage/ {host=NR}
 /if \(q38Clock.Unsafe\(\)\) return true/ {unsafe++}
 /c.q38Clock.Quarantine\(\)/ {retain++}
 /cookie->context->q38Clock.RequiresExplicitEnd\(\)/ {close_gate++}
 END {if (!(held && finish>held && pci>finish && bank && begin>bank && host>begin && begin_count==1 && finish_count==1 && unsafe==1 && retain==1 && close_gate>=1)) exit 1}
' "$source"
printf 'Actual native-clock helper evidence retained at %s\n' "$temporary"
