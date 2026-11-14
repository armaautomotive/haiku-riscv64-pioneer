#!/bin/sh
# One fixed synthetic DEBUG batch. Never retry or restore after uncertainty.
set -eu
PATH=/boot/system/bin:/bin
export PATH
umask 077
[ "$#" = 1 ] || [ "$#" = 2 ] || exit 2
generation=$1;action=${2:---run}
[ "$action" = --run ] || [ "$action" = --check-only ] || exit 2
case "$generation" in ''|0*|*[!0-9]*) exit 2;; esac
[ "$generation" -gt 0 ] && [ "$generation" -le 999999 ] || exit 2
candidate_generation=$((generation+1));restored_generation=$((generation+2))
[ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ] || exit 2
stage=/boot/home/pioneer-work-batch-debug-20261113-a
installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
stable=/boot/home/pioneer-p100a-hot-reload-20261019-a/tenstorrent_pci_probe-scale-reuse-qualification-041fef18
admin=/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload
guard=/boot/home/config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard
flush=/boot/home/pioneer-qgw8-flush-tool-20261107-a/checked-flush
stable_hash=041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
candidate_hash=b115cab685c461cbb1f152833f227c4a134fe114e89e0df9a51cc1d5ecba890f
client_hash=2432963fa571d94f527c4bcc260824731d6bc74b947ef330ace8b781af41d70a
candidate=$stage/candidate;client=$stage/client;candidate_tag=2026111300010152
exact(){
 test -f "$1" && test ! -L "$1" || return 1
 case "$(stat -c '%u:%h:%a' "$1")" in 0:1:700|0:1:755|0:1:555) ;; *) return 1;; esac
 digest=$(sha256sum "$1") || return 1;[ "${digest%% *}" = "$2" ]
}
metadata(){
 value=$("$admin" --status) || return 1;printf '%s\n' "$value"
 [ "$value" = "P100A_RELOAD driver55 tag$1 generation$2 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY" ]
}
idle(){
 listing=$(ps -o Id -o Team) || return 1
 printf '%s\n' "$listing" | awk -v self="$$" '
 $1=="Id" && $2=="Team" && NF==2 {header++;next}
 $1 ~ /^[0-9]+$/ && NF>=2 {teams++;if($1==self)next
  if($0 ~ /(^|[\/[:space:]])(Assistant|HaikuAssistant|llama-|tenstorrent_haiku|inference-|pioneer_q38|pioneer-q|pioneer-native-|pioneer-recurrent-band|pioneer-recurrent-qkv|pioneer-small-alpha|pioneer-small-beta|pioneer-dram|graph-csr-debug|source-check)/ ||
   $0 ~ /pioneer-p100a-(dram|batched-output|wide-gdn|token)/)owners++
  next}
 NF{bad++} END{if(header!=1 || !teams || bad || owners)exit 1}'
}
health(){
 value=$(sysinfo) || return 1
 printf '%s\n' "$value" | awk '/^64 .*running at/{cpus++} /used\/max [0-9]+ \/ 137034936320\)/{ram++} END{if(cpus!=1 || ram!=1)exit 1}'
}
checked_flush(){ exact "$flush" 242400b3c143573ad6c8bd890e12114de4c1573b86d70a59499b0ed61227e90d && "$flush" --flush-only /dev/disk/scsi/0/3/0/0; }
exchange(){
 oldhash=$1;newhash=$2;oldtag=$3;newtag=$4;oldgeneration=$5;newgeneration=$6;replacement=$7
 exact "$installed" "$oldhash" && exact "$replacement" "$newhash" && idle && metadata "$oldtag" "$oldgeneration" || return 1
 test ! -e "$stage/driver.incoming" && test ! -L "$stage/driver.incoming" || return 1
 cp -p "$replacement" "$stage/driver.incoming";chmod 755 "$stage/driver.incoming"
 exact "$stage/driver.incoming" "$newhash" && exact "$installed" "$oldhash" && idle && checked_flush || return 1
 "$admin" --quiesce "$oldtag" || return 1
 exact "$stage/driver.incoming" "$newhash" && exact "$installed" "$oldhash" || return 1
 mv -f "$stage/driver.incoming" "$installed"
 exact "$installed" "$newhash" && checked_flush || return 1
 previous=$(printf '%016x' "$oldgeneration") || return 1
 "$admin" --wait "$newtag" "$previous" || return 1
 metadata "$newtag" "$newgeneration"
}
test -d "$stage" && test ! -L "$stage" && [ "$(stat -c '%u:%a' "$stage")" = 0:700 ] || exit 1
device=/boot/home/config/non-packaged/add-ons/kernel/drivers/dev/misc/tenstorrent_pci_probe
test -L "$device" && [ "$(readlink "$device")" = ../../bin/tenstorrent_pci_probe ] || exit 1
[ "$(stat -c '%d' "$stage")" = "$(stat -c '%d' "$installed")" ] || exit 1
exact "$stable" "$stable_hash" && exact "$installed" "$stable_hash" && exact "$candidate" "$candidate_hash" && exact "$client" "$client_hash" || exit 1
exact "$admin" b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f && \
 exact "$guard" d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229 && \
 exact "$flush" 242400b3c143573ad6c8bd890e12114de4c1573b86d70a59499b0ed61227e90d || exit 1
idle && metadata 2026102100010012 "$generation" && health || exit 1
for name in .trial-once stable.backup driver.incoming trial.stdout trial.stderr trial.exit trial.parent.pid trial.state;do
 test ! -e "$stage/$name" && test ! -L "$stage/$name" || exit 1
done
if [ "$action" = --check-only ];then echo WORK_BATCH_CHECK_ONLY_NO_SWAP_OR_GPU_EXECUTION;exit 0;fi
# WORK_BATCH_FLOW_BEGIN
mkdir "$stage/.trial-once"
printf '%s\n' "$$" > "$stage/trial.parent.pid"
cp -p "$stable" "$stage/stable.backup";exact "$stage/stable.backup" "$stable_hash" || exit 1
printf '%s\n' ADMITTED_BEFORE_SWAP > "$stage/trial.state"
exchange "$stable_hash" "$candidate_hash" 2026102100010012 "$candidate_tag" "$generation" "$candidate_generation" "$candidate" || exit 1
idle && metadata "$candidate_tag" "$candidate_generation" && exact "$installed" "$candidate_hash" && exact "$client" "$client_hash" || exit 1
printf '%s\n' RUNNING_FIXED_SYNTHETIC_DEBUG > "$stage/trial.state"
if "$client" --fixed-synthetic > "$stage/trial.stdout" 2> "$stage/trial.stderr";then result=0;else result=$?;fi
printf '%s\n' "$result" > "$stage/trial.exit"
if [ "$result" != 0 ] || test -s "$stage/trial.stderr" || \
 [ "$(cat "$stage/trial.stdout")" != 'PASS_TWO_WAVE_BATCH_SYNTHETIC_REFERENCE_EXACT_KNOWN_CLEAN;NO_MODEL_OR_SPEED_CLAIM' ] || \
 [ "$(wc -l < "$stage/trial.stdout" | tr -d ' ')" != 1 ];then
 printf '%s\n' UNPROVEN_NO_END_RESET_RETRY_RESTORE > "$stage/trial.state"
 echo WORK_BATCH_STOP_RETAIN_OWNER_NO_RETRY;exit 70
fi
metadata "$candidate_tag" "$candidate_generation" || exit 1
printf '%s\n' DEBUG_CLOSED_EXACT > "$stage/trial.state"
exchange "$candidate_hash" "$stable_hash" "$candidate_tag" 2026102100010012 "$candidate_generation" "$restored_generation" "$stable" || exit 1
exact "$installed" "$stable_hash" && metadata 2026102100010012 "$restored_generation" && idle && health || exit 1
printf '%s\n' STABLE_RESTORED_EXACT > "$stage/trial.state"
echo WORK_BATCH_DEBUG_CLEAN_NO_FULL_MODEL_SPEED_CLAIM
# WORK_BATCH_FLOW_END
