#!/bin/bash
# One-shot tag206 authentication/cache/capability readiness test; no Generate.
set -eu
[ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ] || exit 2
stage=/boot/home/pioneer-q38-readiness-tag206-20261116-b
installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
candidateSource=/boot/home/pioneer-q38-gdn-l1-tag206-20261116-a/tenstorrent_pci_probe-gdn-l1-206
stableSource=/boot/home/pioneer-p100a-hot-reload-20261019-a/tenstorrent_pci_probe-scale-reuse-qualification-041fef18
admin=/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload
worker=$stage/readiness-diagnostic-tag206
candidateHash=1c2ad62853c4617690956e083da8c364d27c5f1399fdc9509f8c6f781684e475
workerHash=92f39ed2633499fc286f3bcd398e437c6cc2ff9cacd5f16cf298dfb4972c153b
stableHash=041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
adminHash=b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f
guardHash=d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229
exact(){
	test -f "$1" && test ! -L "$1" || return 1
	digest=$(sha256sum "$1") || return 1
	[ "${digest%% *}" = "$2" ]
}
idle(){
	listing=$(ps -o Id -o Team) || return 1
	printf '%s\n' "$listing" | awk -v self="$$" '
	$1=="Id" && $2=="Team" && NF==2 {header++;next}
	$1 ~ /^[0-9]+$/ && NF>=2 {
	 teams++;if($1==self)next
	 if($0 ~ /Assistant|llama-|tenstorrent_haiku|inference-|pioneer_q38|pioneer-q|pioneer-native-|pioneer-dram|graph-csr-debug/)owners++
	 next
	}
	NF{bad++} END{if(header!=1 || !teams || bad || owners)exit 1}'
}
metadata(){
	value=$("$admin" --status) || return 1
	printf '%s\n' "$value"
	[ "$value" = "P100A_RELOAD driver55 tag$1 generation$2 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY" ]
}
test -d "$stage" && test ! -L "$stage" || exit 1
[ "$(stat -c '%u:%a' "$stage")" = 0:700 ] || exit 1
test ! -e "$stage/.trial-once" && test ! -L "$stage/.trial-once" || exit 1
exact "$candidateSource" "$candidateHash" && exact "$stableSource" "$stableHash" || exit 1
exact "$admin" "$adminHash" && exact /boot/home/config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard "$guardHash" || exit 1
exact "$installed" "$stableHash" && exact "$worker" "$workerHash" || exit 1
idle && metadata 2026102100010012 6 || exit 1
test ! -e "$stage/stable.backup" && test ! -e "$stage/candidate.incoming" && test ! -e "$stage/restore.incoming" || exit 1
cp -p "$stableSource" "$stage/stable.backup"
cp -p "$candidateSource" "$stage/candidate.incoming"
chmod 755 "$stage/candidate.incoming"
exact "$stage/stable.backup" "$stableHash" && exact "$stage/candidate.incoming" "$candidateHash" || exit 1
mkdir "$stage/.trial-once"
printf '%s\n' TAG206_READY_ONLY_TRIAL_STARTED > "$stage/trial.state"

restore(){
	[ -f "$stage/candidate-active" ] || return 0
	metadata 2026111500010206 7 >/dev/null || return 1
	idle || return 1
	cp -p "$stage/stable.backup" "$stage/restore.incoming" || return 1
	chmod 755 "$stage/restore.incoming" || return 1
	exact "$stage/restore.incoming" "$stableHash" && exact "$installed" "$candidateHash" && idle || return 1
	"$admin" --quiesce 2026111500010206 || return 1
	exact "$stage/restore.incoming" "$stableHash" && exact "$installed" "$candidateHash" || return 1
	mv -f "$stage/restore.incoming" "$installed" || return 1
	sync || return 1
	rm -f "$stage/candidate-active"
	exact "$installed" "$stableHash" || return 1
	"$admin" --wait 2026102100010012 0000000000000007 || return 1
	metadata 2026102100010012 8 && idle || return 1
	printf '%s\n' TAG206_READY_ONLY_STABLE041_GEN8_RESTORED > "$stage/trial.state"
}
trap 'restore || echo TAG206_READY_ONLY_RESTORE_REQUIRED >&2' EXIT HUP INT TERM

"$admin" --quiesce 2026102100010012 || exit 1
exact "$stage/candidate.incoming" "$candidateHash" && exact "$installed" "$stableHash" || exit 1
mv -f "$stage/candidate.incoming" "$installed" || exit 1
sync || exit 1
exact "$installed" "$candidateHash" || exit 1
touch "$stage/candidate-active"
"$admin" --wait 2026111500010206 0000000000000006 || exit 1
metadata 2026111500010206 7 && idle || exit 1
set +e
LIBRARY_PATH=/boot/home/develop/llama-c060ca974c77/build-pioneer/bin:/boot/system/lib \
	"$worker" --readiness-diagnostic > "$stage/readiness.stdout" 2> "$stage/readiness.stderr"
workerStatus=$?
set -e
printf '%s\n' "$workerStatus" > "$stage/readiness.exit"
cat "$stage/readiness.stdout"
cat "$stage/readiness.stderr"
restore
trap - EXIT HUP INT TERM
echo TAG206_READY_ONLY_STABLE041_GEN8_RESTORED
exit 0
