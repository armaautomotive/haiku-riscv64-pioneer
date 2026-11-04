#!/bin/sh
# NEW strict documented-rounding fixed fixture,metadata-contract revision.
# Prior53f client stopped BEFORE SUPPORT/RUN:its metadata echo check cannot
# pass DriverReloadControl's zeroed selectors. No GPU retry/old-owner cleanup.
# Direct exec only. Failure:retain/close only,NO retry/reset/automatic restore.
set -eu
[ "$#" = 0 ] && [ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ] || exit 2
stage=/boot/home/pioneer-native-bfp8-documented-20261104-b
admission=/boot/home/pioneer-native-bfp8-documented-20261104-a
observed=/boot/home/pioneer-native-bfp8-observe-20261104-a
installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
stable=/boot/home/pioneer-p100a-hot-reload-20261019-a/tenstorrent_pci_probe-scale-reuse-qualification-041fef18
admin=/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload
candidate=$stage/tenstorrent_pci_probe-native-bfp78
client=$stage/pioneer-native-bfp8-documented-diagnostic
stable_hash=041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
candidate_hash=e79e070b9e4fe434b868898748834e0cd89b03bcb250d9c1694b4d0b890f46ac
client_hash=e3ca77c4f3f314885fee3cdcb080c155c92d0e005a255b94ae072b84faa95715
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
exact "$stable" "$stable_hash" && exact "$installed" "$candidate_hash" && exact "$candidate" "$candidate_hash" && exact "$client" "$client_hash" || exit 1
exact "$admin" b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f || exit 1
exact /boot/home/config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229 || exit 1
exact /boot/home/config/non-packaged/servers/HaikuAssistantQwen27 861e5287e6f417c28968422d01721c95bc2c73ed17e9111806dfa76477984a74 || exit 1
exact /boot/home/config/non-packaged/servers/HaikuAssistantService c509da438d06f7e0dbcb23f4ef0fec2b000bfab923f5c7e86836127736d29e18 || exit 1
for name in state fixed8.stderr recovery.state;do
 test -f "$observed/$name" && test ! -L "$observed/$name" && [ "$(stat -c '%u:%h' "$observed/$name")" = 0:1 ] || exit 1
done
[ "$(cat "$observed/state")" = UNPROVEN_FIXED_BFP8_NO_END_RESET_RETRY_RESTORE ] || exit 1
[ "$(cat "$observed/recovery.state")" = NEW_POWER_EPOCH_STABLE041_RESTORED_CACHED_ASSISTANT_PRESERVED ] || exit 1
digest=$(sha256sum "$observed/fixed8.stderr")
[ "${digest%% *}" = 66112229d3b318d6e79864ecf6d45ce3007a32ae44b7b6c939ce7b05c6ccdf36 ] || exit 1
# NATIVE_BFP8_PRE_RUN_REFUSAL_GATE_BEGIN
test -d "$admission" && test ! -L "$admission" && [ "$(stat -c '%u:%a' "$admission")" = 0:700 ] || exit 1
exact "$admission/pioneer_native_bfp8_documented_physical.sh" 7b38656c597dc4809522737bae9dad6a67414326dca1d46c4afc4395db8f8de9 || exit 1
exact "$admission/pioneer-native-bfp8-documented-diagnostic" 53f89a183edca74fb2acadbfbbd3a2186847d5e5baafe5d533734c051ed95240 || exit 1
for name in state documented8.exit documented8.stdout documented8.stderr;do
 test -f "$admission/$name" && test ! -L "$admission/$name" && [ "$(stat -c '%u:%h' "$admission/$name")" = 0:1 ] || exit 1
done
[ "$(cat "$admission/state")" = UNPROVEN_DOCUMENTED_BFP8_NO_END_RESET_RETRY_RESTORE ] || exit 1
[ "$(cat "$admission/documented8.exit")" = 1 ] || exit 1
test ! -s "$admission/documented8.stdout" && test ! -s "$admission/documented8.stderr" || exit 1
idle && metadata 2026110300010078 3 && health || exit 1
# NATIVE_BFP8_PRE_RUN_REFUSAL_GATE_END
for name in .trial-once stable041.backup driver.incoming documented8.stdout documented8.stderr documented8.exit parent.pid state;do
 test ! -e "$stage/$name" && test ! -L "$stage/$name" || exit 1
done
mkdir "$stage/.trial-once" # NEVER removed,including failure
printf '%s\n' "$$" > "$stage/parent.pid"
printf '%s\n' ADMITTED_NATIVE_IDLE_PRIOR_CLIENT_NEVER_REACHED_SUPPORT_RUN > "$stage/state"
cp -p "$stable" "$stage/stable041.backup"
exact "$stage/stable041.backup" "$stable_hash" || exit 1
# NATIVE_BFP8_DOCUMENTED_FLOW_BEGIN
idle && metadata 2026110300010078 3 && exact "$installed" "$candidate_hash" && exact "$client" "$client_hash" || exit 1
printf '%s\n' RUNNING_ONE_DOCUMENTED_BFP8 > "$stage/state"
if "$client" --fixed-bfp8-documented-rounding > "$stage/documented8.stdout" 2> "$stage/documented8.stderr";then result=0;else result=$?;fi
printf '%s\n' "$result" > "$stage/documented8.exit"
cat "$stage/documented8.stdout"
cat "$stage/documented8.stderr"
if [ "$result" != 0 ];then
 printf '%s\n' UNPROVEN_DOCUMENTED_BFP8_NO_END_RESET_RETRY_RESTORE > "$stage/state"
 echo NATIVE_BFP8_DOCUMENTED_STOP_RETAIN_OWNER_NO_RETRY
 exit 70
fi
test ! -s "$stage/documented8.stderr" || exit 1
# NATIVE_BFP8_DOCUMENTED_OUTPUT_GATE_BEGIN
awk '
 NR==FNR {if($1=="NATIVE_BFP_RETURNED_WORD"){prior[$3 SUBSEP $4]=substr($5,4);oldwords++}next}
 $1=="NATIVE_BFP8_DOCUMENTED_RETURNED_WORD" && !good && NF==5 && $2~/^pass[01]$/ && $3~/^row([0-9]|[12][0-9]|3[01])$/ &&
 length($4)==11 && $4~/^got[0-9a-f]+$/ && length($5)==16 && $5~/^expected[0-9a-f]+$/ {
  if(seen[$2 SUBSEP $3]++ || substr($4,4)!=substr($5,9) || substr($4,4)!=prior[$2 SUBSEP $3])bad++
  words++;next
 }
 $1=="PASS_NATIVE_BFP8_DOCUMENTED_FIXED_KERNEL" && $2=="bits8" && $3=="rows32" && $4=="dual_jobs2" && $5=="exact_words64" && $6~/^polls[1-9][0-9]*$/ && substr($6,6)+0<=20000 &&
 $7=="generation3;EXPLICIT_END_L1_TLB_PCI_RESTORED;NO_FULL_K_MODEL_QUALITY_OR_SPEED_CLAIM" && NF==7 {if(words!=64)bad++;good++;next}
 NF{bad++} END{if(oldwords!=64 || words!=64 || good!=1 || bad)exit 1}' "$observed/fixed8.stderr" "$stage/documented8.stdout" || exit 1
# NATIVE_BFP8_DOCUMENTED_OUTPUT_GATE_END
metadata 2026110300010078 3 || exit 1
printf '%s\n' KNOWN_CLOSED_DOCUMENTED_BFP8_EXACT64 > "$stage/state"
exchange "$candidate_hash" "$stable_hash" 2026110300010078 2026102100010012 3 4 "$stable" || exit 1
exact "$installed" "$stable_hash" && metadata 2026102100010012 4 && idle && health || exit 1
printf '%s\n' STABLE041_RESTORED_DOCUMENTED_BFP8_EXACT64_PASSED > "$stage/state"
echo NATIVE_BFP8_DOCUMENTED_KNOWN_CLEAN_NO_FULL_K_MODEL_SPEED_CLAIM
# NATIVE_BFP8_DOCUMENTED_FLOW_END
