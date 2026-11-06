#!/bin/sh
# ONE fresh diagnostic113 manual-Last comparison. UNKNOWN stops;NO retry/reset/auto restore.
set -eu
[ "$#" = 0 ] && [ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ] || exit 2
stage=/boot/home/pioneer-native-factored-q4-manual113-a
installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
stable=/boot/home/pioneer-p100a-hot-reload-20261019-a/tenstorrent_pci_probe-scale-reuse-qualification-041fef18
admin=/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload
candidate=$stage/tenstorrent_pci_probe-native-q4-fpu-stream113
client=$stage/pioneer-native-factored-q4-manual-last
stable_hash=041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
candidate_hash=7ba43209502612a99cfbf9a51ce33f5251432661f22937559b195fb327addfd4
client_hash=f373138506859094a8010380babde1c7622165b42e3a2f6f0e243c3eecfe5bac
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
runtime=/boot/home/develop/llama-c060ca974c77/build-pioneer/bin
exact "$runtime/libllama.so.0.2.0" 0ae23a2c211af9fbafb89c3d93bdd136a4bdd574baeeaf2d622f582efcf426af || exit 1
exact "$runtime/libggml.so.0.21.0" a3b81c133641bbdacde8dd1d0e0ffa7a0a1ec0c33e95c426df1db59735379c16 || exit 1
exact "$runtime/libggml-base.so.0.21.0" 9cb45a36558bd78aa4d4b7ed5385cbf9df32de208f9e6c9b6d064e8527dca389 || exit 1
exact "$runtime/libggml-cpu.so.0.21.0" 69e9ec2c43a123afbb256c3feb6313cdcebdf1b2a87278ac3cdac8a1a65273d2 || exit 1
# Fresh SAME-open original/payload/OLD and NEW references run INSIDE the pinned
# client BEFORE GPU open. An old104 bit-reference cannot validate the new order.
original=/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf
test -f "$original" && test ! -L "$original" && [ "$(stat -c '%u:%h:%a:%s' "$original")" = 0:1:600:18973870528 ] || exit 1
idle && metadata 2026102100010012 2 && health || exit 1
for name in .trial-once stable041.backup driver.incoming factored-q4-compare.stdout factored-q4-compare.stderr factored-q4-compare.exit parent.pid state;do
 test ! -e "$stage/$name" && test ! -L "$stage/$name" || exit 1
done
umask 077
mkdir "$stage/.trial-once"
printf '%s\n' "$$" > "$stage/parent.pid"
printf '%s\n' ADMITTED_BEFORE_SWAP > "$stage/state"
cp -p "$stable" "$stage/stable041.backup"
exact "$stage/stable041.backup" "$stable_hash" || exit 1
# NATIVE_FACTORED_Q4_COMPARE_FLOW_BEGIN
exchange "$stable_hash" "$candidate_hash" 2026102100010012 2026110600010113 2 3 "$candidate" || exit 1
idle && metadata 2026110600010113 3 && exact "$installed" "$candidate_hash" && exact "$client" "$client_hash" || exit 1
printf '%s\n' RUNNING_FACTORED_Q4_COMPARE_SOURCE_AUTH_AND_TWO_FIXED_BANDS > "$stage/state"
echo FACTORED_Q4_COMPARE_SOURCE_AUTH_AND_TWO_FIXED_BANDS_STARTED_WAIT_SAME_PARENT
if "$client" --fixed-old-new-q4 > "$stage/factored-q4-compare.stdout" 2> "$stage/factored-q4-compare.stderr";then result=0;else result=$?;fi
printf '%s\n' "$result" > "$stage/factored-q4-compare.exit"
cat "$stage/factored-q4-compare.stdout"
cat "$stage/factored-q4-compare.stderr"
if [ "$result" != 0 ];then
 printf '%s\n' UNPROVEN_FACTORED_Q4_COMPARE_NO_END_RESET_RETRY_RESTORE > "$stage/state"
 echo NATIVE_FACTORED_Q4_COMPARE_STOP_RETAIN_OWNER_NO_RETRY
 sync
 exit 70
fi
test ! -s "$stage/factored-q4-compare.stderr" || exit 1
# FACTORED_COMPARE_OUTPUT_GATE_BEGIN
awk '
 BEGIN{words=0;dataWords=0;dataTotal=0;dataDone=0;total=0;samples=0;timers=0;auth=0;verified=0;good=0;bad=0}
 $0=="FACTORED_COMPARE_AUTHENTICATING_ORIGINAL_AND_BOTH_CPU_REFERENCES;NO_DEVICE_OPEN" && !auth && !verified && !total {auth++;next}
 $0=="FACTORED_COMPARE_AUTHENTICATED c600de03 all9_originalPayloads_bothArithmeticReferences;CPU_DIAGNOSTIC_NOT_WORK" && auth==1 && !verified && !total {verified++;next}
 $1=="FACTORED_COMPARE_DATA_WORD" && NF==7 && auth==1 && verified==1 && !good && samples<2 && timers==samples*2 &&
 !words && !dataDone && dataWords<64 && $2=="sample"(samples?6:0) && $3=="kind"int(dataWords/32) && $4=="row"(dataWords%32) &&
 length($5)==11 && $5~/^got[0-9a-f]+$/ && length($6)==16 && $6~/^expected[0-9a-f]+$/ && $7=="exact1" {
  if(seen["DATA" SUBSEP $2 SUBSEP $3 SUBSEP $4]++ || substr($5,4)!=substr($6,9))bad++
  dataWords++;dataTotal++;next
 }
 $1=="FACTORED_COMPARE_DATA_DONE" && NF==4 && auth==1 && verified==1 && !good && samples<2 && timers==samples*2 &&
 !words && !dataDone && dataWords==64 && $2=="sample"(samples?6:0) && $3=="words64" &&
 $4=="mismatches0;DATA_ONLY_NO_END_OR_SPEED_AUTHORITY" {dataDone=1;next}
 $1=="FACTORED_COMPARE_WORD" && NF==6 && auth==1 && verified==1 && !good && samples<2 && timers==samples*2 && dataDone==1 && dataWords==64 &&
 $2=="sample"(samples?6:0) && words<64 && $3=="kind"int(words/32) && $4=="row"(words%32) &&
 length($5)==11 && $5~/^got[0-9a-f]+$/ && length($6)==16 && $6~/^expected[0-9a-f]+$/ {
  if(seen[$2 SUBSEP $3 SUBSEP $4]++ || substr($5,4)!=substr($6,9))bad++
  words++;total++;next
 }
 $1=="PASS_NATIVE_FACTORED_COMPARE" && NF==9 && auth==1 && verified==1 && !good && samples<2 &&
 $2=="sample"(samples?6:0) && $3=="width"(samples?17408:5120) && $4=="exact64" && $5=="generation3" && $6=="saved_l137136" && dataDone==1 && dataWords==64 {
  blocks=(samples?17408:5120)/256
  if(words!=64 || $7!="saved_gddr"(blocks*1280) || $8!="requests"(2*blocks*26) ||
   $9!="flits"(2*blocks*80)";EXPLICIT_END_L1_GDDR_NIU_TLB_FIFO_CLOCK_PCI_RESTORED_NOT_MODEL_TPS")bad++
  samples++;words=0;dataWords=0;dataDone=0;next
 }
 $1=="FACTORED_COMPARE_TIMING" && auth==1 && verified==1 && !good && samples>=1 && samples<=2 &&
 (timers==(samples-1)*2 || timers==(samples-1)*2+1) &&
 $2=="sample"(samples==1?0:6) && $3=="width"(samples==1?5120:17408) && $4=="kind"(timers%2) {
  available=NF==8 && $5=="state1" && $6~/^start[1-9][0-9]*$/ && $7~/^end[1-9][0-9]*$/ &&
   $8~/^cycles[1-9][0-9]*;0OLD_BF16_1NEW_FACTORED_NOT_MODEL_TPS$/
  unavailable=NF==5 && ($5=="state0;UNAVAILABLE_NOT_TIMING_OR_CLEANUP_AUTHORITY" || $5=="state2;UNAVAILABLE_NOT_TIMING_OR_CLEANUP_AUTHORITY")
  if(!available && !unavailable)bad++
  timers++;next
 }
 $0=="PASS_NATIVE_FACTORED_COMPARE_ALL samples2 exact128 cleanEnds2;DIAGNOSTIC_NOT_WORK_NOT_MODEL_QUALITY_OR_TPS" &&
 auth==1 && verified==1 && samples==2 && total==128 && dataTotal==128 && timers==4 && !words && !dataWords && !dataDone {good++;next}
 NF{bad++} END{if(total!=128 || dataTotal!=128 || samples!=2 || timers!=4 || auth!=1 || verified!=1 || good!=1 || bad)exit 1}' "$stage/factored-q4-compare.stdout" || exit 1
# FACTORED_COMPARE_OUTPUT_GATE_END
metadata 2026110600010113 3 || exit 1
printf '%s\n' KNOWN_CLOSED_FACTORED_Q4_COMPARE_EXACT128 > "$stage/state"
exchange "$candidate_hash" "$stable_hash" 2026110600010113 2026102100010012 3 4 "$stable" || exit 1
exact "$installed" "$stable_hash" && metadata 2026102100010012 4 && idle && health || exit 1
printf '%s\n' STABLE041_RESTORED_FACTORED_Q4_COMPARE_EXACT128 > "$stage/state"
echo NATIVE_FACTORED_Q4_COMPARE_KNOWN_CLEAN_NO_MODEL_TPS_CLAIM
sync
# NATIVE_FACTORED_Q4_COMPARE_FLOW_END
