#!/bin/sh
# One fixed real-Qwen BFP8 matrix diagnostic. Never retry/reset/restore on
# an uncertain result. This is NOT full-model inference or a Work mode.
set -eu
PATH=/boot/system/bin:/bin
export PATH
umask 077
[ "$#" = 0 ] || { [ "$#" = 1 ] && [ "$1" = --check-only ]; } || exit 2
[ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ] || exit 2
action=${1:---run}
stage=/boot/home/pioneer-recurrent-band-20261112-a
installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
stable=/boot/home/pioneer-p100a-hot-reload-20261019-a/tenstorrent_pci_probe-scale-reuse-qualification-041fef18
admin=/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload
guard=/boot/home/config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard
flush=/boot/home/pioneer-qgw8-flush-tool-20261107-a/checked-flush
candidate=$stage/tenstorrent_pci_probe-recurrent-band147
client=$stage/pioneer-recurrent-band-diagnostic147
reference=$stage/independent-reference.log
stable_hash=041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
candidate_hash=4303afa8c22582185071a8b7111ca0d6014953a151c904e88eab8bf2c16b82da
client_hash=afff31530bd9ce684b78d2c9ac8b3f5b22f052e98a84f318ee1e8cf316470e40
exact(){
 test -f "$1" && test ! -L "$1" || return 1
 case "$(stat -c '%u:%h:%a' "$1")" in 0:1:700|0:1:755|0:1:555) ;; *) return 1;; esac
 digest=$(sha256sum "$1") || return 1;[ "${digest%% *}" = "$2" ]
}
data_exact(){
 test -f "$1" && test ! -L "$1" || return 1
 case "$(stat -c '%u:%h:%a' "$1")" in 0:1:600|0:1:644) ;; *) return 1;; esac
 digest=$(sha256sum "$1") || return 1;[ "${digest%% *}" = "$2" ]
}
idle(){
 listing=$(ps -o Id -o Team) || return 1
 printf '%s\n' "$listing" | awk -v self="$$" '
 $1=="Id" && $2=="Team" && NF==2 {header++;next}
 $1 ~ /^[0-9]+$/ && NF>=2 {teams++;if($1==self)next
  if($0 ~ /(^|[\/[:space:]])(Assistant|HaikuAssistant|llama-|tenstorrent_haiku|inference-|pioneer_q38|pioneer-q|pioneer-native-|pioneer-recurrent-band|pioneer-dram|graph-csr-debug|source-check)/ ||
   $0 ~ /pioneer-p100a-(dram|batched-output|wide-gdn|token)/)owners++
  next}
 NF{bad++} END{if(header!=1 || !teams || bad || owners)exit 1}'
}
metadata(){
 value=$("$admin" --status) || return 1;printf '%s\n' "$value"
 [ "$value" = "P100A_RELOAD driver55 tag$1 generation$2 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY" ]
}
health(){
 value=$(sysinfo) || return 1
 printf '%s\n' "$value" | awk '/^64 .*running at/{cpus++} /used\/max [0-9]+ \/ 137034936320\)/{ram++} END{if(cpus!=1 || ram!=1)exit 1}'
}
cache_verified(){
 cache_dir=/boot/home/pioneer-recurrent-q8-cache-verify-20261112-a
 test -d "$cache_dir" && test ! -L "$cache_dir" && [ "$(stat -c '%u:%a' "$cache_dir")" = 0:700 ] || return 1
 for name in exit stderr stdout;do
  test -f "$cache_dir/$name" && test ! -L "$cache_dir/$name" || return 1
  case "$(stat -c '%u:%h:%a' "$cache_dir/$name")" in 0:1:600|0:1:644) ;; *) return 1;; esac
 done
 test -f "$cache_dir/exit" && test ! -L "$cache_dir/exit" && [ "$(/bin/cat "$cache_dir/exit")" = 0 ] \
  && test -f "$cache_dir/stderr" && test ! -L "$cache_dir/stderr" && test ! -s "$cache_dir/stderr" \
  && test -f "$cache_dir/stdout" && test ! -L "$cache_dir/stdout" \
  && grep -Fx 'RECURRENT_Q8_CACHE_MATCH fragments=1008 pages=1436160 raw=5882511360 padded=5882511360; NO DEVICE OR SPEED' "$cache_dir/stdout" >/dev/null
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
exact "$stage/pioneer-recurrent-source-preflight" 1c7d24da70f995f82d44da0c388ee6e93299f1e0f15d8cebcd8f94e361ac0f82 || exit 1
data_exact "$reference" 8bde663a476f09c74c219418a9406180a84d76657c2f491745f322d6537d0bb5 || exit 1
data_exact "$stage/private/PioneerNativeBfpModelBandData.h" c4781e0c6c3963ff1e1b3ac36c1d21fbb1dd74e2785826489cf67cbb139c9f36 && \
 data_exact "$stage/private/PioneerNativeBfpModelBandMetadata.h" a5fadca5d6baeaa921472989cf038dd4aa2a90a666f4c7b3262c8ee141841ff2 || exit 1
test -f "$stage/source-preflight-exit" && [ "$(/bin/cat "$stage/source-preflight-exit")" = 0 ] && test ! -s "$stage/source-preflight-stderr" || exit 1
grep -Fx 'PASS_RECURRENT_SOURCE tensor3 samples3 fresh_payload_and_reference_exact; NO DEVICE OR SPEED' "$stage/source-preflight-stdout" >/dev/null || exit 1
cache_verified || exit 1
original=/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf
test -f "$original" && test ! -L "$original" && [ "$(stat -c '%u:%h:%a:%s' "$original")" = 0:1:600:18973870528 ] || exit 1
idle && metadata 2026102100010012 36 && health || exit 1
for name in .recurrent-trial-once recurrent-stable.backup driver.incoming recurrent.stdout recurrent.stderr recurrent.exit recurrent.parent.pid recurrent.state;do
 test ! -e "$stage/$name" && test ! -L "$stage/$name" || exit 1
done
if [ "$action" = --check-only ];then
 echo RECURRENT_BAND_CHECK_ONLY_PASSED_NO_DRIVER_SWAP_OR_GPU_EXECUTION
 exit 0
fi
mkdir "$stage/.recurrent-trial-once"
printf '%s\n' "$$" > "$stage/recurrent.parent.pid"
printf '%s\n' ADMITTED_BEFORE_SWAP > "$stage/recurrent.state"
cp -p "$stable" "$stage/recurrent-stable.backup"
exact "$stage/recurrent-stable.backup" "$stable_hash" || exit 1
# RECURRENT_BAND_FLOW_BEGIN
exchange "$stable_hash" "$candidate_hash" 2026102100010012 2026111200010147 36 37 "$candidate" || exit 1
idle && metadata 2026111200010147 37 && exact "$installed" "$candidate_hash" && exact "$client" "$client_hash" || exit 1
printf '%s\n' RUNNING_RECURRENT_SOURCE_AUTH_AND_THREE_SAMPLES > "$stage/recurrent.state"
if "$client" --fixed-learned-bands > "$stage/recurrent.stdout" 2> "$stage/recurrent.stderr";then result=0;else result=$?;fi
printf '%s\n' "$result" > "$stage/recurrent.exit"
if [ "$result" != 0 ] || test -s "$stage/recurrent.stderr";then
 printf '%s\n' UNPROVEN_RECURRENT_BANDS_NO_END_RESET_RETRY_RESTORE > "$stage/recurrent.state"
 echo RECURRENT_BAND_STOP_RETAIN_OWNER_NO_RETRY
 exit 70
fi
# RECURRENT_BAND_OUTPUT_GATE_BEGIN
awk '
 BEGIN{samples=0;first[0]=0;first[1]=3072;first[2]=6112}
 NR==FNR {if($1=="RECURRENT_INTEGER_REFERENCE"){
  s=substr($2,7)+0;r=substr($4,4)+0
  if($2!="sample"s || s>2 || $3!="first"first[s] || $4!="row"r || r>31 || length($5)!=12 || $5!~/^word[0-9a-f]+$/ || seenref[s SUBSEP r]++)bad++
  prior[s SUBSEP r]=substr($5,5);oldwords++}next}
 $0=="AUTHENTICATING_FIXED_ORIGINAL_AND_FRESH_LEARNED_REFERENCE;CPU_DIAGNOSTIC_ONLY_NO_DEVICE_YET" && !auth && !verified && !total {auth++;next}
 $0=="AUTHENTICATED_FIXED_MODEL c600de03 tensor3 width5120 rows6144 samples3 payload_and_reference_exact;DEBUG_NOT_WORK" && auth==1 && !verified && !total {verified++;next}
 $1=="NATIVE_LEARNED_BAND_RETURNED_WORD" && auth==1 && verified==1 && !good && samples<3 && NF==7 &&
 $2=="sample"samples && $3=="first"first[samples] && $4~/^pass[01]$/ && $5~/^row([0-9]|[12][0-9]|3[01])$/ &&
 length($6)==11 && $6~/^got[0-9a-f]+$/ && length($7)==16 && $7~/^expected[0-9a-f]+$/ {
  row=substr($5,4)+0
  if(seen[$2 SUBSEP $4 SUBSEP $5]++ || substr($6,4)!=substr($7,9) || substr($6,4)!=prior[samples SUBSEP row])bad++
  words++;total++;next}
 $1=="PASS_NATIVE_LEARNED_BAND" && auth==1 && verified==1 && !good && samples<3 && NF==10 && $2=="sample"samples && $3=="tensor3" && $4=="first"first[samples] &&
 $5=="width5120" && $6=="phases4" && $7=="dual_jobs2" && $8=="exact_words64" && $9~/^polls[1-9][0-9]*$/ && substr($9,6)+0<=20000 &&
 $10=="generation37;EXPLICIT_END_L1_TLB_PCI_RESTORED;NO_FULL_MODEL_QUALITY_OR_SPEED_CLAIM" {if(words!=64)bad++;samples++;words=0;next}
 $0=="PASS_NATIVE_LEARNED_BANDS samples3 exact_words192 clean_ends3;DEBUG_REFERENCE_NOT_WORK_NO_FULL_MODEL_QUALITY_OR_SPEED_CLAIM" &&
 auth==1 && verified==1 && samples==3 && total==192 && !words {good++;next}
 NF{bad++} END{if(oldwords!=96 || total!=192 || samples!=3 || auth!=1 || verified!=1 || good!=1 || bad)exit 1}' "$reference" "$stage/recurrent.stdout" || exit 1
# RECURRENT_BAND_OUTPUT_GATE_END
metadata 2026111200010147 37 || exit 1
printf '%s\n' KNOWN_CLOSED_RECURRENT_BANDS_EXACT192 > "$stage/recurrent.state"
exchange "$candidate_hash" "$stable_hash" 2026111200010147 2026102100010012 37 38 "$stable" || exit 1
exact "$installed" "$stable_hash" && metadata 2026102100010012 38 && idle && health || exit 1
printf '%s\n' STABLE_RESTORED_RECURRENT_BANDS_EXACT192_PASSED > "$stage/recurrent.state"
echo RECURRENT_BAND_KNOWN_CLEAN_NO_FULL_MODEL_SPEED_CLAIM
# RECURRENT_BAND_FLOW_END
