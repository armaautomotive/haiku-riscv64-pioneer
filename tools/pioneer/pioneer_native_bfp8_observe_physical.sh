#!/bin/sh
# NEW frozen one-shot BFP8 observation,NOT the spent first native trial.
# Invoke directly with exec;no wrapper whose path resembles a GPU owner.
# Strict old Expected()/END policy unchanged;failure retains ALL64 words.
# NO retry/reset/trap cleanup/automatic restore after any unproven result.
set -eu
[ "$#" = 0 ] && [ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ] || exit 2
stage=/boot/home/pioneer-native-bfp8-observe-20261104-a
installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
device=/boot/home/config/non-packaged/add-ons/kernel/drivers/dev/misc/tenstorrent_pci_probe
stable=/boot/home/pioneer-p100a-hot-reload-20261019-a/tenstorrent_pci_probe-scale-reuse-qualification-041fef18
admin=/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload
guard=/boot/home/config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard
candidate=$stage/tenstorrent_pci_probe-native-bfp78
client=$stage/pioneer-native-bfp-diagnostic
stable_hash=041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
candidate_hash=e79e070b9e4fe434b868898748834e0cd89b03bcb250d9c1694b4d0b890f46ac
client_hash=428e7a15ad97501eb711dc6a2e59f30c68d14c1b022acc32b9b753ce793aefb4
exact(){
 test -f "$1" && test ! -L "$1" || return 1
 case "$(stat -c '%u:%h:%a' "$1")" in 0:1:700|0:1:755|0:1:555) ;; *) return 1;; esac
 digest=$(sha256sum "$1") || return 1
 [ "${digest%% *}" = "$2" ]
}
idle(){
 listing=$(ps -o Id -o Team) || return 1
 printf '%s\n' "$listing" | awk -v self="$$" '
 $1=="Id" && $2=="Team" && NF==2 {header++;next}
 $1 ~ /^[0-9]+$/ && NF>=2 {
  teams++;if($1==self)next
  if($0 ~ /(^|[\/[:space:]])(Assistant|HaikuAssistant|llama-|tenstorrent_haiku|inference-|pioneer_q38|pioneer-q|pioneer-native-bfp|pioneer-dram|pioneer-assistant-cache|graph-csr-debug|source-check)/ ||
   $0 ~ /pioneer-p100a-(dram|batched-output|wide-gdn|token)/)owners++
  next
 }
 NF{bad++} END{if(header!=1 || !teams || bad || owners)exit 1}'
}
metadata(){
 current=$("$admin" --status) || return 1
 printf '%s\n' "$current"
 [ "$current" = "P100A_RELOAD driver55 tag$1 generation$2 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY" ]
}
health(){
 value=$(sysinfo) || return 1
 printf '%s\n' "$value"
 printf '%s\n' "$value" | awk '
 /^64 .*running at/{cpus++}
 /used\/max [0-9]+ \/ 137034936320\)/{ram++}
 END{if(cpus!=1 || ram!=1)exit 1}'
}
exchange(){
 oldhash=$1;newhash=$2;oldtag=$3;newtag=$4;oldgeneration=$5;newgeneration=$6;replacement=$7
 exact "$installed" "$oldhash" && exact "$replacement" "$newhash" && idle || return 1
 metadata "$oldtag" "$oldgeneration" || return 1
 test ! -e "$stage/driver.incoming" && test ! -L "$stage/driver.incoming" || return 1
 cp -p "$replacement" "$stage/driver.incoming"
 chmod 755 "$stage/driver.incoming"
 exact "$stage/driver.incoming" "$newhash" && exact "$installed" "$oldhash" && exact "$client" "$client_hash" && idle || return 1
 "$admin" --quiesce "$oldtag" || return 1
 exact "$stage/driver.incoming" "$newhash" && exact "$installed" "$oldhash" || return 1
 mv -f "$stage/driver.incoming" "$installed"
 sync
 exact "$installed" "$newhash" || return 1
 previous=$(printf '%016x' "$oldgeneration")
 "$admin" --wait "$newtag" "$previous" || return 1
 metadata "$newtag" "$newgeneration"
}
test -d "$stage" && test ! -L "$stage" && [ "$(stat -c '%u:%a' "$stage")" = 0:700 ] || exit 1
test -L "$device" && [ "$(readlink "$device")" = ../../bin/tenstorrent_pci_probe ] || exit 1
[ "$(stat -c '%d' "$stage")" = "$(stat -c '%d' "$installed")" ] || exit 1
exact "$stable" "$stable_hash" && exact "$installed" "$stable_hash" && exact "$candidate" "$candidate_hash" && exact "$client" "$client_hash" || exit 1
exact "$admin" b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f || exit 1
exact "$guard" d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229 || exit 1
exact /boot/home/config/non-packaged/servers/HaikuAssistantQwen27 861e5287e6f417c28968422d01721c95bc2c73ed17e9111806dfa76477984a74 || exit 1
exact /boot/home/config/non-packaged/servers/HaikuAssistantService c509da438d06f7e0dbcb23f4ef0fec2b000bfab923f5c7e86836127736d29e18 || exit 1
closed=/boot/home/pioneer-lossless-cache-20261103-a
for suffix in state exit stdout stderr;do
 record=$closed/inference-four.$suffix
 test -f "$record" && test ! -L "$record" && [ "$(stat -c '%u:%h' "$record")" = 0:1 ] || exit 1
done
[ "$(cat "$closed/inference-four.exit")" = 0 ] || exit 1
[ "$(cat "$closed/inference-four.state")" = KNOWN_CLEAN_CACHED_FOUR_TOKEN_WORK_REQUEST_COMPLETED ] || exit 1
test ! -s "$closed/inference-four.stderr" || exit 1
digest=$(sha256sum "$closed/inference-four.stdout")
[ "${digest%% *}" = 316e1655edfac771e3edbda7f7a2408385ec44676c0d0a7e443d730ec38d6a2a ] || exit 1
idle && metadata 2026102100010012 2 && health || exit 1
for name in .trial-once stable041.backup driver.incoming fixed8.stdout fixed8.stderr fixed8.exit parent.pid state;do
 test ! -e "$stage/$name" && test ! -L "$stage/$name" || exit 1
done
mkdir "$stage/.trial-once" # NEVER removed,including failure
printf '%s\n' "$$" > "$stage/parent.pid"
printf '%s\n' ADMITTED_BEFORE_SWAP > "$stage/state"
cp -p "$stable" "$stage/stable041.backup"
exact "$stage/stable041.backup" "$stable_hash" || exit 1
# NATIVE_BFP8_OBSERVE_FLOW_BEGIN
exchange "$stable_hash" "$candidate_hash" 2026102100010012 2026110300010078 2 3 "$candidate" || exit 1
printf '%s\n' NATIVE_INSTALLED_NO_GPU_STARTED > "$stage/state"
idle && metadata 2026110300010078 3 && exact "$installed" "$candidate_hash" && exact "$client" "$client_hash" || exit 1
printf '%s\n' RUNNING_ONE_FIXED_BFP8 > "$stage/state"
if "$client" --fixed-bfp8 > "$stage/fixed8.stdout" 2> "$stage/fixed8.stderr";then result=0;else result=$?;fi
printf '%s\n' "$result" > "$stage/fixed8.exit"
cat "$stage/fixed8.stdout"
cat "$stage/fixed8.stderr" # ALL returned CPU-copy words,not a truncated head
if [ "$result" != 0 ];then
 printf '%s\n' UNPROVEN_FIXED_BFP8_NO_END_RESET_RETRY_RESTORE > "$stage/state"
 echo NATIVE_BFP8_OBSERVATION_STOP_RETAIN_OWNER_NO_RECLASSIFICATION
 exit 70
fi
awk '
 $1=="PASS_NATIVE_BFP_FIXED_KERNEL" && $2=="bits8" && $3=="rows32" && $4=="dual_jobs2" && $5=="exact_words64" && $6 ~ /^polls[1-9][0-9]*$/ &&
 $7=="generation3;EXPLICIT_END_L1_TLB_PCI_RESTORED;NO_FULL_MODEL_QUALITY_OR_SPEED_CLAIM" && NF==7 {good++;next}
 NF{bad++} END{if(good!=1 || bad)exit 1}' "$stage/fixed8.stdout" || exit 1
test ! -s "$stage/fixed8.stderr" || exit 1
metadata 2026110300010078 3 || exit 1
printf '%s\n' KNOWN_CLOSED_FIXED_BFP8_STRICT_IDEAL > "$stage/state"
exchange "$candidate_hash" "$stable_hash" 2026110300010078 2026102100010012 3 4 "$stable" || exit 1
exact "$installed" "$stable_hash" && metadata 2026102100010012 4 && idle && health || exit 1
printf '%s\n' STABLE041_RESTORED_FIXED_BFP8_STRICT_IDEAL_PASSED > "$stage/state"
echo NATIVE_BFP8_OBSERVATION_KNOWN_CLEAN_NO_MODEL_SPEED_CLAIM
# NATIVE_BFP8_OBSERVE_FLOW_END
