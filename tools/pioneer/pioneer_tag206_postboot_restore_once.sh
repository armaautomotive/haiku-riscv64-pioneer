#!/bin/sh
# Post-cold-boot recovery only: no card execution and no retry/reset.
set -eu
fail() { echo "RESTORE_BLOCKED: $*" >&2; exit 1; }
base=/boot/home/pioneer-p100a-hot-reload-20261019-a
admin=$base/pioneer-p100a-reload
stable=$base/tenstorrent_pci_probe-scale-reuse-qualification-041fef18
installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
guard=/boot/home/config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard
stage=/boot/home/pioneer-tag206-postboot-restore-20261117-a
candidate=1c2ad62853c4617690956e083da8c364d27c5f1399fdc9509f8c6f781684e475
known=041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
exact() { [ -f "$1" ] && [ ! -L "$1" ] && [ "$(sha256sum "$1" | cut -d ' ' -f1)" = "$2" ]; }
idle() {
 listing=$(ps -o Id -o Team) || return 1
 printf '%s\n' "$listing" | awk -v self="$$" '
 $1=="Id" && $2=="Team" && NF==2 {header++;next}
 $1 ~ /^[0-9]+$/ && NF>=2 {teams++;if($1==self)next
 if($0 ~ /(^|[\/[[:space:]])(Assistant|HaikuAssistant|llama-|tenstorrent_haiku|inference-|pioneer_q38|pioneer-q|pioneer-native-|pioneer-dram|graph-csr-debug|source-check)/ || $0 ~ /pioneer-p100a-(dram|batched-output|wide-gdn|token)/)owners++;next}
 NF{bad++} END{if(header!=1 || !teams || bad || owners)exit 1}'
}
status() { [ "$("$admin" --status)" = "P100A_RELOAD driver55 tag$1 generation$2 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY" ]; }
[ "$(uname -s):$(id -u)" = Haiku:0 ] || fail platform
exact "$admin" b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f || fail admin
exact "$guard" d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229 || fail guard
exact "$installed" "$candidate" && exact "$stable" "$known" || fail driver_identity
device=/boot/home/config/non-packaged/add-ons/kernel/drivers/dev/misc/tenstorrent_pci_probe
[ -L "$device" ] && [ "$(readlink "$device")" = ../../bin/tenstorrent_pci_probe ] || fail device_link
health=$(sysinfo)
printf '%s\n' "$health" | awk '/^64 .*running at/{c++} /used\/max [0-9]+ \/ 137034936320\)/{m++} END{exit !(c==1 && m==1)}' || fail health
idle && status 2026111500010206 1 || fail busy_or_unexpected_state
[ ! -e "$stage" ] && [ ! -L "$stage" ] || fail stage_exists
umask 077
mkdir "$stage"
[ "$(stat -c '%d' "$stage")" = "$(stat -c '%d' "$installed")" ] || fail different_filesystem
cp -p "$installed" "$stage/candidate.backup"
cp -p "$stable" "$stage/restore.incoming"
chmod 755 "$stage/restore.incoming"
exact "$stage/candidate.backup" "$candidate" && exact "$stage/restore.incoming" "$known" || fail copy
idle && status 2026111500010206 1 && exact "$installed" "$candidate" || fail changed
"$admin" --quiesce 2026111500010206 || fail quiesce
exact "$stage/restore.incoming" "$known" && exact "$installed" "$candidate" || fail post_quiesce_identity
mv -f "$stage/restore.incoming" "$installed"
sync
exact "$installed" "$known" || fail readback
"$admin" --wait 2026102100010012 0000000000000001 || fail wait
status 2026102100010012 2 && idle || fail postflight
echo TAG206_POSTBOOT_STABLE041_GENERATION2_RESTORED
