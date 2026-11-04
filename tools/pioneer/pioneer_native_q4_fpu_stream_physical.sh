#!/bin/sh
# ONE fixed diagnostic86 stage. UNKNOWN stops;NO retry/reset/auto restore.
set -eu
[ "$#" = 0 ] && [ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ] || exit 2
stage=/boot/home/pioneer-native-q4-fpu-stream-20261104-a
installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
stable=/boot/home/pioneer-p100a-hot-reload-20261019-a/tenstorrent_pci_probe-scale-reuse-qualification-041fef18
admin=/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload
candidate=$stage/tenstorrent_pci_probe-native-q4-fpu-stream86
client=$stage/pioneer-native-q4-fpu-stream-diagnostic
stable_hash=041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
candidate_hash=742d6d57b6a521e035d22be93e3b20afcd3dc3bc35efc0b8e12bd1a9db2f0235
client_hash=9df5d0308cbc4f14e9b2d10904df6267b02da415c4622f204244808612a22f47
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
  if($0 ~ /(^|[\/[:space:]])(Assistant|HaikuAssistant|llama-|tenstorrent_haiku|inference-|pioneer_q38|pioneer-q|pioneer-native-|pioneer-dram|pioneer-assistant-cache|graph-csr-debug|source-check)/ ||
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
exact /boot/home/config/non-packaged/apps/HaikuAssistant 0de9141eb3955cf1c993a67c80ef648b3538ce7155c17677458487ee083a4933 || exit 1
# NATIVE_Q4_STREAM_PRIOR_GATE_BEGIN
reference=$stage/q4-stream-reference.log
test -f "$reference" && test ! -L "$reference" && [ "$(stat -c '%u:%h' "$reference")" = 0:1 ] || exit 1
digest=$(sha256sum "$reference")
[ "${digest%% *}" = 56ec253884deb7ba3f8a72dc09202f0b2a687752ebb15274843cbf14483008e9 ] || exit 1
original=/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf
test -f "$original" && test ! -L "$original" && [ "$(stat -c '%u:%h:%a:%s' "$original")" = 0:1:600:18973870528 ] || exit 1
# Full SAME-open SHA,fresh ALL-K compiled payload/inputs and reference in client BEFORE GPU open.
# NATIVE_Q4_STREAM_PRIOR_GATE_END
idle && metadata 2026102100010012 8 && health || exit 1
for name in .trial-once stable041.backup driver.incoming q4-stream.stdout q4-stream.stderr q4-stream.exit parent.pid state;do
 test ! -e "$stage/$name" && test ! -L "$stage/$name" || exit 1
done
umask 077
mkdir "$stage/.trial-once"
printf '%s\n' "$$" > "$stage/parent.pid"
printf '%s\n' ADMITTED_BEFORE_SWAP > "$stage/state"
cp -p "$stable" "$stage/stable041.backup"
exact "$stage/stable041.backup" "$stable_hash" || exit 1
# NATIVE_Q4_STREAM_FLOW_BEGIN
exchange "$stable_hash" "$candidate_hash" 2026102100010012 2026110400010086 8 9 "$candidate" || exit 1
idle && metadata 2026110400010086 9 && exact "$installed" "$candidate_hash" && exact "$client" "$client_hash" || exit 1
printf '%s\n' RUNNING_Q4_STREAM_SOURCE_AUTH_AND_NINE_FULL_K_SAMPLES > "$stage/state"
echo Q4_STREAM_SOURCE_AUTH_AND_NINE_FULL_K_SAMPLES_STARTED_WAIT_SAME_PARENT
if "$client" --fixed-q4-fpu-full-k-stream > "$stage/q4-stream.stdout" 2> "$stage/q4-stream.stderr";then result=0;else result=$?;fi
printf '%s\n' "$result" > "$stage/q4-stream.exit"
cat "$stage/q4-stream.stdout"
cat "$stage/q4-stream.stderr"
if [ "$result" != 0 ];then
 printf '%s\n' UNPROVEN_Q4_STREAM_NO_END_RESET_RETRY_RESTORE > "$stage/state"
 echo NATIVE_Q4_STREAM_STOP_RETAIN_OWNER_NO_RETRY
 exit 70
fi
test ! -s "$stage/q4-stream.stderr" || exit 1
# NATIVE_Q4_STREAM_OUTPUT_GATE_BEGIN
awk '
 BEGIN{samples=0}
 NR==FNR {if($1=="Q4_STREAM_FRACTION_REFERENCE"){prior[$2 SUBSEP $6]=substr($7,5);position[$2]=$4;tensor[$2]=$3;width[$2]=$5;oldwords++}next}
 $0=="AUTHENTICATING_FIXED_ORIGINAL_AND_FRESH_Q4_STREAM_REFERENCE;CPU_DIAGNOSTIC_ONLY_NO_DEVICE_YET" && !auth && !verified && !total {auth++;next}
 $0=="AUTHENTICATED_FIXED_Q4_STREAM_MODEL c600de03 samples9 widths5120_17408 rows32 full_k_payload_and_reference_exact;DEBUG_NOT_WORK" && auth==1 && !verified && !total {verified++;next}
 $1=="NATIVE_Q4_FPU_STREAM_RETURNED_WORD" && auth==1 && verified==1 && !good && samples<9 && NF==9 &&
 $2=="sample"samples && $3==tensor[$2] && $4==position[$2] && $5==width[$2] && words<64 && $6=="pass"int(words/32) && $7=="row"(words%32) &&
 length($8)==11 && $8~/^got[0-9a-f]+$/ && length($9)==16 && $9~/^expected[0-9a-f]+$/ {
  if(seen[$2 SUBSEP $6 SUBSEP $7]++ || substr($8,4)!=substr($9,9) || substr($8,4)!=prior[$2 SUBSEP $7])bad++
  words++;total++;next
 }
 $1=="PASS_NATIVE_Q4_FPU_STREAM_SAMPLE" && auth==1 && verified==1 && !good && samples<9 && NF==14 &&
 $2=="sample"samples && $3==tensor[$2] && $4==position[$2] && $5==width[$2] && $6=="phases4" && $7=="dual_jobs2" && $8=="exact_words64" &&
 $9~/^polls[1-9][0-9]*$/ && substr($9,6)+0<=20000 && $10=="generation9" && $11=="saved_l126908" {
  blocks=substr($5,6)+0;blocks/=256
  if(words!=64 || $12!="saved_gddr"(blocks*1664) || $13!="requests"(2*blocks*56) ||
   $14!="flits"(2*blocks*104)";EXPLICIT_END_L1_GDDR_NIU_FIFO_TLB_CLOCK_PCI_RESTORED;NO_FULL_MODEL_QUALITY_OR_SPEED_CLAIM")bad++
  samples++;words=0;next
 }
 $0=="PASS_NATIVE_Q4_FPU_STREAM samples9 exact_words576 clean_ends9;DEBUG_REFERENCE_NOT_WORK_NO_FULL_MODEL_QUALITY_OR_SPEED_CLAIM" &&
 auth==1 && verified==1 && samples==9 && total==576 && !words {good++;next}
 NF{bad++} END{if(oldwords!=288 || total!=576 || samples!=9 || auth!=1 || verified!=1 || good!=1 || bad)exit 1}' "$reference" "$stage/q4-stream.stdout" || exit 1
# NATIVE_Q4_STREAM_OUTPUT_GATE_END
metadata 2026110400010086 7 || exit 1
printf '%s\n' KNOWN_CLOSED_Q4_STREAM_EXACT576 > "$stage/state"
exchange "$candidate_hash" "$stable_hash" 2026110400010086 2026102100010012 9 10 "$stable" || exit 1
exact "$installed" "$stable_hash" && metadata 2026102100010012 10 && idle && health || exit 1
printf '%s\n' STABLE041_RESTORED_Q4_STREAM_EXACT576_PASSED > "$stage/state"
echo NATIVE_Q4_STREAM_KNOWN_CLEAN_NO_FULL_MODEL_SPEED_CLAIM
# NATIVE_Q4_STREAM_FLOW_END
