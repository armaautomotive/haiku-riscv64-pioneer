#!/bin/bash
# One software-only restore after confirmed physical OFF/ON and a fresh
# 64-CPU, full-RAM Haiku boot. No execution or reset of the quarantined image.
set -euo pipefail
umask 077
fail(){ printf 'Q8T4_POSTBOOT_RESTORE_REFUSED %s\n' "$1" >&2; exit 1; }
readonly trusted=/boot/home/pioneer-p100a-hot-reload-20261019-a
readonly admin=$trusted/pioneer-p100a-reload stable=$trusted/tenstorrent_pci_probe-scale-reuse-qualification-041fef18
readonly installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
readonly guard=/boot/home/config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard
readonly dev=/boot/home/config/non-packaged/add-ons/kernel/drivers/dev/misc/tenstorrent_pci_probe
readonly failed=/boot/home/pioneer-q38-two-token-q4-215
readonly repair=/boot/home/pioneer-q8t4-postboot-restore-215
readonly lock=/boot/home/.pioneer-q38-head-csr-exclusive
readonly oldHash=d73dbf803bd05087bd4afb554b841032a8a383ac5495bc49193b4e8870670cd2
readonly stableHash=041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
readonly oldTag=2026111700010215 stableTag=2026102100010012
exact(){
	test -f "$1" && test ! -L "$1" && [ "$(stat -c '%u:%h' "$1")" = 0:1 ] &&
	[ "$(sha256sum "$1" | cut -d ' ' -f1)" = "$2" ]
}
idle(){
	local listing
	listing=$(ps -o Id -o Team) || return 1
	printf '%s\n' "$listing" | awk -v self="$$" '
	$1=="Id" && $2=="Team" && NF==2{header++;next}
	$1 ~ /^[0-9]+$/ && NF>=2 {
	  teams++;if($1==self)next
	  if($0 ~ /(^|[\/[:space:]])(Assistant|HaikuAssistant|llama-|tenstorrent_haiku|inference-|pioneer_q38|pioneer-q|pioneer-native-|pioneer-dram|graph-csr-debug|source-check)/ ||
	    $0 ~ /pioneer-p100a-(dram|batched-output|wide-gdn|token)/)owners++
	  next
	}
	NF{bad++} END{if(header!=1 || !teams || bad || owners)exit 1}'
}
metadata(){
	local observed
	observed=$("$admin" --status) || return 1
	printf '%s\n' "$observed"
	[ "$observed" = "P100A_RELOAD driver55 tag$1 generation$2 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY" ]
}
health(){
	sysinfo | awk '/^64 .*running at/{c++;print} /used\/max [0-9]+ \/ 137034936320\)/{m++;print} END{exit !(c==1 && m==1)}'
}
[ "$(uname -s):$(id -u)" = Haiku:0 ] || fail platform
test -L "$dev" && [ "$(readlink "$dev")" = ../../bin/tenstorrent_pci_probe ] || fail device_link
for dir in "$failed" "$lock"; do
	test -d "$dir" && test ! -L "$dir" && [ "$(stat -c '%u:%a' "$dir")" = 0:700 ] || fail stage_or_lock
done
exact "$failed/state" 86aadc66b39017b2b77524edd86c8ca60a604bfa63cf2bd03d985d27771611b5 || fail failed_state
exact "$failed/compare.exit" 4355a46b19d348dc2f57c046f8ef63d4538ebb936000f3c9ee954a27460dd865 || fail failed_exit
exact "$failed/run-once.sh" de652e39c9de1e7b3814eb7aa4820cd7d340c90835a6e72e80516efba0cc1ab4 || fail failed_runner
exact "$failed/stable041.backup" "$stableHash" || fail trial_backup
exact "$admin" b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f || fail admin
exact "$guard" d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229 || fail guard
exact "$stable" "$stableHash" && exact "$installed" "$oldHash" || fail driver_identity
idle && metadata "$oldTag" 1 && health || fail not_fresh_idle_boot
mkdir "$repair" || fail repair_already_exists
[ "$(stat -c '%d' "$repair")" = "$(stat -c '%d' "$installed")" ] || fail different_filesystem
cp -p "$installed" "$repair/failed-candidate.backup"
cp -p "$stable" "$repair/restore.incoming"
chmod 755 "$repair/restore.incoming"
exact "$repair/failed-candidate.backup" "$oldHash" && exact "$repair/restore.incoming" "$stableHash" || fail staging_hash
idle && metadata "$oldTag" 1 && exact "$installed" "$oldHash" || fail changed_before_quiesce
"$admin" --quiesce "$oldTag" || fail quiesce
exact "$repair/restore.incoming" "$stableHash" && exact "$installed" "$oldHash" || fail changed_after_quiesce
mv -f "$repair/restore.incoming" "$installed"
sync
exact "$installed" "$stableHash" && "$admin" --wait "$stableTag" 0000000000000001 || fail restore_not_verified
idle && metadata "$stableTag" 2 && health || fail postflight
rmdir "$lock" || fail old_lock_retained
printf '%s\n' Q8T4_POSTBOOT_STABLE041_GEN2_RESTORED
