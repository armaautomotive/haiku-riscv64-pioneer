#!/bin/sh
# NEW fixed remaining6144+17408 full-K BFP8 diagnostic;each needs clean END.
# Direct exec only. Failure:retain/close only,NO retry/reset/automatic restore.
set -eu
[ "$#" = 0 ] && [ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ] || exit 2
stage=/boot/home/pioneer-native-bfp-band-20261104-b
observed=/boot/home/pioneer-native-bfp-band-20261104-a
installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
stable=/boot/home/pioneer-p100a-hot-reload-20261019-a/tenstorrent_pci_probe-scale-reuse-qualification-041fef18
admin=/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload
candidate=$stage/tenstorrent_pci_probe-native-bfp-band79
client=$stage/pioneer-native-bfp-band-diagnostic
stable_hash=041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
candidate_hash=301cd00175c078b447a4204233ae04d5d30e897e888e8c4525189738e22a6189
client_hash=0283070c764a5d992407bb5e06212cd82d75536bef37caa6cbd825f176618fe3
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
exact "$stable" "$stable_hash" && exact "$installed" "$stable_hash" && exact "$candidate" "$candidate_hash" && exact "$client" "$client_hash" || exit 1
exact "$admin" b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f || exit 1
exact /boot/home/config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229 || exit 1
exact /boot/home/config/non-packaged/servers/HaikuAssistantQwen27 861e5287e6f417c28968422d01721c95bc2c73ed17e9111806dfa76477984a74 || exit 1
exact /boot/home/config/non-packaged/servers/HaikuAssistantService c509da438d06f7e0dbcb23f4ef0fec2b000bfab923f5c7e86836127736d29e18 || exit 1
# NATIVE_BFP_BAND_PRIOR_GATE_BEGIN
test -d "$observed" && test ! -L "$observed" && [ "$(stat -c '%u:%a' "$observed")" = 0:700 ] || exit 1
for name in state band5120.stdout band5120.exit;do
 test -f "$observed/$name" && test ! -L "$observed/$name" && [ "$(stat -c '%u:%h' "$observed/$name")" = 0:1 ] || exit 1
done
[ "$(cat "$observed/state")" = STABLE041_RESTORED_FULL_K5120_BFP8_EXACT64_PASSED ] || exit 1
[ "$(cat "$observed/band5120.exit")" = 0 ] || exit 1
digest=$(sha256sum "$observed/band5120.stdout")
[ "${digest%% *}" = 5c3a33f4a124860f81714eacf3a234204877c73cb31fe2c9f07c43faacdf491e ] || exit 1
reference=$stage/full-k-reference.log
test -f "$reference" && test ! -L "$reference" && [ "$(stat -c '%u:%h' "$reference")" = 0:1 ] || exit 1
digest=$(sha256sum "$reference")
[ "${digest%% *}" = e16f4282406ea8f21ea3b7bdf3f9100482e3eae1b9fc2e16cbcf4b1c7a3e00cb ] || exit 1
# NATIVE_BFP_BAND_PRIOR_GATE_END
idle && metadata 2026102100010012 6 && health || exit 1
for name in .trial-once stable041.backup driver.incoming band6144.stdout band6144.stderr band6144.exit band17408.stdout band17408.stderr band17408.exit parent.pid state;do
 test ! -e "$stage/$name" && test ! -L "$stage/$name" || exit 1
done
mkdir "$stage/.trial-once" # NEVER removed,including failure
printf '%s\n' "$$" > "$stage/parent.pid"
printf '%s\n' ADMITTED_BEFORE_SWAP > "$stage/state"
cp -p "$stable" "$stage/stable041.backup"
exact "$stage/stable041.backup" "$stable_hash" || exit 1
# NATIVE_BFP_REMAINING_FLOW_BEGIN
exchange "$stable_hash" "$candidate_hash" 2026102100010012 2026110400010079 6 7 "$candidate" || exit 1
for width in 6144 17408;do
 idle && metadata 2026110400010079 7 && exact "$installed" "$candidate_hash" && exact "$client" "$client_hash" || exit 1
 printf '%s\n' "RUNNING_FULL_K$width" > "$stage/state"
 if "$client" "--fixed-k$width" > "$stage/band$width.stdout" 2> "$stage/band$width.stderr";then result=0;else result=$?;fi
 printf '%s\n' "$result" > "$stage/band$width.exit"
 cat "$stage/band$width.stdout"
 cat "$stage/band$width.stderr"
 if [ "$result" != 0 ];then
  printf '%s\n' UNPROVEN_REMAINING_FULL_K_NO_END_RESET_RETRY_RESTORE > "$stage/state"
  echo NATIVE_BFP_REMAINING_STOP_RETAIN_OWNER_NO_RETRY
  exit 70
 fi
 test ! -s "$stage/band$width.stderr" || exit 1
 # Independent reference key is a FIXED script loop value,never a caller selector.
 awk -v width="$width" '
  NR==FNR {if($1=="BAND_LEASE_SYNTHETIC_REFERENCE" && $2=="k"width){prior[$3]=substr($4,5);oldwords++}next}
  $1=="NATIVE_BFP_BAND_RETURNED_WORD" && !good && NF==6 && $2=="k"width && $3~/^pass[01]$/ && $4~/^row([0-9]|[12][0-9]|3[01])$/ &&
  length($5)==11 && $5~/^got[0-9a-f]+$/ && length($6)==16 && $6~/^expected[0-9a-f]+$/ {
   if(seen[$3 SUBSEP $4]++ || substr($5,4)!=substr($6,9) || substr($5,4)!=prior[$4])bad++
   words++;next
  }
  $1=="PASS_NATIVE_BFP_FULL_K" && $2=="bits8" && $3=="width"width && $4=="rows32" && $5=="dual_jobs2" && $6=="phases4" && $7=="exact_words64" &&
  $8~/^polls[1-9][0-9]*$/ && substr($8,6)+0<=20000 &&
  $9=="generation7;EXPLICIT_END_L1_TLB_PCI_RESTORED;NO_MODEL_QUALITY_OR_SPEED_CLAIM" && NF==9 {if(words!=64)bad++;good++;next}
  NF{bad++} END{if(oldwords!=32 || words!=64 || good!=1 || bad)exit 1}' "$reference" "$stage/band$width.stdout" || exit 1
 metadata 2026110400010079 7 || exit 1
 printf '%s\n' "KNOWN_CLOSED_FULL_K$width" > "$stage/state"
done
exchange "$candidate_hash" "$stable_hash" 2026110400010079 2026102100010012 7 8 "$stable" || exit 1
exact "$installed" "$stable_hash" && metadata 2026102100010012 8 && idle && health || exit 1
printf '%s\n' STABLE041_RESTORED_REMAINING_FULL_K_EXACT128_PASSED > "$stage/state"
echo NATIVE_BFP_REMAINING_KNOWN_CLEAN_NO_FULL_MODEL_SPEED_CLAIM
# NATIVE_BFP_REMAINING_FLOW_END
