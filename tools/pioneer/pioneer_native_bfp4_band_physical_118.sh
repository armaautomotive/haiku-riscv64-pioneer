#!/bin/sh
# ONE fixed synthetic BFP4 K5120 trial. Unproven results quarantine ownership:
# never retry, issue END, reset, query GPU or restore after a failed client.
set -eu
[ "$#" = 0 ] && [ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ] || exit 2
stage=/boot/home/pioneer-native-bfp4-band-118-20261106-a
installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
stable=/boot/home/pioneer-p100a-hot-reload-20261019-a/tenstorrent_pci_probe-scale-reuse-qualification-041fef18
admin=/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload
candidate=$stage/tenstorrent_pci_probe-native-bfp4-band118
client=$stage/pioneer-native-bfp4-band-diagnostic
reference=$stage/reference.txt
stable_hash=041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
candidate_hash=c5882d54033948ce1531359ab262d24b84653c75c97b73a78be374cdffa88db8
client_hash=7d6ec8b8a8ac3a0724f31c6c764fd97733791cd18f25e7c25e714ea0ce697516
reference_hash=3448d29dc93451b95262466950541b33f013dd41f30db9275a09dbffcdd296c9
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
 value=$("$admin" --status) || return 1
 printf '%s\n' "$value"
 [ "$value" = "P100A_RELOAD driver55 tag$1 generation$2 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY" ]
}
health(){
 value=$(sysinfo) || return 1
 printf '%s\n' "$value"
 printf '%s\n' "$value" | awk '/^64 .*running at/{cpus++} /used\/max [0-9]+ \/ 137034936320\)/{ram++} END{if(cpus!=1 || ram!=1)exit 1}'
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
test -f "$reference" && test ! -L "$reference" && [ "$(stat -c '%u:%h:%a' "$reference")" = 0:1:644 ] || exit 1
digest=$(sha256sum "$reference")
[ "${digest%% *}" = "$reference_hash" ] || exit 1
idle && metadata 2026102100010012 12 && health || exit 1
for name in .trial-once stable041.backup driver.incoming band5120.stdout band5120.stderr band5120.exit parent.pid state;do
 test ! -e "$stage/$name" && test ! -L "$stage/$name" || exit 1
done
mkdir "$stage/.trial-once" # NEVER removed, including failure
printf '%s\n' "$$" > "$stage/parent.pid"
printf '%s\n' ADMITTED_BEFORE_SWAP > "$stage/state"
cp -p "$stable" "$stage/stable041.backup"
exact "$stage/stable041.backup" "$stable_hash" || exit 1
# BFP4_FLOW_BEGIN
exchange "$stable_hash" "$candidate_hash" 2026102100010012 2026110600010118 12 13 "$candidate" || exit 1
printf '%s\n' NATIVE_INSTALLED_NO_GPU_STARTED > "$stage/state"
idle && metadata 2026110600010118 13 && exact "$installed" "$candidate_hash" && exact "$client" "$client_hash" || exit 1
printf '%s\n' RUNNING_ONE_FULL_K5120_BFP4 > "$stage/state"
if "$client" --fixed-k5120 > "$stage/band5120.stdout" 2> "$stage/band5120.stderr";then result=0;else result=$?;fi
printf '%s\n' "$result" > "$stage/band5120.exit"
cat "$stage/band5120.stdout"
cat "$stage/band5120.stderr"
if [ "$result" != 0 ];then
 printf '%s\n' UNPROVEN_FULL_K5120_BFP4_NO_END_RESET_RETRY_RESTORE > "$stage/state"
 echo BFP4_STOP_RETAIN_OWNER_NO_RETRY
 exit 70
fi
test ! -s "$stage/band5120.stderr" || exit 1
# BFP4_OUTPUT_GATE_BEGIN
awk '
 NR==FNR {if(NF!=2 || $1!~/^row([0-9]|[12][0-9]|3[01])$/ || length($2)!=8 || $2!~/^[0-9a-f]+$/ || prior[$1]++)bad++;value[$1]=$2;oldwords++;next}
 $1=="NATIVE_BFP_BAND_RETURNED_WORD" && !good && NF==6 && $2=="k5120" && $3~/^pass[01]$/ && $4~/^row([0-9]|[12][0-9]|3[01])$/ &&
 length($5)==11 && $5~/^got[0-9a-f]+$/ && length($6)==16 && $6~/^expected[0-9a-f]+$/ {
  if(seen[$3 SUBSEP $4]++ || substr($5,4)!=substr($6,9) || substr($5,4)!=value[$4])bad++
  words++;next
 }
 $1=="PASS_NATIVE_BFP_FULL_K" && $2=="bits4" && $3=="width5120" && $4=="rows32" && $5=="dual_jobs2" && $6=="phases2" && $7=="exact_words64" &&
 $8~/^polls[1-9][0-9]*$/ && substr($8,6)+0<=20000 &&
 $9=="generation13;EXPLICIT_END_L1_TLB_PCI_RESTORED;NO_MODEL_QUALITY_OR_SPEED_CLAIM" && NF==9 {if(words!=64)bad++;good++;next}
 NF{bad++} END{if(oldwords!=32 || words!=64 || good!=1 || bad)exit 1}' "$reference" "$stage/band5120.stdout" || exit 1
# BFP4_OUTPUT_GATE_END
metadata 2026110600010118 13 || exit 1
printf '%s\n' KNOWN_CLOSED_FULL_K5120_BFP4_EXACT64 > "$stage/state"
exchange "$candidate_hash" "$stable_hash" 2026110600010118 2026102100010012 13 14 "$stable" || exit 1
exact "$installed" "$stable_hash" && metadata 2026102100010012 14 && idle && health || exit 1
printf '%s\n' STABLE041_RESTORED_FULL_K5120_BFP4_EXACT64_PASSED > "$stage/state"
echo BFP4_KNOWN_CLEAN_NO_MODEL_SPEED_CLAIM
# BFP4_FLOW_END
