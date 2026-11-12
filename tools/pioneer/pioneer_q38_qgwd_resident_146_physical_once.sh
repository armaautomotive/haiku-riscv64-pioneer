#!/bin/sh
# Experimental fixed-input two-request QGWD resident tag146 diagnostic.
# NEVER launch until the predecessor and native safety review are complete.
# On uncertainty, stop: no retry, reset, or automatic driver restoration.
set -eu
PATH=/boot/system/bin:/bin
export PATH
[ "$#" = 0 ] || { [ "$#" = 1 ] && [ "$1" = --check-only ]; } || exit 2
[ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ] || exit 2
action=${1:---run}
umask 077
stage=/boot/home/pioneer-qgwd-resident-diagnostic-146-20261112-a
installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
device=/boot/home/config/non-packaged/add-ons/kernel/drivers/dev/misc/tenstorrent_pci_probe
stable=/boot/home/pioneer-p100a-hot-reload-20261019-a/tenstorrent_pci_probe-scale-reuse-qualification-041fef18
admin=/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload
guard=/boot/home/config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard
flush=/boot/home/pioneer-qgw8-flush-tool-20261107-a/checked-flush
candidate=$stage/tenstorrent_pci_probe-qgwd-resident-146
client=$stage/pioneer-q38-qgwd-resident-client146
preflight=/boot/home/pioneer-qgwd-four-cache-preflight-20261110-a/pioneer-q38-qgwd-cache-preflight
gate=$stage/pioneer_q38_qgwd_resident_candidate_output.awk
cache4=/HaikuData/pioneer-qgw8-bfp4-native-20261107
cache8=/boot/home/pioneer-hybrid-bfp8-20261107-cache
cachehead=/HaikuData/pioneer-qgw8-head-native-20261107
cachedense=/HaikuData/pioneer-qgwd-dense-bfp8-20261109-a
stable_hash=041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
candidate_hash=e809110d19ba4b63c93e1eb22c6d6a6c430bc8cefa17debef67a8bca9d69dcf2
client_hash=12f5e15d737229810b3c6cce7c229437f782f96727217fb838d47c5f5264d8ec
preflight_hash=0aee55451ae1d5e90e29060f119ad5a479a411841f6e93096f2456fe6bb668df
gate_hash=deb34cbc0ecfb620dc0549e526b1fe97b149b8cffa07d987a50e035732a3ad08
admin_hash=b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f
guard_hash=d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229
flush_hash=242400b3c143573ad6c8bd890e12114de4c1573b86d70a59499b0ed61227e90d
exact(){
 test -f "$1" && test ! -L "$1" || return 1
 case "$(stat -c '%u:%h:%a' "$1")" in 0:1:700|0:1:755|0:1:555) ;; *) return 1;; esac
 digest=$(sha256sum "$1") || return 1; [ "${digest%% *}" = "$2" ]
}
exact_data(){
 test -f "$1" && test ! -L "$1" || return 1
 case "$(stat -c '%u:%h:%a' "$1")" in 0:1:600|0:1:644) ;; *) return 1;; esac
 digest=$(sha256sum "$1") || return 1; [ "${digest%% *}" = "$2" ]
}
private_dir(){ test -d "$1" && test ! -L "$1" && [ "$(stat -c '%u:%a' "$1")" = 0:700 ]; }
previous_data(){
 test -f "$1" && test ! -L "$1" || return 1
 case "$(stat -c '%u:%h:%a' "$1")" in 0:1:600|0:1:644) ;; *) return 1;; esac
}
metadata(){
 value=$("$admin" --status) || return 1;printf '%s\n' "$value"
 [ "$value" = "P100A_RELOAD driver55 tag$1 generation$2 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY" ]
}
idle(){
 listing=$(ps -o Id -o Team) || return 1
 printf '%s\n' "$listing" | awk -v self="$$" '
 $1=="Id" && $2=="Team" && NF==2 {header++;next}
 $1 ~ /^[0-9]+$/ && NF>=2 {
  teams++;if($1==self)next
  if($0 ~ /(^|[\/[:space:]])(Assistant|HaikuAssistant|llama-|tenstorrent_haiku|inference-|pioneer_q38|pioneer-q|pioneer-native-|pioneer-dram|graph-csr-debug|source-check)/ ||
   $0 ~ /pioneer-p100a-(dram|batched-output|wide-gdn|token)/ || $0 ~ /(upload-test|preflight-test|full-test)/)owners++
  next
 }
 NF{bad++} END{if(header!=1 || !teams || bad || owners)exit 1}'
}
health(){
 value=$(sysinfo) || return 1
 printf '%s\n' "$value" | awk '/^64 .*running at/{cpus++} /used\/max [0-9]+ \/ 137034936320\)/{ram++} END{if(cpus!=1 || ram!=1)exit 1}'
}
checked_flush(){ exact "$flush" "$flush_hash" && "$flush" --flush-only /dev/disk/scsi/0/3/0/0; }
exchange(){
 oldhash=$1;newhash=$2;oldtag=$3;oldgeneration=$4;newtag=$5;newgeneration=$6;replacement=$7
 exact "$installed" "$oldhash" && exact "$replacement" "$newhash" && idle && metadata "$oldtag" "$oldgeneration" || return 1
 test ! -e "$stage/driver.incoming" && test ! -L "$stage/driver.incoming" || return 1
 cp -p "$replacement" "$stage/driver.incoming" || return 1
 chmod 755 "$stage/driver.incoming" || return 1
 exact "$stage/driver.incoming" "$newhash" && exact "$installed" "$oldhash" && idle && checked_flush || return 1
 "$admin" --quiesce "$oldtag" || return 1
 exact "$stage/driver.incoming" "$newhash" && exact "$installed" "$oldhash" || return 1
 mv -f "$stage/driver.incoming" "$installed" || return 1
 exact "$installed" "$newhash" && checked_flush || return 1
 previous=$(printf '%016x' "$oldgeneration") || return 1
 "$admin" --wait "$newtag" "$previous" || return 1
 metadata "$newtag" "$newgeneration"
}
private_dir "$stage" && private_dir "$cache4" && private_dir "$cache8" && private_dir "$cachehead" && private_dir "$cachedense" || exit 1
for name in .trial-once parent.pid driver.incoming stable.backup preflight.stdout preflight.stderr preflight.exit trial.stdout trial.stderr trial.exit state; do
 test ! -e "$stage/$name" && test ! -L "$stage/$name" || exit 1
done
exact "$stable" "$stable_hash" && exact "$installed" "$stable_hash" && exact "$candidate" "$candidate_hash" || exit 1
exact "$client" "$client_hash" && exact "$preflight" "$preflight_hash" && exact "$admin" "$admin_hash" && \
 exact "$guard" "$guard_hash" && exact "$flush" "$flush_hash" && exact_data "$gate" "$gate_hash" || exit 1
original=/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf
test -f "$original" && test ! -L "$original" && [ "$(stat -c '%u:%h:%a:%s' "$original")" = 0:1:600:18973870528 ] || exit 1
test -L "$device" && [ "$(readlink "$device")" = ../../bin/tenstorrent_pci_probe ] || exit 1
[ "$(stat -c '%d' "$stage")" = "$(stat -c '%d' "$installed")" ] || exit 1
previous_stage=/boot/home/pioneer-qgwf-unroll-diagnostic-145-20261111-a
private_dir "$previous_stage" || exit 1
for file in state preflight.exit preflight.stdout preflight.stderr trial.exit trial.stdout trial.stderr; do
 previous_data "$previous_stage/$file" || exit 1
done
[ "$(cat "$previous_stage/state")" = CANDIDATE_CLOSED_STABLE_RESTORED_QUALITY_UNVERIFIED ] || exit 1
[ "$(cat "$previous_stage/preflight.exit")" = 0 ] && test ! -s "$previous_stage/preflight.stderr" || exit 1
[ "$(cat "$previous_stage/trial.exit")" = 0 ] && test ! -s "$previous_stage/trial.stderr" || exit 1
[ "$(tail -1 "$previous_stage/preflight.stdout")" = 'PASS_QGWD_PREFLIGHT 1113 BFP4 FFN, 231 BFP8 FFN, 7 BFP8 head, 336 BFP8 dense fragments; NO_DEVICE_UPLOAD_MODEL_EXECUTION_OR_SPEED' ] || exit 1
exact_data "$previous_stage/pioneer_q38_qgwf_rate_candidate_output.awk" fe9c2824fe3feedd39c41e6f96bcbe246d3cd80329dd63954b6587d34a448eb9 || exit 1
awk -f "$previous_stage/pioneer_q38_qgwf_rate_candidate_output.awk" "$previous_stage/trial.stdout" || exit 1
idle && metadata 2026102100010012 34 && health || exit 1
if [ "$action" = --check-only ]; then
 echo QGWD_RESIDENT_ONE_SHOT_CHECK_ONLY_PASSED_NO_MODEL_READ_DRIVER_SWAP_OR_GPU_EXECUTION
 exit 0
fi
mkdir "$stage/.trial-once"
printf '%s\n' "$$" > "$stage/parent.pid"
trap 'code=$?; if [ "$code" -ne 0 ]; then printf "FAILED_%s_NO_AUTO_RETRY_RESET_OR_RESTORE\n" "$code" > "$stage/state"; fi' EXIT
checked_flush || exit 1
printf '%s\n' VERIFYING_MODEL_AND_CACHES_NO_DEVICE > "$stage/state"
if "$preflight" --verify-qgwd "$cache4" "$cache8" "$cachehead" "$cachedense" > "$stage/preflight.stdout" 2> "$stage/preflight.stderr"; then result=0;else result=$?;fi
printf '%s\n' "$result" > "$stage/preflight.exit"
checked_flush || exit 1
[ "$result" = 0 ] && test ! -s "$stage/preflight.stderr" && \
 [ "$(tail -1 "$stage/preflight.stdout")" = 'PASS_QGWD_PREFLIGHT 1113 BFP4 FFN, 231 BFP8 FFN, 7 BFP8 head, 336 BFP8 dense fragments; NO_DEVICE_UPLOAD_MODEL_EXECUTION_OR_SPEED' ] || exit 69
exact "$client" "$client_hash" && exact "$preflight" "$preflight_hash" && exact "$candidate" "$candidate_hash" && exact_data "$gate" "$gate_hash" || exit 1
idle && metadata 2026102100010012 34 && health || exit 1
cp -p "$stable" "$stage/stable.backup"
exact "$stage/stable.backup" "$stable_hash" && checked_flush || exit 1
printf '%s\n' BACKUP_VERIFIED > "$stage/state"
exchange "$stable_hash" "$candidate_hash" 2026102100010012 34 2026111100010146 35 "$candidate" || exit 1
exact "$client" "$client_hash" && idle && metadata 2026111100010146 35 || exit 1
printf '%s\n' CANDIDATE_RUNNING_OWNERSHIP_UNKNOWN > "$stage/state"
checked_flush || exit 1
if "$client" --run-qgwd-resident-146 "$cache4" "$cache8" "$cachehead" "$cachedense" > "$stage/trial.stdout" 2> "$stage/trial.stderr"; then result=0;else result=$?;fi
printf '%s\n' "$result" > "$stage/trial.exit"
checked_flush || exit 1
[ "$result" = 0 ] && test ! -s "$stage/trial.stderr" && exact_data "$gate" "$gate_hash" && awk -f "$gate" "$stage/trial.stdout" || exit 70
printf '%s\n' CANDIDATE_CLOSED_RESTORATION_PENDING > "$stage/state"
exact "$installed" "$candidate_hash" && idle && metadata 2026111100010146 35 || exit 71
exchange "$candidate_hash" "$stable_hash" 2026111100010146 35 2026102100010012 36 "$stage/stable.backup" || exit 72
exact "$installed" "$stable_hash" && idle && metadata 2026102100010012 36 && health || exit 73
printf '%s\n' CANDIDATE_CLOSED_STABLE_RESTORED_QUALITY_UNVERIFIED > "$stage/state"
checked_flush || exit 1
echo QGWD_RESIDENT_ONE_SHOT_CLOSED_STABLE_RESTORED_NO_QUALITY_CLAIM
