#!/bin/bash
# Exact tag220 one-shot. Current admitted stable generation is 4.
set -euo pipefail
umask 077
stage=/boot/home/pioneer-q38-twin-q8-220
lock=/boot/home/.pioneer-q38-head-csr-exclusive
installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
device=/boot/home/config/non-packaged/add-ons/kernel/drivers/dev/misc/tenstorrent_pci_probe
trusted=/boot/home/pioneer-p100a-hot-reload-20261019-a
admin=$trusted/pioneer-p100a-reload
stable=$trusted/tenstorrent_pci_probe-scale-reuse-qualification-041fef18
guard=/boot/home/config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard
candidate=$stage/tenstorrent_pci_probe-twin-q8-220
client=$stage/pioneer-q38-twin-q8-compare
readonly stableTag=2026102100010012 candidateTag=2026111700010220
readonly stableHash=041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
readonly candidateHash=8bfd096f35df5b1dd2c96f368e774cd1631b714cff256b7c46e345a8de6bce6a
readonly clientHash=4140e029a0d21207580cca9ea15a9a01465b26c293f937d79ebe55d5742ccbda
readonly manifestHash=6ba3e08c4d88ac2950dd500433828c121b20037bd8f3f103318f4132de947e32

fail(){ printf 'TAG220_REFUSED %s\n' "$1" >&2; exit 1; }
absent(){ test ! -e "$1" && test ! -L "$1"; }
exact(){
	local digest
	test -f "$1" && test ! -L "$1" || return 1
	case "$(stat -c '%u:%h:%a' "$1")" in 0:1:700|0:1:755|0:1:555|0:1:600) ;; *) return 1;; esac
	digest=$(sha256sum "$1") || return 1
	[ "${digest%% *}" = "$2" ]
}
paths(){ test -d "$stage" && test ! -L "$stage" && [ "$(stat -c '%u:%a' "$stage")" = 0:700 ]; }
metadata(){
	local value
	value=$("$admin" --status) || return 1
	printf '%s\n' "$value"
	printf '%s\n' "$value" | /bin/grep -Eq "^P100A_RELOAD driver[1-9][0-9]* tag$1 generation$2 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY$"
}
idle(){
	local listing
	listing=$(ps -o Id -o Team) || return 1
	printf '%s\n' "$listing" | awk -v self="$$" -v parent="${1:-0}" '
	$1=="Id" && $2=="Team" && NF==2 {header++;next}
	$1~/^[0-9]+$/ && NF>=2 {
	  teams++;if($1==self)next
	  if($1==parent || $0~/(^|[\/[:space:]])(Assistant|HaikuAssistant|llama-|tenstorrent_haiku|inference-|pioneer_q38|pioneer-q|pioneer-native-|pioneer-dram|graph-csr-debug|source-check)/ ||
	   $0~/pioneer-p100a-(dram|batched-output|wide-gdn|token)/)owners++
	  next
	}
	NF{bad++} END{if(header!=1 || !teams || bad || owners)exit 1}'
}
health(){
	sysinfo | awk '/^64 .*running at/{cpus++;print}
	  /used\/max [0-9]+ \/ 137034936320\)/{ram++;print}
	  END{if(cpus!=1 || ram!=1)exit 1}'
}
package(){
	test -L "$device" && [ "$(readlink "$device")" = ../../bin/tenstorrent_pci_probe ] &&
	exact "$admin" b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f &&
	exact "$guard" d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229 &&
	exact "$stable" "$stableHash" && exact "$candidate" "$candidateHash" &&
	exact "$client" "$clientHash" && exact "$stage/manifest.json" "$manifestHash"
}
receipt(){
	test ! -s "$stage/compare.stderr" || return 1
	awk '
	BEGIN{d[0]="0e94ceba949d9266e0383ca43b96c73da4bcaf0785fec9d45b4570f73e021254"
	      d[1]="42f5a264959edb0b327fc337a227ae2f4bcb55bc74a637cbde09087b057ebfd2"}
	NR<=4 {
	 v=int((NR-1)/2);a=1+(NR-1)%2
	 if(NF!=7 || $1!="Q8_PAIR_CLOSED" || $2!="arm=" a || $3!="variant=" v ||
	  $4!="pages=68" || $5!~/^polls=[1-9][0-9]*$/ || $6!~/^matrix_wait_us=[1-9][0-9]*$/ || $7!="output_sha=" d[v]) exit 1
	 next
	}
	NR==5 && $0=="PASS_Q8_PAIR_FOUR_CLOSED_MATCHING_PINNED_ARMS;NO_TOKENS_PER_SECOND"{pass=1}
	END{if(NR!=5 || !pass)exit 1}' "$stage/compare.stdout"
}
inputs(){
	paths || fail stage
	[ "$(stat -c '%d' "$stage")" = "$(stat -c '%d' "$installed")" ] || fail driver_volume
	absent "$lock" || fail locked
	for name in .trial-once stable041.backup candidate.incoming restore.incoming state compare.stdout compare.stderr compare.exit parent.exit launch.pid; do
		absent "$stage/$name" || fail "spent:$name"
	done
	package && exact "$installed" "$stableHash" || fail package
	test -x "$client" && test -f /boot/system/lib/libstdc++.so.6 || fail client_runtime
	idle && metadata "$stableTag" 4 && health || fail idle_or_generation
}
mark(){ printf '%s\n' "$1" > "$stage/state"; }
restore(){
	paths && exact "$installed" "$candidateHash" && idle && metadata "$candidateTag" 5 || return 1
	absent "$stage/restore.incoming" || return 1
	exact "$stage/stable041.backup" "$stableHash" || return 1
	cp -p "$stage/stable041.backup" "$stage/restore.incoming" || return 1
	chmod 755 "$stage/restore.incoming" || return 1
	exact "$stage/restore.incoming" "$stableHash" && exact "$installed" "$candidateHash" && idle || return 1
	"$admin" --quiesce "$candidateTag" || return 1
	exact "$stage/restore.incoming" "$stableHash" && exact "$installed" "$candidateHash" || return 1
	mv -f "$stage/restore.incoming" "$installed" && sync || return 1
	exact "$installed" "$stableHash" && "$admin" --wait "$stableTag" 0000000000000005 || return 1
	idle && metadata "$stableTag" 6 && health || return 1
	mark STABLE041_GEN6_RESTORED
}
cleanup(){
	local rc=$?
	trap - EXIT HUP INT TERM
	if [ "$exchange" = 1 ] && ! restore; then
		mark RESTORE_NOT_CONFIRMED_DO_NOT_RETRY_OR_RESET
		printf '%s\n' TAG220_RESTORE_NOT_CONFIRMED >&2
		rc=1
	elif ! rmdir "$lock"; then
		rc=1
	fi
	( set -C; printf '%s\n' "$rc" > "$stage/parent.exit" )
	exit "$rc"
}
postflight(){
	local parent=$1 name value
	local evidence=(state parent.exit parent.stdout parent.stderr run-once.sh launch.pid)
	[[ "$parent" =~ ^[1-9][0-9]{0,8}$ ]] || fail parent
	paths && package || fail package
	for name in "${evidence[@]}"; do
		test -f "$stage/$name" && test ! -L "$stage/$name" || fail "evidence:$name"
	done
	for name in compare.exit compare.stdout compare.stderr; do
		if ! absent "$stage/$name"; then
			test -f "$stage/$name" && test ! -L "$stage/$name" || fail "evidence:$name"
			evidence+=("$name")
		fi
	done
	printf '%s\n' "$parent" | cmp -s "$stage/launch.pid" - || fail parent_identity
	idle "$parent" || fail team_still_live
	value=$(< "$stage/state")
	case "$value" in
	STABLE041_GEN6_RESTORED)
		absent "$lock" && absent "$stage/restore.incoming" && absent "$stage/candidate.incoming" &&
		exact "$installed" "$stableHash" && exact "$stage/stable041.backup" "$stableHash" &&
		metadata "$stableTag" 6 && health || fail stable_postflight
		if [ "${#evidence[@]}" = 9 ] && printf '0\n' | cmp -s "$stage/compare.exit" - &&
			printf '0\n' | cmp -s "$stage/parent.exit" - && test ! -s "$stage/parent.stderr" && receipt; then
			printf '%s\n' TAG220_VERIFIED_FOUR_ARMS_AND_STABLE_RESTORED
		else
			printf '%s\n' TAG220_CLIENT_FAILED_BUT_STABLE_RESTORED
		fi
		;;
	RESTORE_NOT_CONFIRMED_DO_NOT_RETRY_OR_RESET)
		test -d "$lock" && test ! -L "$lock" || fail lost_quarantine_lock
		# File hashes only here. Do not open the driver after uncertainty.
		sha256sum "$installed"
		printf '%s\n' TAG220_RESTORE_UNCONFIRMED_LOCK_RETAINED_NO_DEVICE_OPEN
		;;
	*) fail unexpected_state;;
	esac
	(cd "$stage" && sha256sum "${evidence[@]}")
}

[ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ] || exit 2
if [ "$#" = 2 ] && [ "$1" = --postflight ]; then postflight "$2"; exit; fi
[ "$#" = 1 ] || exit 2
case "$1" in --preflight|--run) ;; *) exit 2;; esac
inputs
if [ "$1" = --preflight ]; then printf '%s\n' TAG220_PREFLIGHT_NO_DRIVER_CHANGE; exit; fi
mkdir "$lock" || fail locked
exchange=0
trap cleanup EXIT
trap 'exit 1' HUP INT TERM
mkdir "$stage/.trial-once" || fail one_shot
( set -C; printf '%s\n' "$$" > "$stage/launch.pid" )
mark STAGING_TAG220
cp -p "$stable" "$stage/stable041.backup"
cp -p "$candidate" "$stage/candidate.incoming"
chmod 755 "$stage/candidate.incoming"
exact "$stage/stable041.backup" "$stableHash" && exact "$stage/candidate.incoming" "$candidateHash" || fail staged_hash
exact "$installed" "$stableHash" && idle && metadata "$stableTag" 4 || fail before_exchange
exchange=1
"$admin" --quiesce "$stableTag" || fail stable_quiesce
exact "$stage/candidate.incoming" "$candidateHash" && exact "$installed" "$stableHash" || fail pre_replace_hash
mv -f "$stage/candidate.incoming" "$installed" && sync || fail candidate_replace
exact "$installed" "$candidateHash" && "$admin" --wait "$candidateTag" 0000000000000004 || fail candidate_wait
idle && metadata "$candidateTag" 5 || fail candidate_generation
mark RUNNING_TAG220_ONE_SHARD
rc=0
LIBRARY_PATH=/boot/system/lib "$client" --compare 5 > "$stage/compare.stdout" 2> "$stage/compare.stderr" || rc=$?
printf '%s\n' "$rc" > "$stage/compare.exit"
[ "$rc" = 0 ] && receipt || fail compare_receipt
printf '%s\n' TAG220_CLIENT_CLOSED_NO_TOKEN_RATE
