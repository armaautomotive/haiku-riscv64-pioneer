#!/bin/sh
# One fixed pair diagnostic. Never retry or restore after uncertainty.
set -eu
PATH=/boot/system/bin:/bin;export PATH
umask 077
[ "$#" = 1 ] || [ "$#" = 2 ] || exit 2
generation=$1;action=${2:---run}
[ "$action" = --run ] || [ "$action" = --check-only ] || exit 2
case "$generation" in ''|0*|*[!0-9]*) exit 2;; esac
[ "$generation" -gt 0 ] && [ "$generation" -le 999999 ] || exit 2
candidate_generation=$((generation+1));restored_generation=$((generation+2))
[ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ] || exit 2
stage=/boot/home/pioneer-recurrent-qkv-pair-20261113-e
prior=/boot/home/pioneer-recurrent-qkv-band-20261113-a
installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
stable=/boot/home/pioneer-p100a-hot-reload-20261019-a/tenstorrent_pci_probe-scale-reuse-qualification-041fef18
admin=/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload
guard=/boot/home/config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard
flush=/boot/home/pioneer-qgw8-flush-tool-20261107-a/checked-flush
stable_hash=041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
candidate_hash=5873c3f962e4c80ecd90c9ea1ce15ef7ef70851b69203c7d9654427eb9fbb660
client_hash=ffd07d7983d94e6a02e9de0aeda90196faa75ea3e4f4cabd4dc96ffb0ce1d40f
candidate=$stage/candidate;client=$stage/client;preflight=$prior/source-preflight;reference=$prior/independent-reference.log
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
 $1 ~ /^[0-9]+$/ && NF>=2 {teams++;if($1==self || $0 ~ /\/bin\/(bash|sh) -c .*pioneer-recurrent-qkv-pair-20261113-/)next
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
test -d "$prior" && test ! -L "$prior" && [ "$(stat -c '%u:%a' "$prior")" = 0:700 ] || exit 1
device=/boot/home/config/non-packaged/add-ons/kernel/drivers/dev/misc/tenstorrent_pci_probe
test -L "$device" && [ "$(readlink "$device")" = ../../bin/tenstorrent_pci_probe ] || exit 1
[ "$(stat -c '%d' "$stage")" = "$(stat -c '%d' "$installed")" ] || exit 1
exact "$stable" "$stable_hash" && exact "$installed" "$stable_hash" && exact "$candidate" "$candidate_hash" && exact "$client" "$client_hash" || exit 1
exact "$preflight" 41980f3c88a35ade0f596df0623405170a393bfa445dd43c5b74a6b11fbf5207 && \
 data_exact "$reference" c500fa4fc094e944a54204d8cb96f2275db1357b8d8646576abc97720283a4c0 || exit 1
exact "$admin" b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f && \
 exact "$guard" d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229 && \
 exact "$flush" 242400b3c143573ad6c8bd890e12114de4c1573b86d70a59499b0ed61227e90d || exit 1
original=/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf
test -f "$original" && test ! -L "$original" && [ "$(stat -c '%u:%h:%a:%s' "$original")" = 0:1:600:18973870528 ] || exit 1
idle && metadata 2026102100010012 "$generation" && health || exit 1
for name in .trial-once stable.backup driver.incoming trial.stdout trial.stderr trial.exit trial.parent.pid trial.state source.stdout source.stderr source.exit;do
 test ! -e "$stage/$name" && test ! -L "$stage/$name" || exit 1
done
if [ "$action" = --check-only ];then echo PAIR_CHECK_ONLY_NO_SWAP_OR_GPU_EXECUTION;exit 0;fi
# PAIR_QKV_FLOW_BEGIN
mkdir "$stage/.trial-once"
printf '%s\n' "$$" > "$stage/trial.parent.pid"
printf '%s\n' VERIFYING_SOURCE_BEFORE_SWAP > "$stage/trial.state"
if "$preflight" > "$stage/source.stdout" 2> "$stage/source.stderr";then result=0;else result=$?;fi
printf '%s\n' "$result" > "$stage/source.exit"
[ "$result" = 0 ] && test ! -s "$stage/source.stderr" && grep -Fx 'PASS_RECURRENT_QKV_SOURCE tensor5 samples3 fresh_payload_and_reference_exact; NO DEVICE OR SPEED' "$stage/source.stdout" >/dev/null || exit 70
cp -p "$stable" "$stage/stable.backup";exact "$stage/stable.backup" "$stable_hash" || exit 1
printf '%s\n' ADMITTED_BEFORE_SWAP > "$stage/trial.state"
exchange "$stable_hash" "$candidate_hash" 2026102100010012 2026111300010153 "$generation" "$candidate_generation" "$candidate" || exit 1
idle && metadata 2026111300010153 "$candidate_generation" && exact "$installed" "$candidate_hash" && exact "$client" "$client_hash" || exit 1
printf '%s\n' RUNNING_PAIR_DEBUG > "$stage/trial.state"
if "$client" --fixed-qkv-pair > "$stage/trial.stdout" 2> "$stage/trial.stderr";then result=0;else result=$?;fi
printf '%s\n' "$result" > "$stage/trial.exit"
if [ "$result" != 0 ] || test -s "$stage/trial.stderr";then
 printf '%s\n' UNPROVEN_NO_END_RESET_RETRY_RESTORE > "$stage/trial.state"
 echo PAIR_STOP_RETAIN_OWNER_NO_RETRY;exit 70
fi
# PAIR_QKV_OUTPUT_GATE_BEGIN
overlap=$(awk -v generation="$candidate_generation" '
 BEGIN{first[0]=0;first[1]=5120;first[2]=10208}
 NR==FNR {if($1=="QKV_INTEGER_REFERENCE"){
  s=substr($2,7)+0;r=substr($4,4)+0
  if(NF!=5 || $2!="sample"s || s>2 || $3!="first"first[s] || $4!="row"r || r>31 || length($5)!=12 || $5!~/^word[0-9a-f]+$/ || seenref[s SUBSEP r]++)bad++
  prior[s SUBSEP r]=substr($5,5);oldwords++}else if(NF)bad++;next}
 $0=="AUTHENTICATING_FIXED_ORIGINAL_AND_FRESH_LEARNED_REFERENCE;CPU_DIAGNOSTIC_ONLY_NO_DEVICE_YET" && !auth && !words {auth++;next}
 $1=="PAIR_QKV_WORD" && auth==1 && !good && NF==6 && $2~/^sample[01]$/ && $4~/^pass[01]$/ && $5~/^row([0-9]|[12][0-9]|3[01])$/ && length($6)==11 && $6~/^got[0-9a-f]+$/ {
  s=substr($2,7)+0;r=substr($5,4)+0
  if($3!="first"first[s] || seen[$2 SUBSEP $4 SUBSEP $5]++ || substr($6,4)!=prior[s SUBSEP r])bad++
  words++;next}
 $1=="PASS_NATIVE_QKV_PAIR" && $2=="samples2" && $3=="exact_words128" && $4~/^overlap[01]$/ && $5=="generation"generation";EXPLICIT_END_L1_TLB_PCI_RESTORED;NO_INFERENCE_SPEED_CLAIM" && auth==1 && words==128 && !good {good++;overlap=substr($4,8);next}
 NF{bad++} END{if(oldwords!=96 || words!=128 || auth!=1 || good!=1 || bad)exit 1;print overlap}' "$reference" "$stage/trial.stdout") || exit 1
# PAIR_QKV_OUTPUT_GATE_END
metadata 2026111300010153 "$candidate_generation" || exit 1
printf '%s\n' KNOWN_CLOSED_EXACT128 > "$stage/trial.state"
exchange "$candidate_hash" "$stable_hash" 2026111300010153 2026102100010012 "$candidate_generation" "$restored_generation" "$stable" || exit 1
exact "$installed" "$stable_hash" && metadata 2026102100010012 "$restored_generation" && idle && health || exit 1
printf '%s\n' "STABLE_RESTORED_EXACT128_OVERLAP$overlap" > "$stage/trial.state"
if [ "$overlap" = 1 ];then echo PAIR_OVERLAP_VERIFIED_NO_INFERENCE_SPEED_CLAIM;else echo PAIR_OUTPUTS_VALID_NO_OVERLAP_NO_SPEED_CLAIM;fi
# PAIR_QKV_FLOW_END
