#!/bin/sh
# One candidate launch. An uncertain step is NEVER retried or auto-reset.
set -eu
PATH=/boot/system/bin:/bin
export PATH
[ "$#" = 0 ] && [ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ] || exit 2
umask 077
stage=/boot/home/pioneer-hybrid125-full-trial-20261107-a
installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
stable=/boot/home/pioneer-p100a-hot-reload-20261019-a/tenstorrent_pci_probe-scale-reuse-qualification-041fef18
admin=/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload
guard=/boot/home/config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard
candidate=$stage/driver125
client=$stage/full-test
preflight=$stage/preflight-test
gate=$stage/output.awk
cache4=/boot/home/pioneer-hybrid-bfp4-20261107-cache
cache8=/boot/home/pioneer-hybrid-bfp8-20261107-cache
stable_hash=041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
candidate_hash=379c12385a8069a2ca4d8b71dd3039cff7340022ca4fe3ad090427d5a7a46099
client_hash=9f8e5abb9543f7d2d31fbace9778bda0862867439c1d9f08d10f86335d5d5c92
admin_hash=b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f
guard_hash=d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229
preflight_hash=e354dbf45eebcee92479988391c3b9538390a742163e297e77bb33b5c62f49a6
gate_hash=6799fec637859923d1de6cc30735fef64bda2d84db2b7884b5deccdc34171e15
exact(){
 test -f "$1" && test ! -L "$1" || return 1
 case "$(stat -c '%u:%h:%a' "$1")" in 0:1:700|0:1:755|0:1:555) ;; *) return 1;; esac
 digest=$(sha256sum "$1") || return 1
 [ "${digest%% *}" = "$2" ]
}
exact_data(){
 test -f "$1" && test ! -L "$1" || return 1
 [ "$(stat -c '%u:%h:%a' "$1")" = 0:1:600 ] || return 1
 digest=$(sha256sum "$1") || return 1
 [ "${digest%% *}" = "$2" ]
}
private_dir(){ test -d "$1" && test ! -L "$1" && [ "$(stat -c '%u:%a' "$1")" = 0:700 ]; }
metadata(){
 value=$("$admin" --status) || return 1
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
exchange(){
 oldhash=$1;newhash=$2;oldtag=$3;oldgeneration=$4;newtag=$5;newgeneration=$6;replacement=$7
 exact "$installed" "$oldhash" && exact "$replacement" "$newhash" && idle && metadata "$oldtag" "$oldgeneration" || return 1
 test ! -e "$stage/driver.incoming" && test ! -L "$stage/driver.incoming" || return 1
 cp -p "$replacement" "$stage/driver.incoming" || return 1
 chmod 755 "$stage/driver.incoming" || return 1
 exact "$stage/driver.incoming" "$newhash" && exact "$installed" "$oldhash" && idle || return 1
 "$admin" --quiesce "$oldtag" || return 1
 exact "$stage/driver.incoming" "$newhash" && exact "$installed" "$oldhash" || return 1
 mv -f "$stage/driver.incoming" "$installed" || return 1
 sync || return 1
 exact "$installed" "$newhash" || return 1
 previous=$(printf '%016x' "$oldgeneration") || return 1
 "$admin" --wait "$newtag" "$previous" || return 1
 metadata "$newtag" "$newgeneration"
}
private_dir "$stage" && private_dir "$cache4" && private_dir "$cache8" || exit 1
for name in .trial-once parent.pid driver.incoming stable.backup preflight.stdout preflight.stderr preflight.exit trial.stdout trial.stderr trial.exit state;do
 test ! -e "$stage/$name" && test ! -L "$stage/$name" || exit 1
done
exact "$stable" "$stable_hash" && exact "$installed" "$stable_hash" && exact "$candidate" "$candidate_hash" || exit 1
exact "$client" "$client_hash" && exact "$preflight" "$preflight_hash" && exact "$admin" "$admin_hash" && exact "$guard" "$guard_hash" || exit 1
exact_data "$gate" "$gate_hash" || exit 1
original=/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf
test -f "$original" && test ! -L "$original" && [ "$(stat -c '%u:%h:%a:%s' "$original")" = 0:1:600:18973870528 ] || exit 1
test -L /boot/home/config/non-packaged/add-ons/kernel/drivers/dev/misc/tenstorrent_pci_probe || exit 1
[ "$(readlink /boot/home/config/non-packaged/add-ons/kernel/drivers/dev/misc/tenstorrent_pci_probe)" = ../../bin/tenstorrent_pci_probe ] || exit 1
[ "$(stat -c '%d' "$stage")" = "$(stat -c '%d' "$installed")" ] || exit 1
idle && metadata 2026102100010012 20 && health || exit 1
mkdir "$stage/.trial-once"
printf '%s\n' "$$" > "$stage/parent.pid"
trap 'code=$?; if [ "$code" -ne 0 ]; then printf "FAILED_%s_NO_AUTO_RETRY_RESET_OR_RESTORE\n" "$code" > "$stage/state"; fi' EXIT
printf '%s\n' VERIFYING_MODEL_AND_CACHES_NO_DEVICE > "$stage/state"
if "$preflight" --verify-prepared "$cache4" "$cache8" > "$stage/preflight.stdout" 2> "$stage/preflight.stderr";then result=0;else result=$?;fi
printf '%s\n' "$result" > "$stage/preflight.exit"
[ "$result" = 0 ] && test ! -s "$stage/preflight.stderr" && \
 [ "$(tail -1 "$stage/preflight.stdout")" = 'PASS_HYBRID_PREFLIGHT 1071 BFP4 plus 273 BFP8 fragments, existing caches verified; NO_DEVICE_UPLOAD_MODEL_EXECUTION_OR_SPEED' ] || exit 69
exact "$client" "$client_hash" && exact "$preflight" "$preflight_hash" && exact "$candidate" "$candidate_hash" && exact_data "$gate" "$gate_hash" || exit 1
idle && metadata 2026102100010012 20 && health || exit 1
cp -p "$stable" "$stage/stable.backup"
exact "$stage/stable.backup" "$stable_hash" || exit 1
printf '%s\n' BACKUP_VERIFIED > "$stage/state"
exchange "$stable_hash" "$candidate_hash" 2026102100010012 20 2026110700010125 21 "$candidate" || exit 1
exact "$client" "$client_hash" && idle && metadata 2026110700010125 21 || exit 1
printf '%s\n' CANDIDATE_RUNNING_OWNERSHIP_UNKNOWN > "$stage/state"
if "$client" --run-hybrid-candidate "$cache4" "$cache8" 32 248319 > "$stage/trial.stdout" 2> "$stage/trial.stderr";then result=0;else result=$?;fi
printf '%s\n' "$result" > "$stage/trial.exit"
[ "$result" = 0 ] && test ! -s "$stage/trial.stderr" && exact_data "$gate" "$gate_hash" && awk -f "$gate" "$stage/trial.stdout" || exit 70
printf '%s\n' CANDIDATE_CLOSED_RESTORATION_PENDING > "$stage/state"
exact "$installed" "$candidate_hash" && idle && metadata 2026110700010125 21 || exit 71
exchange "$candidate_hash" "$stable_hash" 2026110700010125 21 2026102100010012 22 "$stage/stable.backup" || exit 72
exact "$installed" "$stable_hash" && idle && metadata 2026102100010012 22 && health || exit 73
printf '%s\n' CANDIDATE_CLOSED_STABLE_RESTORED_QUALITY_UNVERIFIED > "$stage/state"
echo HYBRID_FULL_ONE_SHOT_CLOSED_STABLE_RESTORED_NO_QUALITY_OR_SPEED_CLAIM
