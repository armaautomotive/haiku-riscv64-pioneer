#!/bin/sh
# Fixed alpha/beta diagnostics only. Never retry or restore after uncertainty.
set -eu
PATH=/boot/system/bin:/bin
export PATH
umask 077
[ "$#" = 2 ] || [ "$#" = 3 ] || exit 2
kind=$1;generation=$2;action=${3:---run}
[ "$action" = --run ] || [ "$action" = --check-only ] || exit 2
case "$generation" in ''|0*|*[!0-9]*) exit 2;; esac
[ "$generation" -gt 0 ] && [ "$generation" -le 999999 ] || exit 2
candidate_generation=$((generation+1));restored_generation=$((generation+2))
case "$kind" in
 alpha)
  stage=/boot/home/pioneer-small-alpha-band-20261113-a
  candidate_tag=2026111300010149;tensor=11
  candidate_hash=1e850ea71a7a361d3d1a259626f6ad51c1270db9ee664cd47fbe445b6b7b01ea
  client_hash=09b912af473745e599f7e7a4b4bbcea81ff52c3e5ad97ce8d564fc6fda707eb6
  preflight_hash=0f3e072ae1c9d5bfc19250eee2aaaaf9dfa8bb193e2a32926b3605d46c61d005
  reference_hash=da1c6069bd8f850e402de13ad0178281baf7a076bd1b735542fa593e1788cdfe
  source_pass='PASS_SMALL_ALPHA_SOURCE tensor11 samples3 fresh_payload_and_reference_exact; NO DEVICE OR SPEED';;
 beta)
  stage=/boot/home/pioneer-small-beta-band-20261113-a
  candidate_tag=2026111300010150;tensor=12
  candidate_hash=0b1c07c4a22ec54d71848a9e3b36aa2140d10b76b83063a1c49b83430db44f1e
  client_hash=de1c4f64d9a11d84c39b671b6e8046c373d2de8cc1ef8ec2c627d47a90ba8a17
  preflight_hash=7ef135bb6b4672a56c6c69af63f919e2a5945c03eb808c51ad7f4a84c1787c20
  reference_hash=0b858fccc098aae4a9f2eb19891c33dd98347c95226de10e83732eea39ca8fdb
  source_pass='PASS_SMALL_BETA_SOURCE tensor12 samples3 fresh_payload_and_reference_exact; NO DEVICE OR SPEED';;
 qkv)
  stage=/boot/home/pioneer-recurrent-qkv-band-20261113-a
  candidate_tag=2026111300010151;tensor=5
  candidate_hash=d0fb7b1e5e9021a4974aa58ae7a120294f445b8aa7af7557815c648ca533ea73
  client_hash=e547a342e88b650b74277fc38e3a5c1ae9ec5a6538bd363a6aca3f5bb55778bd
  preflight_hash=41980f3c88a35ade0f596df0623405170a393bfa445dd43c5b74a6b11fbf5207
  reference_hash=c500fa4fc094e944a54204d8cb96f2275db1357b8d8646576abc97720283a4c0
  source_pass='PASS_RECURRENT_QKV_SOURCE tensor5 samples3 fresh_payload_and_reference_exact; NO DEVICE OR SPEED';;
 *) exit 2;;
esac
if [ "$kind" = qkv ];then matrix_rows=10240;first1=5120;first2=10208;reference_kind=QKV_INTEGER_REFERENCE;tail_padding=0
else matrix_rows=48;first1=32;first2=0;reference_kind=SMALL_INTEGER_REFERENCE;tail_padding=1;fi
[ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ] || exit 2
installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
stable=/boot/home/pioneer-p100a-hot-reload-20261019-a/tenstorrent_pci_probe-scale-reuse-qualification-041fef18
admin=/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload
guard=/boot/home/config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard
flush=/boot/home/pioneer-qgw8-flush-tool-20261107-a/checked-flush
stable_hash=041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
candidate=$stage/candidate;client=$stage/client;preflight=$stage/source-preflight;reference=$stage/independent-reference.log
exact(){
 test -f "$1" && test ! -L "$1" || return 1
 case "$(stat -c '%u:%h:%a' "$1")" in 0:1:700|0:1:755|0:1:555) ;; *) return 1;; esac
 digest=$(sha256sum "$1") || return 1;[ "${digest%% *}" = "$2" ]
}
data_exact(){
 test -f "$1" && test ! -L "$1" && [ "$(stat -c '%u:%h:%a' "$1")" = 0:1:600 ] || return 1
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
exact "$preflight" "$preflight_hash" && data_exact "$reference" "$reference_hash" || exit 1
exact "$admin" b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f && \
 exact "$guard" d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229 && \
 exact "$flush" 242400b3c143573ad6c8bd890e12114de4c1573b86d70a59499b0ed61227e90d || exit 1
original=/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf
test -f "$original" && test ! -L "$original" && [ "$(stat -c '%u:%h:%a:%s' "$original")" = 0:1:600:18973870528 ] || exit 1
idle && metadata 2026102100010012 "$generation" && health || exit 1
for name in .trial-once stable.backup driver.incoming trial.stdout trial.stderr trial.exit trial.parent.pid trial.state source.stdout source.stderr source.exit;do
 test ! -e "$stage/$name" && test ! -L "$stage/$name" || exit 1
done
if [ "$action" = --check-only ];then echo SMALL_BAND_CHECK_ONLY_NO_SWAP_OR_GPU_EXECUTION;exit 0;fi
# SMALL_RECURRENT_FLOW_BEGIN
mkdir "$stage/.trial-once"
printf '%s\n' "$$" > "$stage/trial.parent.pid"
printf '%s\n' VERIFYING_SOURCE_BEFORE_SWAP > "$stage/trial.state"
if "$preflight" > "$stage/source.stdout" 2> "$stage/source.stderr";then result=0;else result=$?;fi
printf '%s\n' "$result" > "$stage/source.exit"
[ "$result" = 0 ] && test ! -s "$stage/source.stderr" && grep -Fx "$source_pass" "$stage/source.stdout" >/dev/null || exit 70
cp -p "$stable" "$stage/stable.backup";exact "$stage/stable.backup" "$stable_hash" || exit 1
printf '%s\n' ADMITTED_BEFORE_SWAP > "$stage/trial.state"
exchange "$stable_hash" "$candidate_hash" 2026102100010012 "$candidate_tag" "$generation" "$candidate_generation" "$candidate" || exit 1
idle && metadata "$candidate_tag" "$candidate_generation" && exact "$installed" "$candidate_hash" && exact "$client" "$client_hash" || exit 1
printf '%s\n' RUNNING_SOURCE_AUTH_AND_THREE_SAMPLES > "$stage/trial.state"
if "$client" --fixed-learned-bands > "$stage/trial.stdout" 2> "$stage/trial.stderr";then result=0;else result=$?;fi
printf '%s\n' "$result" > "$stage/trial.exit"
if [ "$result" != 0 ] || test -s "$stage/trial.stderr";then
 printf '%s\n' UNPROVEN_NO_END_RESET_RETRY_RESTORE > "$stage/trial.state"
 echo SMALL_BAND_STOP_RETAIN_OWNER_NO_RETRY;exit 70
fi
# SMALL_RECURRENT_OUTPUT_GATE_BEGIN
awk -v tensor="$tensor" -v generation="$candidate_generation" -v matrix_rows="$matrix_rows" -v first1="$first1" -v first2="$first2" -v reference_kind="$reference_kind" -v tail_padding="$tail_padding" '
 BEGIN{samples=0;first[0]=0;first[1]=first1;first[2]=first2}
 NR==FNR {if($1==reference_kind){
  s=substr($2,7)+0;r=substr($4,4)+0
  if(NF!=5 || $2!="sample"s || s>2 || $3!="first"first[s] || $4!="row"r || r>31 || length($5)!=12 || $5!~/^word[0-9a-f]+$/ || seenref[s SUBSEP r]++)bad++
  prior[s SUBSEP r]=substr($5,5);if(tail_padding && s==1 && r>=16 && prior[s SUBSEP r]!="00000000")bad++;oldwords++}else if(NF)bad++;next}
 $0=="AUTHENTICATING_FIXED_ORIGINAL_AND_FRESH_LEARNED_REFERENCE;CPU_DIAGNOSTIC_ONLY_NO_DEVICE_YET" && !auth && !verified && !total {auth++;next}
 $0=="AUTHENTICATED_FIXED_MODEL c600de03 tensor"tensor" width5120 rows"matrix_rows" samples3 payload_and_reference_exact;DEBUG_NOT_WORK" && auth==1 && !verified && !total {verified++;next}
 $1=="NATIVE_LEARNED_BAND_RETURNED_WORD" && auth==1 && verified==1 && !good && samples<3 && NF==7 &&
 $2=="sample"samples && $3=="first"first[samples] && $4~/^pass[01]$/ && $5~/^row([0-9]|[12][0-9]|3[01])$/ &&
 length($6)==11 && $6~/^got[0-9a-f]+$/ && length($7)==16 && $7~/^expected[0-9a-f]+$/ {
  row=substr($5,4)+0
  if(seen[$2 SUBSEP $4 SUBSEP $5]++ || substr($6,4)!=substr($7,9) || substr($6,4)!=prior[samples SUBSEP row])bad++
  words++;total++;next}
 $1=="PASS_NATIVE_LEARNED_BAND" && auth==1 && verified==1 && !good && samples<3 && NF==10 && $2=="sample"samples && $3=="tensor"tensor && $4=="first"first[samples] &&
 $5=="width5120" && $6=="phases4" && $7=="dual_jobs2" && $8=="exact_words64" && $9~/^polls[1-9][0-9]*$/ && substr($9,6)+0<=20000 &&
 $10=="generation"generation";EXPLICIT_END_L1_TLB_PCI_RESTORED;NO_FULL_MODEL_QUALITY_OR_SPEED_CLAIM" {if(words!=64)bad++;samples++;words=0;next}
 $0=="PASS_NATIVE_LEARNED_BANDS samples3 exact_words192 clean_ends3;DEBUG_REFERENCE_NOT_WORK_NO_FULL_MODEL_QUALITY_OR_SPEED_CLAIM" &&
 auth==1 && verified==1 && samples==3 && total==192 && !words {good++;next}
 NF{bad++} END{if(oldwords!=96 || total!=192 || samples!=3 || auth!=1 || verified!=1 || good!=1 || bad)exit 1}' "$reference" "$stage/trial.stdout" || exit 1
# SMALL_RECURRENT_OUTPUT_GATE_END
metadata "$candidate_tag" "$candidate_generation" || exit 1
printf '%s\n' KNOWN_CLOSED_EXACT192 > "$stage/trial.state"
exchange "$candidate_hash" "$stable_hash" "$candidate_tag" 2026102100010012 "$candidate_generation" "$restored_generation" "$stable" || exit 1
exact "$installed" "$stable_hash" && metadata 2026102100010012 "$restored_generation" && idle && health || exit 1
printf '%s\n' STABLE_RESTORED_EXACT192 > "$stage/trial.state"
echo SMALL_BAND_KNOWN_CLEAN_NO_FULL_MODEL_SPEED_CLAIM
# SMALL_RECURRENT_FLOW_END
