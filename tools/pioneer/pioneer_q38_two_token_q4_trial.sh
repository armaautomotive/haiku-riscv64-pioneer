#!/bin/bash
# One-shot Q8T4 timing on a verified stable041 generation10 Haiku session.
# Back up before exchange. Never retry or swap after uncertain ownership.
set -euo pipefail
umask 077
readonly stage=/boot/home/pioneer-q38-two-token-q4-215
readonly lock=/boot/home/.pioneer-q38-head-csr-exclusive
readonly installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
readonly dev=/boot/home/config/non-packaged/add-ons/kernel/drivers/dev/misc/tenstorrent_pci_probe
readonly trusted=/boot/home/pioneer-p100a-hot-reload-20261019-a
readonly admin=$trusted/pioneer-p100a-reload
readonly stable=$trusted/tenstorrent_pci_probe-scale-reuse-qualification-041fef18
readonly guard=/boot/home/config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard
readonly candidate=$stage/tenstorrent_pci_probe-native-q8t4
readonly client=$stage/pioneer-q38-two-token-q4-compare
readonly model=/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf
readonly stableTag=2026102100010012 newTag=2026111700010215
readonly start=10 trial=11 end=12
readonly stableHash=041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
readonly newHash=d73dbf803bd05087bd4afb554b841032a8a383ac5495bc49193b4e8870670cd2
readonly clientHash=a902f0b5fc5521648127869e0efa48a56a808294d9dbda117792992439149843

fail(){ printf 'Q8T4_REFUSED %s\n' "$1" >&2; exit 1; }
absent(){ test ! -e "$1" && test ! -L "$1"; }
exact(){
	local digest
	test -f "$1" && test ! -L "$1" || return 1
	case "$(stat -c '%u:%h:%a' "$1")" in 0:1:700|0:1:755|0:1:555|0:1:600) ;; *) return 1;; esac
	digest=$(sha256sum "$1") || return 1
	[ "$(printf '%s\n' "$digest" | awk '{print $1}')" = "$2" ]
}
metadata(){
	local observed
	observed=$("$admin" --status) || return 1
	printf '%s\n' "$observed"
	[ "$observed" = "P100A_RELOAD driver55 tag$1 generation$2 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY" ]
}
idle(){
	local listing
	listing=$(ps -o Id -o Team) || return 1
	# Do not run ps alongside the awk process bearing the owner patterns.
	printf '%s\n' "$listing" | awk -v self="$$" '
	$1=="Id" && $2=="Team" && NF==2 {header++;next}
	$1 ~ /^[0-9]+$/ && NF>=2 {
	  teams++;if($1==self)next
	  if($0 ~ /(^|[\/[:space:]])(Assistant|HaikuAssistant|llama-|tenstorrent_haiku|inference-|pioneer_q38|pioneer-q|pioneer-native-|pioneer-dram|graph-csr-debug|source-check)/ ||
	     $0 ~ /pioneer-p100a-(dram|batched-output|wide-gdn|token)/) {
	    printf "Q8T4_OWNER team%s executable%s\n",$1,$2 > "/dev/stderr";owners++
	  }
	  next
	}
	NF{bad++} END{if(header!=1 || !teams || bad || owners)exit 1}'
}
health(){
	sysinfo | awk '
	/^64 .*running at/{cpus++;print}
	/used\/max [0-9]+ \/ 137034936320\)/{ram++;print}
	END{if(cpus!=1 || ram!=1)exit 1}'
}
paths(){
	test -d "$stage" && test ! -L "$stage" && [ "$(stat -c '%u:%a' "$stage")" = 0:700 ]
}
inputs(){
	paths || fail stage
	[ "$(stat -c '%d' "$stage")" = "$(stat -c '%d' "$installed")" ] || fail driver_volume
	absent "$lock" || fail already_locked
	for file in .trial-once stable041.backup candidate.incoming restore.incoming state compare.stdout compare.stderr compare.exit; do
		absent "$stage/$file" || fail "spent:$file"
	done
	test -L "$dev" && [ "$(readlink "$dev")" = ../../bin/tenstorrent_pci_probe ] || fail device_link
	exact "$admin" b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f || fail admin_hash
	exact "$guard" d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229 || fail guard_hash
	exact "$installed" "$stableHash" && exact "$stable" "$stableHash" || fail stable_hash
	exact "$candidate" "$newHash" && exact "$client" "$clientHash" || fail trial_hash
	test -f "$model" && test ! -L "$model" && [ "$(stat -c '%u:%h:%a:%s' "$model")" = 0:1:600:18973870528 ] || fail model_identity
	test -f /boot/system/lib/libstdc++.so.6 || fail client_runtime
	idle && metadata "$stableTag" "$start" && health || fail not_idle_stable041
}
mark(){ printf '%s\n' "$1" > "$stage/state"; }
restore(){
	paths && exact "$installed" "$newHash" && idle && metadata "$newTag" "$trial" || return 1
	absent "$stage/restore.incoming" || return 1
	cp -p "$stage/stable041.backup" "$stage/restore.incoming" || return 1
	chmod 755 "$stage/restore.incoming" || return 1
	exact "$stage/restore.incoming" "$stableHash" && exact "$installed" "$newHash" && idle || return 1
	"$admin" --quiesce "$newTag" || return 1
	exact "$stage/restore.incoming" "$stableHash" && exact "$installed" "$newHash" || return 1
	mv -f "$stage/restore.incoming" "$installed" && sync || return 1
	exact "$installed" "$stableHash" && "$admin" --wait "$stableTag" "$(printf '%016x' "$trial")" || return 1
	metadata "$stableTag" "$end" && idle && health || return 1
	mark STABLE041_GEN12_RESTORED
}
cleanup(){
	local rc=$?
	trap - EXIT HUP INT TERM
	if [ "$exchange" = 1 ] && ! restore; then
		mark RESTORE_NOT_CONFIRMED_DO_NOT_RETRY_OR_RESET
		printf '%s\n' Q8T4_RESTORE_NOT_CONFIRMED >&2
		exit 1
	fi
	rmdir "$lock" || fail lock_release
	exit "$rc"
}

[ "$#" = 1 ] || exit 2
case "$1" in --preflight|--run) ;; *) exit 2;; esac
[ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ] || exit 2
inputs
if [ "$1" = --preflight ]; then
	printf '%s\n' Q8T4_PREFLIGHT_VERIFIED_NO_DRIVER_CHANGE
	exit 0
fi
mkdir "$lock" || fail locked
exchange=0
trap cleanup EXIT
trap 'exit 1' HUP INT TERM
mkdir "$stage/.trial-once" || fail one_shot
mark STAGING_Q8T4
cp -p "$stable" "$stage/stable041.backup"
cp -p "$candidate" "$stage/candidate.incoming"
chmod 755 "$stage/candidate.incoming"
exact "$stage/stable041.backup" "$stableHash" && exact "$stage/candidate.incoming" "$newHash" || fail stage_hash
exact "$installed" "$stableHash" && idle && metadata "$stableTag" "$start" || fail before_exchange
exchange=1
"$admin" --quiesce "$stableTag" || fail stable_quiesce
exact "$stage/candidate.incoming" "$newHash" && exact "$installed" "$stableHash" || fail pre_replace_hash
mv -f "$stage/candidate.incoming" "$installed" && sync || fail candidate_replace
exact "$installed" "$newHash" && "$admin" --wait "$newTag" "$(printf '%016x' "$start")" || fail candidate_wait
idle && metadata "$newTag" "$trial" || fail candidate_generation
mark RUNNING_Q8T4_DIAGNOSTIC
if LIBRARY_PATH=/boot/system/lib "$client" --exercise "$trial" "$model" > "$stage/compare.stdout" 2> "$stage/compare.stderr"; then
	result=0
else
	result=$?
fi
printf '%s\n' "$result" > "$stage/compare.exit"
[ "$result" = 0 ] || fail compare_client
test ! -s "$stage/compare.stderr" || fail nonempty_client_stderr
awk '
NR==1 && /^Q8T4_CLOSED K17408 output32 exact32 bank0 GDDRreads68\/68\/68 wall_us[1-9][0-9]*\/[1-9][0-9]*\/[1-9][0-9]*;SINGLE_SHARD_NOT_TOKEN_RATE$/ {exact++}
END{if(NR!=1 || exact!=1)exit 1}' "$stage/compare.stdout" || fail no_verified_timing
printf '%s\n' Q8T4_COMPARE_CLOSED
