#!/bin/bash
# Tag220 only: AFTER verified OFF/ON and fresh 64-CPU SD-free Haiku.
# Do not use on the previous boot; never submit work to the failed driver.
set -euo pipefail
umask 077
stage=/boot/home/pioneer-q38-twin-q8-220
repair=/boot/home/pioneer-q38-twin-q8-220-recovery
lock=/boot/home/.pioneer-q38-head-csr-exclusive
installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
device=/boot/home/config/non-packaged/add-ons/kernel/drivers/dev/misc/tenstorrent_pci_probe
trusted=/boot/home/pioneer-p100a-hot-reload-20261019-a
admin=$trusted/pioneer-p100a-reload
stable=$trusted/tenstorrent_pci_probe-scale-reuse-qualification-041fef18
guard=/boot/home/config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard
readonly stableTag=2026102100010012 failedTag=2026111700010220
readonly stableHash=041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
readonly failedHash=8bfd096f35df5b1dd2c96f368e774cd1631b714cff256b7c46e345a8de6bce6a
fail(){ printf 'TAG220_RECOVERY_REFUSED %s\n' "$1" >&2; exit 1; }
absent(){ test ! -e "$1" && test ! -L "$1"; }
exact(){
	test -f "$1" && test ! -L "$1" && [ "$(stat -c '%u:%h' "$1")" = 0:1 ] &&
	printf '%s  %s\n' "$2" "$1" | sha256sum -c -
}
idle(){
	local listing
	listing=$(ps -o Id -o Team) || return 1
	printf '%s\n' "$listing" | awk -v self="$$" '
	$1=="Id" && $2=="Team" && NF==2{header++;next}
	$1~/^[0-9]+$/ && NF>=2{
	  t++;if($1==self)next
	  if($0~/(^|[\/[:space:]])(Assistant|HaikuAssistant|llama-|tenstorrent_haiku|inference-|pioneer_q38|pioneer-q|pioneer-native-|pioneer-dram|graph-csr-debug|source-check)/ ||
	   $0~/pioneer-p100a-(dram|batched-output|wide-gdn|token)/)bad++
	  next
	}
	NF{bad++} END{if(header!=1 || !t || bad)exit 1}'
}
metadata(){
	local value
	value=$("$admin" --status) || return 1
	printf '%s\n' "$value"
	printf '%s\n' "$value" | /bin/grep -Eq "^P100A_RELOAD driver[1-9][0-9]* tag$1 generation$2 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY$"
}
health(){ sysinfo | awk '/^64 .*running at/{c++;print} /used\/max [0-9]+ \/ 137034936320\)/{m++;print} END{exit !(c==1 && m==1)}'; }
[ "$#" = 1 ] && [ "$(uname -s):$(id -u)" = Haiku:0 ] || exit 2
case "$1" in --preflight|--restore|--postflight) ;; *) exit 2;; esac
test -L "$device" && [ "$(readlink "$device")" = ../../bin/tenstorrent_pci_probe ] || fail device_link
exact "$admin" b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f &&
exact "$guard" d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229 &&
exact "$stable" "$stableHash" || fail trusted_files
if [ "$1" = --postflight ]; then
	absent "$lock" && absent "$repair/restore.incoming" &&
	exact "$installed" "$stableHash" && exact "$repair/failed-candidate.backup" "$failedHash" &&
	idle && metadata "$stableTag" 2 && health || fail postflight
	printf '%s\n' TAG220_RECOVERY_POSTFLIGHT_IDLE_STABLE041
	exit 0
fi
for dir in "$stage" "$lock"; do
	test -d "$dir" && test ! -L "$dir" && [ "$(stat -c '%u:%a' "$dir")" = 0:700 ] || fail stage_or_lock
done
test -z "$(ls -A "$lock")" && absent "$repair" || fail spent_or_other_trial
# The verified pre-power logs were saved separately. These six digests
# pin the differing files observed and archived after BFS recovery.
exact "$stage/state" 57e9f72e7f3a4324f81b8175691cb3b207dcd848bd2dc19d4bf6c4072f75e81b &&
exact "$stage/launch.pid" 8d631dbc1a2e1f561cc47c1055ec9f4e6cc3224a9c643cedab6e6497b8439d46 &&
exact "$stage/run-once.sh" 043d4f2f848a47431a12bbec52bda72609008015d86ecbf085ccf23108c244ce &&
exact "$stage/manifest.json" 6ba3e08c4d88ac2950dd500433828c121b20037bd8f3f103318f4132de947e32 &&
exact "$stage/compare.stderr" 177e516bf18cb3a3c84b728c65db9e8471ca2a62133e9737b9aef87d03944fd3 &&
exact "$stage/compare.exit" 77b40b137c5627ea9233b2250b996d428e92e17b11030c20d024a76737a52389 &&
exact "$stage/parent.exit" 5b716509bf786d114b77d4a7460d3fc13bf22b67c25b0993a72ae5ee5e1f65f3 &&
exact "$stage/parent.stdout" 90d2b8a809c0ff133c85702bc4c34e907f8b550cd21d147073e5638df6e6d299 &&
exact "$stage/parent.stderr" 4ef5a8efb06cc5ae17642f2ad6cf139de9024dbd1fdb87753c2d67b074a3ddec &&
exact "$stage/stable041.backup" "$stableHash" && exact "$installed" "$failedHash" || fail evidence_or_candidate
idle && metadata "$failedTag" 1 && health || fail not_fresh_idle_boot
if [ "$1" = --preflight ]; then printf '%s\n' TAG220_RECOVERY_PREFLIGHT; exit 0; fi
mkdir "$repair" || fail lock
[ "$(stat -c '%d' "$repair")" = "$(stat -c '%d' "$installed")" ] || fail different_volume
cp -p "$installed" "$repair/failed-candidate.backup"
cp -p "$stable" "$repair/restore.incoming"
chmod 755 "$repair/restore.incoming"
exact "$repair/failed-candidate.backup" "$failedHash" && exact "$repair/restore.incoming" "$stableHash" || fail staged_hash
idle && metadata "$failedTag" 1 && exact "$installed" "$failedHash" || fail changed_before_quiesce
"$admin" --quiesce "$failedTag" || fail quiesce
exact "$repair/restore.incoming" "$stableHash" && exact "$installed" "$failedHash" || fail changed_after_quiesce
mv -f "$repair/restore.incoming" "$installed" && sync || fail replace
exact "$installed" "$stableHash" && "$admin" --wait "$stableTag" 0000000000000001 || fail restore_wait
idle && metadata "$stableTag" 2 && health || fail restore_health
rmdir "$lock" || fail old_lock_retained
printf '%s\n' TAG220_RECOVERY_STABLE041_GENERATION2
