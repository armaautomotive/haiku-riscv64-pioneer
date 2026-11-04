#!/bin/sh
# NEW fixed LEARNED-weight diagnostic80,three samples,all need clean END.
# Direct exec only. Failure:retain/close only,NO retry/reset/automatic restore.
set -eu
[ "$#" = 0 ] && [ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ] || exit 2
stage=/boot/home/pioneer-native-model-band-20261104-a
observed=/boot/home/pioneer-native-bfp-band-20261104-b
installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
stable=/boot/home/pioneer-p100a-hot-reload-20261019-a/tenstorrent_pci_probe-scale-reuse-qualification-041fef18
admin=/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload
candidate=$stage/tenstorrent_pci_probe-native-model-band80
client=$stage/pioneer-native-model-band-diagnostic
stable_hash=041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
candidate_hash=04a03e6d725a9406843c8808df68e363af3a5d664bc7d65efdac9d5ccbed1327
client_hash=f63e878f128e9c6d7f23453477c32fdd76b1eccfc643b0b321ed6bacacf9b587
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
  if($0 ~ /(^|[\/[:space:]])(Assistant|HaikuAssistant|llama-|tenstorrent_haiku|inference-|pioneer_q38|pioneer-q|pioneer-native-bfp|pioneer-native-model|pioneer-dram|pioneer-assistant-cache|graph-csr-debug|source-check)/ ||
   $0 ~ /pioneer-p100a-(dram|batched-output|wide-gdn|token)/)owners++
  next
 }
 NF{bad++} END{if(header!=1 || !teams || bad || owners)exit 1}'
}
metadata(){
 value=$("$admin" --status) || return 1
 printf '%s\n' "$value"
 [ "$value" = "P100A_RELOAD driver55 tag$1 generation$2 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY" ]
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
device=/boot/home/config/non-packaged/add-ons/kernel/drivers/dev/misc/tenstorrent_pci_probe
test -L "$device" && [ "$(readlink "$device")" = ../../bin/tenstorrent_pci_probe ] || exit 1
[ "$(stat -c '%d' "$stage")" = "$(stat -c '%d' "$installed")" ] || exit 1
exact "$stable" "$stable_hash" && exact "$installed" "$stable_hash" && exact "$candidate" "$candidate_hash" && exact "$client" "$client_hash" || exit 1
exact "$admin" b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f || exit 1
exact /boot/home/config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229 || exit 1
exact /boot/home/config/non-packaged/servers/HaikuAssistantQwen27 861e5287e6f417c28968422d01721c95bc2c73ed17e9111806dfa76477984a74 || exit 1
exact /boot/home/config/non-packaged/servers/HaikuAssistantService c509da438d06f7e0dbcb23f4ef0fec2b000bfab923f5c7e86836127736d29e18 || exit 1
# NATIVE_LEARNED_PRIOR_GATE_BEGIN
test -d "$observed" && test ! -L "$observed" && [ "$(stat -c '%u:%a' "$observed")" = 0:700 ] || exit 1
for name in state band6144.stdout band6144.exit band17408.stdout band17408.exit;do
 test -f "$observed/$name" && test ! -L "$observed/$name" && [ "$(stat -c '%u:%h' "$observed/$name")" = 0:1 ] || exit 1
done
[ "$(cat "$observed/state")" = STABLE041_RESTORED_REMAINING_FULL_K_EXACT128_PASSED ] || exit 1
[ "$(cat "$observed/band6144.exit")" = 0 ] && [ "$(cat "$observed/band17408.exit")" = 0 ] || exit 1
digest=$(sha256sum "$observed/band6144.stdout")
[ "${digest%% *}" = 7ea5f7c6a9428f8797b97dc419f40428ca6e02929de242549b85b61724572ae1 ] || exit 1
digest=$(sha256sum "$observed/band17408.stdout")
[ "${digest%% *}" = e357e6b1bdfc5d36f17a136ed1088b12c0163d2223f68a4dffd4dd14c6de79ba ] || exit 1
reference=$stage/learned-reference.log
test -f "$reference" && test ! -L "$reference" && [ "$(stat -c '%u:%h' "$reference")" = 0:1 ] || exit 1
digest=$(sha256sum "$reference")
[ "${digest%% *}" = 35142021d16a51e50b38657418800f3dcb102ddadc3961ec680d16ebf693ce83 ] || exit 1
original=/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf
test -f "$original" && test ! -L "$original" && [ "$(stat -c '%u:%h:%a:%s' "$original")" = 0:1:600:18973870528 ] || exit 1
# Full SAME-open SHA/reference is checked INSIDE client before card open.
# NATIVE_LEARNED_PRIOR_GATE_END
idle && metadata 2026102100010012 8 && health || exit 1
for name in .trial-once stable041.backup driver.incoming learned.stdout learned.stderr learned.exit parent.pid state;do
 test ! -e "$stage/$name" && test ! -L "$stage/$name" || exit 1
done
mkdir "$stage/.trial-once" # NEVER removed,including failure
printf '%s\n' "$$" > "$stage/parent.pid"
printf '%s\n' ADMITTED_BEFORE_SWAP > "$stage/state"
cp -p "$stable" "$stage/stable041.backup"
exact "$stage/stable041.backup" "$stable_hash" || exit 1
# NATIVE_LEARNED_FLOW_BEGIN
exchange "$stable_hash" "$candidate_hash" 2026102100010012 2026110400010080 8 9 "$candidate" || exit 1
idle && metadata 2026110400010080 9 && exact "$installed" "$candidate_hash" && exact "$client" "$client_hash" || exit 1
printf '%s\n' RUNNING_LEARNED_SOURCE_AUTH_AND_THREE_SAMPLES > "$stage/state"
echo LEARNED_SOURCE_AUTH_AND_THREE_SAMPLES_STARTED_WAIT_SAME_PARENT
if "$client" --fixed-learned-bands > "$stage/learned.stdout" 2> "$stage/learned.stderr";then result=0;else result=$?;fi
printf '%s\n' "$result" > "$stage/learned.exit"
cat "$stage/learned.stdout"
cat "$stage/learned.stderr"
if [ "$result" != 0 ];then
 printf '%s\n' UNPROVEN_LEARNED_BANDS_NO_END_RESET_RETRY_RESTORE > "$stage/state"
 echo NATIVE_LEARNED_STOP_RETAIN_OWNER_NO_RETRY
 exit 70
fi
test ! -s "$stage/learned.stderr" || exit 1
# NATIVE_LEARNED_OUTPUT_GATE_BEGIN
awk '
 BEGIN{samples=0}
 NR==FNR {if($1=="LEARNED_FRACTION_REFERENCE"){prior[$2 SUBSEP $4]=substr($5,5);position[$2]=$3;oldwords++}next}
 $0=="AUTHENTICATING_FIXED_ORIGINAL_AND_FRESH_LEARNED_REFERENCE;CPU_DIAGNOSTIC_ONLY_NO_DEVICE_YET" && !auth && !verified && !total {auth++;next}
 $0=="AUTHENTICATED_FIXED_MODEL c600de03 tensor7 width5120 rows17408 samples3 payload_and_reference_exact;DEBUG_NOT_WORK" && auth==1 && !verified && !total {verified++;next}
 $1=="NATIVE_LEARNED_BAND_RETURNED_WORD" && auth==1 && verified==1 && !good && samples<3 && NF==7 &&
 $2=="sample"samples && $3==position[$2] && $4~/^pass[01]$/ && $5~/^row([0-9]|[12][0-9]|3[01])$/ &&
 length($6)==11 && $6~/^got[0-9a-f]+$/ && length($7)==16 && $7~/^expected[0-9a-f]+$/ {
  if(seen[$2 SUBSEP $4 SUBSEP $5]++ || substr($6,4)!=substr($7,9) || substr($6,4)!=prior[$2 SUBSEP $5])bad++
  words++;total++;next
 }
 $1=="PASS_NATIVE_LEARNED_BAND" && auth==1 && verified==1 && !good && samples<3 && NF==10 && $2=="sample"samples && $3=="tensor7" && $4==position[$2] &&
 $5=="width5120" && $6=="phases4" && $7=="dual_jobs2" && $8=="exact_words64" && $9~/^polls[1-9][0-9]*$/ && substr($9,6)+0<=20000 &&
 $10=="generation9;EXPLICIT_END_L1_TLB_PCI_RESTORED;NO_FULL_MODEL_QUALITY_OR_SPEED_CLAIM" {if(words!=64)bad++;samples++;words=0;next}
 $0=="PASS_NATIVE_LEARNED_BANDS samples3 exact_words192 clean_ends3;DEBUG_REFERENCE_NOT_WORK_NO_FULL_MODEL_QUALITY_OR_SPEED_CLAIM" &&
 auth==1 && verified==1 && samples==3 && total==192 && !words {good++;next}
 NF{bad++} END{if(oldwords!=96 || total!=192 || samples!=3 || auth!=1 || verified!=1 || good!=1 || bad)exit 1}' "$reference" "$stage/learned.stdout" || exit 1
# NATIVE_LEARNED_OUTPUT_GATE_END
metadata 2026110400010080 9 || exit 1
printf '%s\n' KNOWN_CLOSED_LEARNED_BANDS_EXACT192 > "$stage/state"
exchange "$candidate_hash" "$stable_hash" 2026110400010080 2026102100010012 9 10 "$stable" || exit 1
exact "$installed" "$stable_hash" && metadata 2026102100010012 10 && idle && health || exit 1
printf '%s\n' STABLE041_RESTORED_LEARNED_BANDS_EXACT192_PASSED > "$stage/state"
echo NATIVE_LEARNED_KNOWN_CLEAN_NO_FULL_MODEL_SPEED_CLAIM
# NATIVE_LEARNED_FLOW_END
