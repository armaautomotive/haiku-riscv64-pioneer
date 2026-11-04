#!/bin/sh
# Fixed LOAD ONLY83 C64 placement CONTROL,not an established fix.
# Same packed bytes/16-deep request sequence;independent pre-reset checks.
# Direct exec only. Unknown execution/data:stop,NO further END/reset/retry.
set -eu
[ "$#" = 0 ] && [ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ] || exit 2
stage=/boot/home/pioneer-native-dram-load64-only-20261104-a
observed=/boot/home/pioneer-native-dram-load-only-20261104-a
installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
stable=/boot/home/pioneer-p100a-hot-reload-20261019-a/tenstorrent_pci_probe-scale-reuse-qualification-041fef18
admin=/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload
candidate=$stage/tenstorrent_pci_probe-native-dram-load83
client=$stage/pioneer-native-dram-load64-diagnostic
stable_hash=041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
candidate_hash=7ee39a3615bc93e3b81cd21149fdd020f2e5f3e7a7f1b3db8cca24bffd94a85e
client_hash=10310865e07c34b476632e8e776f76fae603a7a4adea2ec87f226f1f05798ac0
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
  if($0 ~ /(^|[\/[:space:]])(Assistant|HaikuAssistant|llama-|tenstorrent_haiku|inference-|pioneer_q38|pioneer-q|pioneer-native-bfp|pioneer-native-model|pioneer-native-dram|pioneer-dram|pioneer-assistant-cache|graph-csr-debug|source-check)/ ||
   $0 ~ /pioneer-p100a-(dram|batched-output|wide-gdn|token)/)owners++
  next
 }
 NF{bad++} END{if(header!=1 || !teams || bad || owners)exit 1}'
}
metadata(){
 value=$("$admin" --status) || return 1
 printf '%s\n' "$value" || return 1
 [ "$value" = "P100A_RELOAD driver55 tag$1 generation$2 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY" ]
}
health(){
 value=$(sysinfo) || return 1
 printf '%s\n' "$value" || return 1
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
 cp -p "$replacement" "$stage/driver.incoming" || return 1
 chmod 755 "$stage/driver.incoming" || return 1
 exact "$stage/driver.incoming" "$newhash" && exact "$installed" "$oldhash" && exact "$client" "$client_hash" && idle || return 1
 "$admin" --quiesce "$oldtag" || return 1
 exact "$stage/driver.incoming" "$newhash" && exact "$installed" "$oldhash" || return 1
 mv -f "$stage/driver.incoming" "$installed" || return 1
 sync || return 1
 exact "$installed" "$newhash" || return 1
 previous=$(printf '%016x' "$oldgeneration") || return 1
 "$admin" --wait "$newtag" "$previous" || return 1
 metadata "$newtag" "$newgeneration"
}
test -d "$stage" && test ! -L "$stage" && [ "$(stat -c '%u:%a' "$stage")" = 0:700 ] || exit 1
device=/boot/home/config/non-packaged/add-ons/kernel/drivers/dev/misc/tenstorrent_pci_probe
test -L "$device" && [ "$(readlink "$device")" = ../../bin/tenstorrent_pci_probe ] || exit 1
[ "$(stat -c '%d' "$stage")" = "$(stat -c '%d' "$installed")" ] || exit 1
exact "$stable" "$stable_hash" && exact "$installed" "$stable_hash" && exact "$candidate" "$candidate_hash" && exact "$client" "$client_hash" || exit 1
exact "$admin" b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f || exit 1
exact /boot/home/config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229 || exit 1
exact /boot/home/config/non-packaged/servers/HaikuAssistantQwen27 861e5287e6f417c28968422d01721c95bc2c73ed17e9111806dfa76477984a74 || exit 1
exact /boot/home/config/non-packaged/servers/HaikuAssistantService c509da438d06f7e0dbcb23f4ef0fec2b000bfab923f5c7e86836127736d29e18 || exit 1
# NATIVE_LOAD_ONLY_PRIOR_GATE_BEGIN
test -d "$observed" && test ! -L "$observed" && [ "$(stat -c '%u:%a' "$observed")" = 0:700 ] || exit 1
for name in state recovery.state load.stdout load.stderr load.exit;do
 test -f "$observed/$name" && test ! -L "$observed/$name" && [ "$(stat -c '%u:%h' "$observed/$name")" = 0:1 ] || exit 1
done
[ "$(cat "$observed/state")" = UNPROVEN_LOAD_ONLY_NO_FURTHER_END_RESET_RETRY_RESTORE ] || exit 1
[ "$(cat "$observed/load.exit")" = 1 ] || exit 1
[ "$(cat "$observed/recovery.state")" = NEW_POWER_EPOCH_STABLE041_RESTORED_CACHED_ASSISTANT_PRESERVED ] || exit 1
digest=$(sha256sum "$observed/load.stdout")
[ "${digest%% *}" = 63f6c0619cb2769932daf9d8b83d644076a70c953a867ab061c6135ec597d99d ] || exit 1
digest=$(sha256sum "$observed/load.stderr")
[ "${digest%% *}" = f21f376681d8ab635a7df556cc6f231f3ba52073f92668c27a3922981094da7e ] || exit 1
original=/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf
test -f "$original" && test ! -L "$original" && [ "$(stat -c '%u:%h:%a:%s' "$original")" = 0:1:600:18973870528 ] || exit 1
# SAME-open full original/payload proof in client before ANY device open.
# NATIVE_LOAD_ONLY_PRIOR_GATE_END
idle && metadata 2026102100010012 2 && health || exit 1
for name in .trial-once stable041.backup driver.incoming load.stdout load.stderr load.exit parent.pid state;do
 test ! -e "$stage/$name" && test ! -L "$stage/$name" || exit 1
done
umask 077
mkdir "$stage/.trial-once"
printf '%s\n' "$$" > "$stage/parent.pid"
printf '%s\n' ADMITTED_BEFORE_SWAP > "$stage/state"
cp -p "$stable" "$stage/stable041.backup"
exact "$stage/stable041.backup" "$stable_hash" || exit 1
# NATIVE_LOAD_ONLY_FLOW_BEGIN
exchange "$stable_hash" "$candidate_hash" 2026102100010012 2026110400010083 2 3 "$candidate" || exit 1
idle && metadata 2026110400010083 3 && exact "$installed" "$candidate_hash" && exact "$client" "$client_hash" || exit 1
printf '%s\n' RUNNING_LOAD64_ONLY_SOURCE_AUTH_AND_ONE_SAMPLE > "$stage/state"
echo LOAD64_ONLY_SOURCE_AUTH_AND_ONE_SAMPLE_STARTED_WAIT_SAME_PARENT
if "$client" --fixed-load64-only-band > "$stage/load.stdout" 2> "$stage/load.stderr";then result=0;else result=$?;fi
printf '%s\n' "$result" > "$stage/load.exit"
cat "$stage/load.stdout"
cat "$stage/load.stderr"
if [ "$result" != 0 ];then
 printf '%s\n' UNPROVEN_LOAD64_ONLY_NO_FURTHER_END_RESET_RETRY_RESTORE > "$stage/state"
 echo LOAD64_ONLY_STOP_RETAIN_OWNER_NO_RETRY
 exit 70
fi
test ! -s "$stage/load.stderr" || exit 1
# NATIVE_LOAD_ONLY_OUTPUT_GATE_BEGIN
awk '
 NR==1 && $0=="AUTHENTICATING_FIXED_ORIGINAL_AND_FRESH_LEARNED_REFERENCE;CPU_DIAGNOSTIC_ONLY_NO_DEVICE_YET" {auth++;next}
 NR==2 && $0=="AUTHENTICATED_FIXED_MODEL c600de03 tensor7 width5120 sample0;LOAD64_ONLY_NO_FPU_MODEL_OR_WORK" {verified++;next}
 NR==3 && NF==12 && $1=="PASS_NATIVE_DRAM_LOAD64_ONLY" && $2=="sample0" && $3=="tensor7" && $4=="width5120" &&
 $5=="requests320" && $6=="flits2880" && $7=="saved_l155372" && $8=="saved_gddr46080" &&
 $9=="payload_checked46080" && $10=="headers_checked5120" && $11~/^polls[1-9][0-9]*$/ && substr($11,6)+0<=20000 &&
 $12=="generation3;EXPLICIT_END_GDDR_L1_NIU_FIFO_TLB_CLOCK_PCI_RESTORED;NO_FPU_MODEL_QUALITY_OR_SPEED_CLAIM" {good++;next}
 NF{bad++} END{if(NR!=3 || auth!=1 || verified!=1 || good!=1 || bad)exit 1}' "$stage/load.stdout" || exit 1
# NATIVE_LOAD_ONLY_OUTPUT_GATE_END
metadata 2026110400010083 3 || exit 1
printf '%s\n' KNOWN_CLOSED_LOAD64_ONLY_BYTES_VERIFIED > "$stage/state"
exchange "$candidate_hash" "$stable_hash" 2026110400010083 2026102100010012 3 4 "$stable" || exit 1
exact "$installed" "$stable_hash" && metadata 2026102100010012 4 && idle && health || exit 1
printf '%s\n' STABLE041_RESTORED_LOAD64_ONLY_PAYLOAD_AND_HEADERS_PASSED > "$stage/state"
echo LOAD64_ONLY_KNOWN_CLEAN_NO_FPU_MODEL_OR_SPEED_CLAIM
# NATIVE_LOAD_ONLY_FLOW_END
